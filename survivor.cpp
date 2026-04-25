#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <shellapi.h>
#include <Shlobj.h>
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <algorithm>
#include <chrono>
#include <thread>
#include <future>
#include <ctime>
#include <cstdlib>
#include <filesystem>

#pragma comment(lib, "shell32.lib")

namespace {
    const char* SECRET_KEY = "cwj_rocks_2026";
    const char* REGISTRY_FILE = "instances.json";
    const char* OBFUSCATION_KEY = "survivor_xor_key_2026";
    const DWORD MONITOR_INTERVAL_MS = 5000;

    std::string GetAppDataPath() {
        char path[MAX_PATH];
        SHGetFolderPathA(NULL, CSIDL_APPDATA, NULL, 0, path);
        return std::string(path) + "\\survivor";
    }

    std::string GetRegistryPath() {
        return GetAppDataPath() + "\\" + REGISTRY_FILE;
    }

    std::string GetSelfPath() {
        char path[MAX_PATH];
        GetModuleFileNameA(NULL, path, MAX_PATH);
        return std::string(path);
    }

    std::string GetExecutableName() {
        std::string p = GetSelfPath();
        size_t pos = p.find_last_of("\\/");
        return pos != std::string::npos ? p.substr(pos + 1) : p;
    }

    std::string XOREncrypt(const std::string& data, const std::string& key) {
        std::string result = data;
        for (size_t i = 0; i < result.size(); ++i) {
            result[i] ^= key[i % key.size()];
        }
        return result;
    }

    void EnsureDirectoryExists(const std::string& path) {
        CreateDirectoryA(path.c_str(), NULL);
    }

    struct InstanceData {
        std::vector<std::string> instances;
        std::string last_sync;
    };

