# PicoGS

PicoGS is an Unreal Engine 5.5 project for viewing Gaussian Splatting content in a PICO/OpenXR VR workflow. The project combines PICO XR integration, OpenXR, VR interaction support, and the XVerse 3D Gaussian Splatting plugin.

## Features

- Unreal Engine 5.5 C++ project.
- VR-first configuration with `bStartInVR=True`.
- OpenXR-based runtime path through `PICOOpenXR` and `OpenXR`.
- PICO platform support through `OnlineSubsystemPICO` and related PICO plugins.
- Gaussian Splatting support through the `XV3dGS` plugin.
- VR interaction support through `VRExpansionPlugin`, vendored as normal project files.
- Utility C++ code for logging, HTTP result upload, and static mesh vertex export.

## Repository Contents

This repository tracks source code, project configuration, plugin source/config files, and required plugin runtime libraries.

Large Unreal-generated data and local build products are intentionally not tracked:

- `Content/`
- plugin `Content/` folders
- `Binaries/`, `Intermediate/`, `DerivedDataCache/`, `Saved/`
- `.uasset`, `.umap`, `.ply`, `.splat`, `.sog`
- packaged artifacts such as `.apk`

If a scene, Gaussian Splatting dataset, map, or other Unreal asset is needed, restore it locally outside Git or share it through a separate asset delivery channel.

## Requirements

- Unreal Engine 5.5
- Visual Studio 2022 with C++ support for Unreal Engine development
- Git LFS
- PICO/OpenXR runtime and device setup for VR testing
- Android toolchain configured in Unreal Engine if packaging for PICO headsets

## Getting Started

Clone the repository:

```powershell
git clone https://github.com/Alto-R/PICO-UE5-3DGS.git
cd PICO-UE5-3DGS
git lfs install
```

Open `PicoGS.uproject` with Unreal Engine 5.5. If Unreal asks to rebuild project modules, allow it to compile.

For C++ development, generate IDE project files from the `.uproject` context menu or from Unreal Editor, then open the generated solution in Visual Studio.

## Plugins

The project currently includes or references these main plugins:

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

`VRExpansionPlugin` is committed as a regular vendored plugin directory, not as a Git submodule. Its original license and README are kept under `Plugins/VRExpansionPlugin/`.

## Useful Runtime Command

The project defines a console command:

```text
CaptureMeshData.MeshData
```

It captures static mesh actor information and a sample of vertex positions from the current play world. Check `Source/PicoGS/Private/WriteLogTest/WriteLogTest.cpp` before using it in production, because the current output path is local-machine specific.

## Version Control Notes

This repository is intended to keep the project reproducible without committing large Unreal content and generated files. Before committing, check:

```powershell
git status
```

If new UE assets or packaged data appear, make sure they are intentionally ignored or distributed outside the code repository.

## License

Project-specific licensing has not been finalized. Third-party plugins and libraries keep their own license terms in their respective plugin folders.
