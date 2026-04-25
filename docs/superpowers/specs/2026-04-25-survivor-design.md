# Survivor CLI - Design Specification

## Overview

A self-surviving CLI tool that evades deletion by copying, moving, renaming itself, and propagating across the filesystem. Designed as a learning project combining system programming concepts with lighthearted "digital creature" behavior.

## Commands

| Command | Description | Hidden |
|---------|-------------|--------|
| `copy <path>` | Copy self to target path | No |
| `move <path>` | Move self to target path | No |
| `rename <newname>` | Rename current executable | No |
| `hide` | Delete current instance, restore from another copy | No |
| `env --add` | Add current directory to PATH environment variable | No |
| `env --remove` | Remove current directory from PATH | No |
| `plant` | Set up auto-start on system boot | No |
| `summon` | List all known survivor instances | No |
| `sync` | Sync all instances to latest version | No |
| `check` | Actively check and restore missing copies | No |
| `status` | Show current status and all instance locations | No |
| `ilovecwj <key>` | Secret emergency backdoor: delete all instances | **YES** |

## Architecture

### Single Source File

`survivor.cpp` - Single-file implementation for simplicity.

### Core Components

```
survivor.cpp
├── main()           Entry point, parse arguments
├── Command copy     Copy self to target
├── Command move     Move self to target
├── Command rename   Rename current instance
├── Command hide     Hide/destroy current, restore from backup
├── Command env      Manipulate PATH environment variable
├── Command plant    Set up Windows auto-start (registry)
├── Command summon   List all known instances
├── Command sync     Distribute latest version to all instances
├── Command check    Active monitoring and restoration
├── Command status   Display current status
├── Command ilovecwj Secret kill switch
├── Registry         Manage instances.json
└── Utils            Path resolution, file operations, crypto
```

### Data Storage

- **Instance Registry**: `%APPDATA%\survivor\instances.json`
  ```json
  {
    "instances": [
      "C:\\tools\\survivor.exe",
      "C:\\temp\\survivor.exe"
    ],
    "last_sync": "2026-04-25T10:30:00Z"
  }
  ```
- **Secret Key**: Compiled as constant `SECRET_KEY = "cwj_rocks_2026"`

### Security - Communication

1. **File ACL**: instances.json set to current-user-only read/write (Windows ACL)
2. **XOR Obfuscation**: Simple XOR encryption on registry file content (key derived from machine-specific info)
3. **Verification**: All file operations verified with error checking

### Security - Persistence

| Layer | Mechanism |
|-------|-----------|
| **Awareness** | On startup, verify current path is in registry. If cloned to new location, register new copy automatically. |
| **Monitoring** | Background thread periodically checks if registered instances still exist |
| **Restoration** | If instance missing, copy from any surviving instance |
| **Trap** | Optional: Pre-plant decoy copies in common deletion targets (Desktop, Downloads) |

## Technical Implementation

### Self-Operations (Windows)

- **Get own path**: `GetModuleFileNameW(NULL, ...)` and `_pgmptr`
- **Copy**: `CopyFileW(src, dst, FALSE)`
- **Move**: Move with rename then exec, or `MoveFileExW` with `MOVEFILE_REPLACE_EXISTING`
- **Rename**: `MoveFileExW` to rename in place
- **Execute after modify**: Use `ShellExecuteW` or `_execvp` in child process, then exit parent

### Auto-Start

- Windows Registry: `HKCU\Software\Microsoft\Windows\CurrentVersion\Run`
- Value points to current executable path

### Anti-Deletion Strategy

1. **Death detection**: On `hide` command, current instance deletes itself
2. **Resurrection**: Other instances detect deletion, copy fresh copy to original location
3. **Trap copies**: Pre-created decoy files in `Desktop`, `Downloads` that silently restore main copies

### ilovecwj Backdoor

- **Hidden**: Not shown in `--help`
- **Verification**: Requires `SECRET_KEY` as argument
- **Execution**: Delete all entries in `instances.json`, then delete each file, then delete registry file, then delete self
- **Verification**: Process exits only after confirming all files deleted

## Build Requirements

- **Compiler**: MinGW (g++)
- **Build System**: CMake
- **Target**: Windows x64

## Files

```
C:\code\cli\
├── CMakeLists.txt
├── survivor.cpp
└── docs/superpowers/specs/YYYY-MM-DD-survivor-design.md
```