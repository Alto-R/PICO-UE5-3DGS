# PicoGS

PicoGS 是一个基于 Unreal Engine 5.5 的 PICO/OpenXR VR 项目，用于浏览和测试 3D Gaussian Splatting 内容。项目集成了 PICO XR、OpenXR、VR 交互能力以及 XVerse 3D Gaussian Splatting 插件。

## 功能概览

- Unreal Engine 5.5 C++ 项目。
- 面向 VR 启动，配置中启用了 `bStartInVR=True`。
- 通过 `PICOOpenXR` 和 `OpenXR` 使用 OpenXR 运行路径。
- 通过 `OnlineSubsystemPICO` 及相关 PICO 插件支持 PICO 平台能力。
- 通过 `XV3dGS` 插件支持 Gaussian Splatting。
- 通过 `VRExpansionPlugin` 支持 VR 交互；该插件以普通项目文件方式纳入仓库。
- 包含用于日志写入、HTTP 结果上传、静态网格顶点导出的 C++ 工具代码。

## 仓库内容

本仓库主要保存项目源码、项目配置、插件源码/配置文件，以及必要的插件运行库。

以下大型 Unreal 数据和本地生成文件不会纳入版本控制：

- `Content/`
- 插件内的 `Content/` 目录
- `Binaries/`、`Intermediate/`、`DerivedDataCache/`、`Saved/`
- `.uasset`、`.umap`、`.ply`、`.splat`、`.sog`
- `.apk` 等打包产物

如果项目运行需要场景、Gaussian Splatting 数据集、地图或其他 Unreal 资产，请在本地单独恢复，或通过独立的资产分发方式共享。

## 环境要求

- Unreal Engine 5.5
- Visual Studio 2022，并安装 Unreal Engine C++ 开发所需组件
- Git LFS
- 用于 VR 测试的 PICO/OpenXR 运行环境和设备配置
- 如果需要打包到 PICO 头显，需要在 Unreal Engine 中配置 Android 工具链

## 快速开始

克隆仓库：

```powershell
git clone https://github.com/Alto-R/PICO-UE5-3DGS.git
cd PICO-UE5-3DGS
git lfs install
```

使用 Unreal Engine 5.5 打开 `PicoGS.uproject`。如果 Unreal 提示重新编译项目模块，允许它编译即可。

如果需要进行 C++ 开发，可以通过 `.uproject` 右键菜单或 Unreal Editor 生成 IDE 工程文件，然后用 Visual Studio 打开生成的解决方案。

## 插件说明

项目当前包含或启用了以下主要插件：

- `PICOOpenXR`
- `OpenXR`
- `OnlineSubsystemPICO`
- `PICOEnterprise`
- `PICOSpatialAudio`
- `PICOXR`
- `VRExpansionPlugin`
- `XV3dGS`
- `Niagara`
- `Text3D`

`VRExpansionPlugin` 现在作为普通第三方插件目录提交到本仓库，而不是 Git submodule。它原本的许可证和 README 保留在 `Plugins/VRExpansionPlugin/` 下。

## 可用运行命令

项目中定义了一个控制台命令：

```text
CaptureMeshData.MeshData
```

该命令会从当前运行世界中采集静态网格 Actor 信息，并导出部分顶点坐标。正式使用前请检查 `Source/PicoGS/Private/WriteLogTest/WriteLogTest.cpp`，因为当前导出路径包含本机固定路径。

## 版本控制说明

本仓库的目标是保存可复现的项目代码和配置，同时避免提交大型 Unreal 内容资产和生成文件。提交前建议检查：

```powershell
git status
```

如果出现新的 UE 资产或打包数据，请确认它们应被忽略，或通过代码仓库之外的方式分发。

## 许可证

项目自身许可证尚未最终确定。第三方插件和库保留其各自目录中的原始许可证条款。
