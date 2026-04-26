# Survivor CLI

**Version 2.3.0** | Self-preserving autonomous CLI tool for Windows

---

## Language Selection

- **English** (current)
- [中文文档](README_CN.md)

---

## Project Description

Survivor is a self-preserving autonomous command-line tool designed for research and educational purposes. It demonstrates advanced persistence techniques by copying, moving, renaming, and propagating itself across the filesystem while maintaining persistent background operations.

**Disclaimer**: This software is provided exclusively for learning and security research. The author assumes no liability for any misuse or damage caused by this tool.

---

## Version History

| Version | Release Date | Key Changes |
|---------|--------------|-------------|
| 1.0.0 | 2026-04-25 | Initial release: Basic copy, move, rename, and persistence functions |
| 2.0.0 | 2026-04-25 | Background daemon threads, self-healing mechanisms, intelligent target selection |
| 2.1.0 | 2026-04-25 | Timestomping, NTFS ADS support, scheduled task camouflage, integrity verification |
| 2.2.0 | 2026-04-25 | Modular architecture, anti-debugging, VM detection, privilege detection, self-recovery |
| 2.3.0 | 2026-04-25 | Stealth enhancements, WMI persistence, DLL injection, remote updates, P2P encrypted communication, Windows service mode, COM hijacking, rootkit capabilities |

---

## Architecture

### Modular Design

The codebase is organized into distinct functional modules:

```
survivor::
├── utils        # Common utility functions
├── detection    # Environment detection (VM, debugging, analysis environments)
├── injection    # Process injection techniques
├── persistence  # Registry and scheduled task management
├── ads          # NTFS Alternate Data Streams
├── spread       # Propagation logic and target selection
├── registry     # Instance registry management
├── cleanup      # Log and artifact cleanup
├── usb          # USB mass storage propagation
├── network      # Network scanning capabilities
├── loops        # Background thread implementations
├── commands     # Command-line interface handlers
├── daemon       # Daemon mode initialization
├── stealth      # File attribute manipulation
├── wmi          # WMI event subscription persistence
├── dll          # DLL payload creation and injection
├── update       # Remote self-update mechanisms
├── p2p          # Peer-to-peer encrypted communication
├── com          # COM component hijacking
├── rootkit      # Process hiding and rootkit techniques
└── service      # Windows Service mode
```

### Core Capabilities

| Feature | Description |
|---------|-------------|
| **Self-Replication** | Copies itself to arbitrary filesystem locations |
| **Self-Relocation** | Moves to new paths while maintaining execution |
| **Dynamic Renaming** | Disguises as system binaries (svchost.exe, rundll32.exe, etc.) |
| **Instance Recovery** | Deletes current instance and recovers from backups |
| **PATH Management** | Adds or removes its directory from system PATH |
| **Persistence** | Maintains presence via registry and scheduled tasks |

---

## Environment Awareness

| Detection Capability | Method | Adaptive Response |
|--------------------|--------|-------------------|
| **Virtual Machine Detection** | Checks for VirtualBox, VMware, QEMU artifacts | Propagation interval increased by 10x |
| **Monitoring Tool Detection** | Scans for Process Monitor, Process Hacker, etc. | Switches to silent mode (suppresses output) |
| **Analysis Environment** | Detects sandboxes, analysis markers in computer names | Extreme caution mode (50x delay multiplier) |
| **Debugger Detection** | Uses IsDebuggerPresent and timing checks | Activates silent mode |
| **Privilege Detection** | Verifies administrator privileges | Displayed in status output |

---

## Autonomous Operations

When executed without arguments, the following background threads are launched:

| Thread | Function | Interval |
|--------|----------|----------|
| **SpreadingLoop** | Randomly copies, moves, or renames to user directories | 30s - 5min |
| **MonitoringLoop** | Detects missing instances, triggers counterattack recovery | 500ms |
| **RestorationLoop** | Periodically verifies and restores missing instances | 30s |
| **GuardianLoop** | Injects into critical processes, monitors and re-injects | 5min |
| **USBMonitorLoop** | Detects USB insertion, propagates to removable media | 5s |
| **CleanupLoop** | Removes log files and clears event logs | 30min |