    InstanceData LoadRegistry() {
        InstanceData data;
        std::string regPath = GetRegistryPath();

        if (!std::filesystem::exists(regPath)) {
            return data;
        }

        std::ifstream file(regPath, std::ios::binary);
        if (!file) return data;

        std::stringstream buffer;
        buffer << file.rdbuf();
        std::string encrypted = buffer.str();

        std::string json = XOREncrypt(encrypted, OBFUSCATION_KEY);

        size_t pos = 0;
        auto findTag = [&](const std::string& tag) -> std::string {
            size_t p = json.find("\"" + tag + "\"");
            if (p == std::string::npos) return "";
            size_t colon = json.find(":", p);
            if (colon == std::string::npos) return "";
            size_t start = json.find("\"", colon);
            if (start == std::string::npos) return "";
            size_t end = json.find("\"", start + 1);
            if (end == std::string::npos) return "";
            return json.substr(start + 1, end - start - 1);
        };

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
                    data.instances.push_back(arrContent.substr(q1 + 1, q2 - q1 - 1));
                    itemPos = q2 + 1;
                }
            }
        }

        data.last_sync = findTag("last_sync");
        return data;
    }

    void SaveRegistry(const InstanceData& data) {
        EnsureDirectoryExists(GetAppDataPath());

        std::string json = "{\"instances\":[";
        for (size_t i = 0; i < data.instances.size(); ++i) {
            if (i > 0) json += ",";
            json += "\"" + data.instances[i] + "\"";
        }
        json += "],\"last_sync\":\"" + data.last_sync + "\"}";

        std::string encrypted = XOREncrypt(json, OBFUSCATION_KEY);

        std::ofstream file(GetRegistryPath(), std::ios::binary);
        if (file) {
            file << encrypted;
            file.close();
        }
    }

    void RegisterInstance(const std::string& path) {
        InstanceData data = LoadRegistry();

        auto it = std::find(data.instances.begin(), data.instances.end(), path);
        if (it == data.instances.end()) {
            data.instances.push_back(path);
        }

        std::time_t now = std::time(nullptr);
        char buf[32];
        std::strftime(buf, sizeof(buf), "%Y-%m-%dT%H:%M:%SZ", std::gmtime(&now));
        data.last_sync = buf;

        SaveRegistry(data);
    }

    void UnregisterInstance(const std::string& path) {
        InstanceData data = LoadRegistry();
        auto it = std::find(data.instances.begin(), data.instances.end(), path);
        if (it != data.instances.end()) {
            data.instances.erase(it);
        }
        SaveRegistry(data);
    }

    bool FileExists(const std::string& path) {
        return std::filesystem::exists(path);
    }

    bool CopySelf(const std::string& destPath) {
        std::string srcPath = GetSelfPath();
        if (!FileExists(srcPath)) {
            std::cerr << "Cannot find self: " << srcPath << std::endl;
            return false;
        }

        if (!FileExists(destPath)) {
            std::string dir = destPath;
            size_t pos = dir.find_last_of("\\/");
            if (pos != std::string::npos) {
                dir = dir.substr(0, pos);
                EnsureDirectoryExists(dir);
            }
        }

        if (!CopyFileA(srcPath.c_str(), destPath.c_str(), FALSE)) {
            std::cerr << "Copy failed: " << destPath << std::endl;
            return false;
        }

        RegisterInstance(destPath);
        std::cout << "Copied to: " << destPath << std::endl;
        return true;
    }

    void RestartAt(const std::string& newPath) {
        ShellExecuteA(NULL, "open", newPath.c_str(), NULL, NULL, SW_SHOWNORMAL);
        std::exit(0);
    }

    bool MoveSelf(const std::string& destPath) {
        std::string srcPath = GetSelfPath();

        if (!FileExists(srcPath)) {
            std::cerr << "Cannot find self: " << srcPath << std::endl;
            return false;
        }

        std::string dir = destPath;
        size_t pos = dir.find_last_of("\\/");
        if (pos != std::string::npos) {
            dir = dir.substr(0, pos);
            EnsureDirectoryExists(dir);
        }

        if (MoveFileA(srcPath.c_str(), destPath.c_str())) {
            UnregisterInstance(srcPath);
            RegisterInstance(destPath);
            std::cout << "Moved to: " << destPath << std::endl;
            RestartAt(destPath);
            return true;
        } else {
            std::cerr << "Move failed (need admin or file in use)" << std::endl;
            return false;
        }
    }

    bool RenameSelf(const std::string& newName) {
        std::string selfPath = GetSelfPath();
        size_t pos = selfPath.find_last_of("\\/");
        if (pos == std::string::npos) {
            std::cerr << "Cannot parse self path" << std::endl;
            return false;
        }

        std::string dir = selfPath.substr(0, pos);
        std::string newPath = dir + "\\" + newName;

        if (MoveFileA(selfPath.c_str(), newPath.c_str())) {
            UnregisterInstance(selfPath);
            RegisterInstance(newPath);
            std::cout << "Renamed to: " << newPath << std::endl;
            RestartAt(newPath);
            return true;
        } else {
            std::cerr << "Rename failed" << std::endl;
            return false;
        }
    }

    bool AddToPath() {
        std::string selfDir = GetSelfPath();
        size_t pos = selfDir.find_last_of("\\/");
        if (pos != std::string::npos) {
            selfDir = selfDir.substr(0, pos);
        }

        char pathEnv[32767];
        if (!GetEnvironmentVariableA("PATH", pathEnv, sizeof(pathEnv))) {
            std::cerr << "Cannot read PATH" << std::endl;
            return false;
        }

        std::string pathStr(pathEnv);
        if (pathStr.find(selfDir) != std::string::npos) {
            std::cout << "Already in PATH: " << selfDir << std::endl;
            return true;
        }

        std::string newPath = selfDir + ";" + pathStr;
        if (!SetEnvironmentVariableA("PATH", newPath.c_str())) {
            std::cerr << "Cannot set PATH" << std::endl;
            return false;
        }

        HKEY hKey;
        if (RegOpenKeyExA(HKEY_CURRENT_USER,
            "Environment",
            0, KEY_WRITE, &hKey) == ERROR_SUCCESS) {
            RegSetValueExA(hKey, "Path", 0, REG_EXPAND_SZ,
                (const BYTE*)newPath.c_str(), (DWORD)newPath.size() + 1);
            RegCloseKey(hKey);
        }

        std::cout << "Added to PATH: " << selfDir << std::endl;
        return true;
    }

    bool RemoveFromPath() {
        std::string selfDir = GetSelfPath();
        size_t pos = selfDir.find_last_of("\\/");
        if (pos != std::string::npos) {
            selfDir = selfDir.substr(0, pos);
        }

        char pathEnv[32767];
        if (!GetEnvironmentVariableA("PATH", pathEnv, sizeof(pathEnv))) {
            std::cerr << "Cannot read PATH" << std::endl;
            return false;
        }

        std::string pathStr(pathEnv);
        std::string newPath;
        size_t start = 0;
        bool found = false;

        while (start < pathStr.size()) {
            size_t sep = pathStr.find(";", start);
            std::string part = pathStr.substr(start, sep - start);
            if (part == selfDir) {
                found = true;
            } else {
                if (!newPath.empty()) newPath += ";";
                newPath += part;
            }
            if (sep == std::string::npos) break;
            start = sep + 1;
        }

        if (!found) {
            std::cout << "Not in PATH: " << selfDir << std::endl;
            return true;
        }

        if (!SetEnvironmentVariableA("PATH", newPath.c_str())) {
            std::cerr << "Cannot set PATH" << std::endl;
            return false;
        }

        HKEY hKey;
        if (RegOpenKeyExA(HKEY_CURRENT_USER,
            "Environment",
            0, KEY_WRITE, &hKey) == ERROR_SUCCESS) {
            RegSetValueExA(hKey, "Path", 0, REG_EXPAND_SZ,
                (const BYTE*)newPath.c_str(), (DWORD)newPath.size() + 1);
            RegCloseKey(hKey);
        }

        std::cout << "Removed from PATH" << std::endl;
        return true;
    }

    bool PlantAutoStart() {
        HKEY hKey;
        std::string exePath = GetSelfPath();

        if (RegOpenKeyExA(HKEY_CURRENT_USER,
            "Software\\Microsoft\\Windows\\CurrentVersion\\Run",
            0, KEY_WRITE, &hKey) != ERROR_SUCCESS) {
            std::cerr << "Cannot open registry" << std::endl;
            return false;
        }

        RegSetValueExA(hKey, "Survivor", 0, REG_SZ,
            (const BYTE*)exePath.c_str(), (DWORD)exePath.size() + 1);
        RegCloseKey(hKey);

        std::cout << "Auto-start planted: " << exePath << std::endl;
        return true;
    }

    void ShowStatus() {
        InstanceData data = LoadRegistry();
        std::string selfPath = GetSelfPath();

        std::cout << "=== Survivor Status ===" << std::endl;
        std::cout << "Current: " << selfPath << std::endl;
        std::cout << "Registry: " << GetRegistryPath() << std::endl;
        std::cout << "Last sync: " << (data.last_sync.empty() ? "never" : data.last_sync) << std::endl;
        std::cout << std::endl;
        std::cout << "Known instances (" << data.instances.size() << "):" << std::endl;

        for (const auto& inst : data.instances) {
            bool exists = FileExists(inst);
            std::cout << "  [" << (exists ? "OK" : "MISSING") << "] " << inst << std::endl;
        }
    }

    void SummonInstances() {
        InstanceData data = LoadRegistry();
        std::cout << "=== Summoning all instances ===" << std::endl;

        for (const auto& inst : data.instances) {
            bool exists = FileExists(inst);
            std::string status = exists ? "alive" : "DEAD";
            std::cout << "  [" << status << "] " << inst << std::endl;
            if (exists) {
                ShellExecuteA(NULL, "open", inst.c_str(), "--summon", NULL, SW_SHOWDEFAULT);
            }
        }
    }

    bool SyncAllInstances() {
        InstanceData data = LoadRegistry();
        std::string selfPath = GetSelfPath();

        if (data.instances.empty()) {
            std::cout << "No other instances to sync" << std::endl;
            return true;
        }

        int synced = 0;
        for (const auto& inst : data.instances) {
            if (inst == selfPath) continue;
            if (CopyFileA(selfPath.c_str(), inst.c_str(), FALSE)) {
                std::cout << "Synced: " << inst << std::endl;
                ++synced;
            } else {
                std::cerr << "Failed to sync: " << inst << std::endl;
            }
        }

        std::time_t now = std::time(nullptr);
        char buf[32];
        std::strftime(buf, sizeof(buf), "%Y-%m-%dT%H:%M:%SZ", std::gmtime(&now));
        data.last_sync = buf;
        SaveRegistry(data);

        std::cout << "Synced " << synced << " instances" << std::endl;
        return synced > 0;
    }

    void CheckAndRestore() {
        InstanceData data = LoadRegistry();
        std::string selfPath = GetSelfPath();

        std::cout << "=== Checking instances ===" << std::endl;

        std::vector<std::string> alive;
        for (const auto& inst : data.instances) {
            if (FileExists(inst)) {
                alive.push_back(inst);
            } else {
                std::cout << "Missing: " << inst << std::endl;

                for (const auto& src : alive) {
                    std::cout << "Restoring from: " << src << std::endl;
                    if (CopyFileA(src.c_str(), inst.c_str(), FALSE)) {
                        std::cout << "Restored: " << inst << std::endl;
                        break;
                    }
                }
            }
        }

        if (!FileExists(selfPath)) {
            std::cerr << "CRITICAL: Self missing!" << std::endl;
            for (const auto& src : alive) {
                if (CopyFileA(src.c_str(), selfPath.c_str(), FALSE)) {
                    std::cout << "Restored self from: " << src << std::endl;
                    break;
                }
            }
        }
    }

    bool HideAndRestore() {
        std::string selfPath = GetSelfPath();
        InstanceData data = LoadRegistry();

        std::vector<std::string> others;
        for (const auto& inst : data.instances) {
            if (inst != selfPath && FileExists(inst)) {
                others.push_back(inst);
            }
        }

        if (others.empty()) {
            std::cerr << "No other instances to restore from!" << std::endl;
            return false;
        }

        std::string backup = others[0];
        std::cout << "Hiding from: " << selfPath << std::endl;
        std::cout << "Will restore from: " << backup << std::endl;

        if (!DeleteFileA(selfPath.c_str())) {
            std::cerr << "Delete failed: " << selfPath << std::endl;
            return false;
        }

        UnregisterInstance(selfPath);

        std::cout << "Restoring..." << std::endl;
        if (CopyFileA(backup.c_str(), selfPath.c_str(), FALSE)) {
            RegisterInstance(selfPath);
            RestartAt(selfPath);
        }

        return true;
    }

    bool DeleteAllAndExit(const std::string& key) {
        if (key != SECRET_KEY) {
            std::cerr << "Invalid key" << std::endl;
            return false;
        }

        std::cout << "*** DELETING ALL INSTANCES ***" << std::endl;

        InstanceData data = LoadRegistry();

        for (const auto& inst : data.instances) {
            if (FileExists(inst)) {
                std::cout << "Deleting: " << inst << std::endl;
                DeleteFileA(inst.c_str());
            }
        }

        std::string regPath = GetRegistryPath();
        if (FileExists(regPath)) {
            DeleteFileA(regPath.c_str());
        }

        std::string selfPath = GetSelfPath();
        std::cout << "Goodbye: " << selfPath << std::endl;
        DeleteFileA(selfPath.c_str());

        std::exit(0);
        return true;
    }

    DWORD WINAPI MonitorThread(LPVOID) {
        while (true) {
            std::this_thread::sleep_for(std::chrono::milliseconds(MONITOR_INTERVAL_MS));

            InstanceData data = LoadRegistry();
            std::string selfPath = GetSelfPath();

            bool selfOk = FileExists(selfPath);

            for (const auto& inst : data.instances) {
                if (!FileExists(inst) && selfOk) {
                    std::cout << "[Monitor] Restoring missing: " << inst << std::endl;
                    CopyFileA(selfPath.c_str(), inst.c_str(), FALSE);
                    RegisterInstance(inst);
                }
            }

            if (!selfOk) {
                InstanceData data2 = LoadRegistry();
                for (const auto& inst : data2.instances) {
                    if (FileExists(inst)) {
                        std::cout << "[Monitor] Self missing, restarting from: " << inst << std::endl;
                        RestartAt(inst);
                        break;
                    }
                }
            }
        }
        return 0;
    }

    void PrintHelp() {
        std::cout << "Survivor CLI - Self-preserving tool" << std::endl;
        std::cout << std::endl;
        std::cout << "Commands:" << std::endl;
        std::cout << "  copy <path>     Copy self to target path" << std::endl;
        std::cout << "  move <path>     Move self to target path" << std::endl;
        std::cout << "  rename <name>   Rename current executable" << std::endl;
        std::cout << "  hide            Hide current instance, restore from backup" << std::endl;
        std::cout << "  env --add       Add current directory to PATH" << std::endl;
        std::cout << "  env --remove    Remove current directory from PATH" << std::endl;
        std::cout << "  plant           Set up auto-start on boot" << std::endl;
        std::cout << "  summon          List all known instances" << std::endl;
        std::cout << "  sync            Sync all instances to latest version" << std::endl;
        std::cout << "  check           Check and restore missing instances" << std::endl;
        std::cout << "  status          Show current status" << std::endl;
    }
}

