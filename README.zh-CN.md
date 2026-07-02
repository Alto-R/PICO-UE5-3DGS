<div align="center">

# PicoGS 👁️‍🗨️

在 **PICO 4 Enterprise** 上用 VR 观看 **3D 高斯泼溅（Gaussian Splatting）** 场景，
通过 PICO 企业串流 SDK 逐帧记录**眼动 + 头部姿态**，再用离线 Python 管线把注视
**还原成场景里的 3D 空间点**。

[![Unreal](https://img.shields.io/badge/Unreal%20Engine-5.5-0E1128?style=flat-square&logo=unrealengine&logoColor=white)](#-环境要求)
[![PICO](https://img.shields.io/badge/PICO-4%20Enterprise-4200FF?style=flat-square)](#-环境要求)
[![3DGS](https://img.shields.io/badge/Gaussian%20Splatting-XV3dGS-00A86B?style=flat-square)](#-为什么是这套技术栈)
[![SteamVR](https://img.shields.io/badge/Runtime-OpenXR%20%2F%20SteamVR-1b2838?style=flat-square&logo=steam&logoColor=white)](#-为什么是这套技术栈)
[![Python](https://img.shields.io/badge/Python-3.11-3776AB?style=flat-square&logo=python&logoColor=white)](#-3d-注视点可视化)

**[English](README.md)** | **中文**

</div>

<br>

## 📑 快速导航

<div align="center">

| | | |
|:---:|:---:|:---:|
| [✨ 核心特性](#-核心特性) | [🏗️ 架构总览](#️-架构总览) | [🧭 为什么是这套技术栈](#-为什么是这套技术栈) |
| [🚀 快速开始](#-快速开始) | [👁️ 眼动记录](#️-眼动记录) | [🎯 3D 注视点可视化](#-3d-注视点可视化) |
| [⚙️ 关键文件](#️-关键文件) | [❓ 常见问题](#-常见问题) | [🗂️ 仓库内容](#️-仓库内容) |

</div>

<br>

## ✨ 核心特性

- **VR 观看 3DGS** —— 用 `XV3dGS` 插件渲染高斯泼溅场景，经 PICO 企业串流串到
  PICO 4 Enterprise。
- **眼动 + 头部姿态记录器** —— `UEyeGazeLoggerComponent` 每帧写一行 CSV（位置、
  视线 pitch/yaw、左右眼睁眼度/瞳孔/注视原点）。
- **PC 端取眼动** —— 数据来自 **企业串流 v1.x SDK** 的 `GetEyeTrackingData`（经串流
  链路），不依赖设备端。
- **通用运行时** —— 应用跑在通用 **OpenXR / SteamVR** 上（停用 PICO 专用插件），
  VRExpansion 角色 + Enhanced Input。
- **3D 注视还原** —— conda Python 工具把每帧视线射线与场景点云求交，输出可交互的
  Plotly 叠加图 + 3D 注视点 CSV。
- **兼容离线管线** —— CSV 结构对齐
  [`ni1o1/vr-gaze-pipeline`](https://github.com/ni1o1/vr-gaze-pipeline)。

## 🏗️ 架构总览

```text
PICO 4 Enterprise  ──串流──►  企业串流 (v1.x, SteamVR)  ◄──►  SteamVR
        ▲  眼动/头部                    │                          ▲
        │                              │ BStreamingSDK_GetEyeTrackingData │ OpenXR
        ▼  显示                         ▼                          │ 渲染
   Unreal Engine 5.5  ───────────────────────────────────────────────┘
        │  UEyeGazeLoggerComponent  (每帧头位 + 视线)
        ▼
   Saved/EyeLogs/<会话>.csv
        │
        ▼  visualization/visualize_gaze.py  (对齐 GS PLY, 射线求交)
   gaze_3d.html (交互)  +  gaze_3d_hits.csv (3D 注视点)
```

## 🧭 为什么是这套技术栈

三条硬约束决定了整体设计：

1. **`XV3dGS` 只支持 Win64** → 无法打包成 Android 原生 APK，只能在 **PC** 运行、
   **串流**到头显。
2. **眼动只在企业串流 *v1.x* SDK 里** —— v2.1/v2.2 已移除。所以 PC 端用 v1.x 的
   `GetEyeTrackingData` 取注视数据。
3. **v1.x 串流只走 SteamVR** → 工程改用**通用 OpenXR / SteamVR**（停用 `PICOOpenXR`
   插件），手柄绑定到 **Valve Index** 的 OpenXR 交互配置。

## 🚀 快速开始

### 环境要求

- **Unreal Engine 5.5** + Visual Studio 2022（C++）、Git LFS。
- 带眼动的 **PICO 4 Enterprise** 头显（系统版本兼容）。
- PC 与头显**都装 PICO 企业串流 v1.2.x**（v1.x —— v2.x 砍掉了眼动），以及 **SteamVR**。
- 把专有的 **BStreamingSDK** `.dll` / `.lib` 放到
  `Source/ThirdParty/BStreamingSDK/lib/Win64/`（见该目录 README）。
- 可视化用 conda Python 环境（见下）。

### 编译

```powershell
git clone https://github.com/Alto-R/PICO-UE5-3DGS.git
cd PICO-UE5-3DGS
git lfs install
```

右键 `PicoGS.uproject` → *Generate Visual Studio project files*，然后编译
**`PicoGSEditor`（Win64, Development）**，或直接打开 `.uproject` 让它编译。

### 运行并采集眼动

1. 先开 **SteamVR**，再开 PC 端 **企业串流（v1.x）**；连上头显并**进入 SteamVR 串流**
   （头显显示 SteamVR 房间）。
2. **两端都开眼动**：PC 端 *设置 → 通用 → "串流时支持眼动功能"*，以及**头显端**眼动
   （首次需**校准**）。
3. 打开工程，用 **VR 预览(VR Preview)** 播放，在场景里四处看。
4. 每次会话写入 `Saved/EyeLogs/<yyyymmdd_hhmmss>.csv`；日志出现
   `Eye-tracking backend active: BStreaming` 即表示正在记录。

### 可视化

```bash
conda create -n gaze3d python=3.11 numpy pandas scipy matplotlib
conda run -n gaze3d python -m pip install open3d plyfile trimesh plotly
conda run -n gaze3d python visualization/visualize_gaze.py --cast
```

浏览器打开 `visualization/gaze_3d.html` 转着看；3D 注视点同时导出到
`visualization/gaze_3d_hits.csv`。

## 👁️ 眼动记录

`UEyeGazeLoggerComponent`（挂在 `APicoVRCharacter` 上）在 `BeginPlay` 选择后端：

- **BStreaming**（首选）—— 加载 `BStreamingSDK.dll`，调 `BStreamingSDK_Init`，每帧读
  `BStreamingSDK_GetEyeTrackingData`（`combined_eye_gaze_vector` 经 HMD 姿态转成世界系
  pitch/yaw）。
- **OpenXR**（兜底）—— 原生运行时下用 `XR_EXT_eye_gaze_interaction`。

CSV 结构（米、度）：

```text
index,Time,vr_X,vr_Y,vr_Z,vr_Pitch,vr_Yaw
  [,left_openness,right_openness,left_pupil,right_pupil,gaze_point_x,gaze_point_y,gaze_point_z,eye_status]
```

- 用 `eye_status == 3` 过滤有效帧（其余是眨眼/丢跟踪）。
- `gaze_point_*` 是**眼球原点(≈0)**，**不是**场景命中点。
- dll 延迟加载：缺 SDK / 没串流时优雅回退，不会崩。

## 🎯 3D 注视点可视化

`visualization/visualize_gaze.py` 还原 3D 注视点：

- **每帧射线**：起点 = 头位，方向由 `vr_Pitch/vr_Yaw` 得出。
- **3D 注视点** = 射线 × 场景点云（KD-tree 步进求交）。
- **对齐**（PLY → UE 世界，已为 `splat_50000` 设为默认）：
  `p_ue = conv(p_ply) + actor_loc`，其中 `--gs-conv colmap`（`UE = (z, x, -y)`）、
  `--actor-loc` 取自 UE 关卡里 GS actor 的位置。换场景改 `--ply` / `--actor-loc` /
  `--gs-conv`。

产物：交互式 `gaze_3d.html`、`gaze_3d_hits.csv`，以及 `final_preview.py` 生成的俯视图 +
停留热力图。详见 [`visualization/README.md`](visualization/README.md)。

## ⚙️ 关键文件

| 路径 | 作用 |
| --- | --- |
| `Source/PicoGS/Private/EyeGaze/EyeGazeLoggerComponent.cpp` | 眼动 + 头位 CSV 记录器（BStreaming / OpenXR 后端） |
| `Source/PicoGS/Private/PicoVRCharacter/` | VRExpansion 角色 + Enhanced Input |
| `Source/ThirdParty/BStreamingSDK/` | PICO v1.x SDK 的 UE 外部模块（头文件 + Build.cs；二进制被忽略） |
| `Content/PICO/BP_PicoVRCharacter`、`.../Input/IMC_VR` | VR 角色蓝图 + 输入绑定（Valve Index 档） |
| `Content/Maps/L_GSTest` | 含高斯泼溅 actor 的测试关卡 |
| `Config/DefaultEngine.ini` | 眼动 + 渲染相关设置 |
| `visualization/visualize_gaze.py` | 3D 注视还原 + Plotly 叠加图 |

## ❓ 常见问题

**眼动 CSV 是空的 / `GetEyeTrackingData` 返回非 0。**
用 **v1.x** 企业串流（v2.x 没有眼动 SDK），PC 端和头显端**都要开眼动**、头显要**校准**，
且串流会话在跑。

**头显停在 SteamVR 房间，画面只在 PC 上出现。**
要在头显里**真正进入 SteamVR 串流**（不是只连上），并用 **VR 预览** 播放，UE 才会把
立体画面提交给 SteamVR。

**手柄没反应。**
停用 `PICOOpenXR` 后 `PICOTouch_*` 键失效。把 `IMC_VR`（IA_Move / IA_Turn）重绑到
**Valve Index** 的 MotionController 摇杆键。

**`pico_et_ft_bt_bridge.exe` 报 `0xc000012d`。**
提交内存超限 —— 调大 Windows 页面文件、关掉占内存的程序；16GB 同时跑 UE + SteamVR +
企业串流很紧张。

**编译卡在 ~0% CPU、不 spawn 编译器。**
Unreal Build Accelerator 的内存门槛。在 `BuildConfiguration.xml` 设
`bAllowUBALocalExecutor=false`，或释放内存 / 调大页面文件。

## 🗂️ 仓库内容

**纳入**：C++ 源码、工程与插件配置、随仓库的插件源码/运行库，以及**项目专属**的
`Content`（Maps/L_GSTest、PICO VR 角色 + `IMC_VR`、`BP_Code`、`assets` 等），走 **Git LFS**。

**不纳入**（已忽略）：`Binaries/`、`Intermediate/`、`DerivedDataCache/`、`Saved/`、
商城模板资源（FirstPerson / Fab / Mannequin）、大的 `splat_*.uasset` 与 `*.ply`、
专有的 `BStreamingSDK` 二进制、生成的 `visualization/*.html`，以及 `*.apk`。

## 📚 致谢

- [Xverse `XV3dGS`](http://www.xverse.cn/) —— Unreal 的高斯泼溅渲染插件。
- `VRExpansionPlugin` —— VR 交互（随仓库；许可见其目录）。
- PICO OpenXR / 企业串流 SDK —— 设备与串流集成。
- [`ni1o1/vr-gaze-pipeline`](https://github.com/ni1o1/vr-gaze-pipeline) —— 离线 3D 语义
  注视分析（CSV 结构参考）。

## 许可

工程许可尚未最终确定。第三方插件与 SDK 各自的许可条款保留在其目录内。