---

## Security Features

- **Path Validation**: Prevents directory traversal attacks via `..` sequences
- **XOR-Obfuscated Registry**: Instance data stored with basic obfuscation
- **Checksum Verification**: Built-in FNV-1a hash for integrity validation
- **Emergency Recovery**: Authenticated command to remove all instances and traces

---

## Command-Line Interface

### Usage

```bash
survivor.exe [command] [arguments]
```

### Commands

| Command | Description | Example |
|---------|-------------|---------|
| *(no arguments)* | Start in daemon mode | `survivor.exe` |
| `copy <path>` | Copy to specified target path | `survivor.exe copy C:\temp\test.exe` |
| `move <path>` | Relocate to target path | `survivor.exe move D:\tools\survivor.exe` |
| `rename <name>` | Rename current instance | `survivor.exe rename svchost.exe` |
| `hide` | Self-delete and recover from backup | `survivor.exe hide` |
| `env --add` | Add directory to system PATH | `survivor.exe env --add` |
| `env --remove` | Remove directory from PATH | `survivor.exe env --remove` |
| `plant` | Install persistence mechanisms | `survivor.exe plant` |
| `status` | Display detailed runtime status | `survivor.exe status` |
| `summon` | List all registered instances | `survivor.exe summon` |
| `sync` | Synchronize all instances | `survivor.exe sync` |
| `check` | Verify and restore instances | `survivor.exe check` |
| `spread` | Manually trigger propagation | `survivor.exe spread` |
| `guardian` | Inject into system processes | `survivor.exe guardian` |
| `version` | Display version information | `survivor.exe version` |
| `verify` | Validate integrity and checksum | `survivor.exe verify` |

### Emergency Command

| Command | Description | Authentication Key |
|---------|-------------|-------------------|
| `ilovecwj <key>` | Remove all instances and cleanup | `cwj_rocks_2026` |

---

## Build Instructions

### Prerequisites

- MinGW-w64 g++ (C++17 support) or MSVC 2019+
- CMake 3.10 or higher
- Windows SDK

### Build Process

```bash
# Create and enter build directory
mkdir build
cd build

# Configure with CMake
cmake -G "MinGW Makefiles" ..

# Compile
mingw32-make

# Display help
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

## Technical Specifications

### Registry Storage

Instance registry (XOR-obfuscated):
```
%APPDATA%\survivor\registry.dat
```

### Configuration Structure

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

### Runtime Variables

| Variable | Description |
|----------|-------------|
| `g_running` | Background thread execution control |
| `g_daemon` | Daemon mode status |
| `g_spreading` | Propagation state |
| `g_spread_multiplier` | Propagation interval multiplier |
| `g_verbose` | Log output control |

---

## Testing Protocol

### Safety Notice

**This software should only be tested in isolated virtual environments.** It autonomously propagates and may be difficult to completely remove from a system.

### Recommended Testing Environment

1. Create a Windows virtual machine using VirtualBox or VMware
2. Take a snapshot before executing any tests
3. Run all tests within the virtual machine
4. Restore the snapshot after testing to ensure complete cleanup

### Basic Verification

```batch
survivor.exe --help
survivor.exe version
survivor.exe status
survivor.exe verify
```

---

## Removal Instructions

### Standard Removal

```batch
survivor.exe ilovecwj cwj_rocks_2026
```

### Manual Cleanup

```batch
reg delete "HKCU\Software\Microsoft\Windows\CurrentVersion\Run" /v Survivor /f
reg delete "HKCU\Software\Microsoft\Windows\CurrentVersion\Run" /v WindowsUpdateCheck /f
rmdir /s /q "%APPDATA%\survivor"
```

---

## Authentication

```
cwj_rocks_2026
```

---

**Last Updated**: 2026-04-25  
**Author**: AK60000  
**License**: MIT

[中文文档](README_CN.md)