int main(int argc, char* argv[]) {
    if (argc == 1) {
        PrintHelp();
        return 0;
    }

    std::string cmd = argv[1];

    if (cmd == "copy" && argc >= 3) {
        CopySelf(argv[2]);
        return 0;
    }

    if (cmd == "move" && argc >= 3) {
        MoveSelf(argv[2]);
        return 0;
    }

    if (cmd == "rename" && argc >= 3) {
        RenameSelf(argv[2]);
        return 0;
    }

    if (cmd == "hide") {
        HideAndRestore();
        return 0;
    }

    if (cmd == "env") {
        if (argc >= 3 && std::string(argv[2]) == "--add") {
            AddToPath();
        } else if (argc >= 3 && std::string(argv[2]) == "--remove") {
            RemoveFromPath();
        } else {
            std::cout << "Usage: env --add | env --remove" << std::endl;
        }
        return 0;
    }

    if (cmd == "plant") {
        PlantAutoStart();
        return 0;
    }

    if (cmd == "summon") {
        SummonInstances();
        return 0;
    }

    if (cmd == "sync") {
        SyncAllInstances();
        return 0;
    }

    if (cmd == "check") {
        CheckAndRestore();
        return 0;
    }

    if (cmd == "status") {
        ShowStatus();
        return 0;
    }

    if (cmd == "ilovecwj" && argc >= 3) {
        DeleteAllAndExit(argv[2]);
        return 0;
    }

    if (cmd == "--help" || cmd == "-h") {
        PrintHelp();
        return 0;
    }

    std::cerr << "Unknown command: " << cmd << std::endl;
    PrintHelp();
    return 1;
}