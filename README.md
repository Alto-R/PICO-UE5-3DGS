<div align="center">

# PicoGS 👁️‍🗨️

View **3D Gaussian Splatting** scenes in VR on a **PICO 4 Enterprise**, log
per-frame **eye-gaze + head-pose** through the PICO Business Streaming SDK, and
reconstruct **3D gaze points** over the scene with an offline Python pipeline.

[![Unreal](https://img.shields.io/badge/Unreal%20Engine-5.5-0E1128?style=flat-square&logo=unrealengine&logoColor=white)](#-requirements)
[![PICO](https://img.shields.io/badge/PICO-4%20Enterprise-4200FF?style=flat-square)](#-requirements)
[![3DGS](https://img.shields.io/badge/Gaussian%20Splatting-XV3dGS-00A86B?style=flat-square)](#-why-this-stack)
[![SteamVR](https://img.shields.io/badge/Runtime-OpenXR%20%2F%20SteamVR-1b2838?style=flat-square&logo=steam&logoColor=white)](#-why-this-stack)
[![Python](https://img.shields.io/badge/Python-3.11-3776AB?style=flat-square&logo=python&logoColor=white)](#-3d-gaze-visualization)

**English** | **[中文](README.zh-CN.md)**

</div>

<br>

## 📑 Contents

<div align="center">

| | | |
|:---:|:---:|:---:|
| [✨ Features](#-features) | [🏗️ Architecture](#️-architecture) | [🧭 Why this stack](#-why-this-stack) |
| [🚀 Quick start](#-quick-start) | [👁️ Eye-gaze logging](#️-eye-gaze-logging) | [🎯 3D gaze visualization](#-3d-gaze-visualization) |
| [⚙️ Key files](#️-key-files) | [❓ FAQ](#-faq) | [🗂️ Repository layout](#️-repository-layout) |

</div>

<br>

## ✨ Features

- **3DGS in VR** — render Gaussian-Splatting scenes with the `XV3dGS` plugin and
  stream them to a PICO 4 Enterprise over PICO Business Streaming.
- **Eye-gaze + head-pose logger** — `UEyeGazeLoggerComponent` writes one CSV row
  per frame (position, gaze pitch/yaw, per-eye openness / pupil / gaze point).
- **PC-side eye tracking** — data comes from the **Business Streaming SDK
  (v1.x)** `GetEyeTrackingData` over the streaming link, not from the device.
- **Runtime-agnostic** — the app runs on generic **OpenXR / SteamVR**
  (PICO-specific plugins disabled), with a VRExpansion pawn + Enhanced Input.
- **3D gaze reconstruction** — a conda-based Python tool ray-casts each gaze ray
  against the scene point cloud and produces an interactive Plotly overlay plus
  a CSV of 3D gaze points.
- **Pipeline-compatible CSV** — the schema matches the offline
  [`ni1o1/vr-gaze-pipeline`](https://github.com/ni1o1/vr-gaze-pipeline).

## 🏗️ Architecture

```text
PICO 4 Enterprise  ──stream──►  Business Streaming (v1.x, SteamVR)  ◄──►  SteamVR
        ▲  eye/head                       │                                   ▲
        │                                 │ BStreamingSDK_GetEyeTrackingData    │ OpenXR
        ▼  display                         ▼                                   │ render
   Unreal Engine 5.5  ──────────────────────────────────────────────────────────┘
        │  UEyeGazeLoggerComponent  (per-frame head pose + gaze)
        ▼
   Saved/EyeLogs/<session>.csv
        │
        ▼  visualization/visualize_gaze.py  (align to GS PLY, ray-cast)
   gaze_3d.html (interactive)  +  gaze_3d_hits.csv (3D gaze points)
```

## 🧭 Why this stack

Three hard constraints shape the whole design:

1. **`XV3dGS` is Win64-only** → the app cannot be packaged as a native Android
   APK; it runs on the **PC** and **streams** to the headset.
2. **Eye tracking exists only in the Business Streaming *v1.x* SDK** — v2.1/v2.2
   removed it. So the PC pulls gaze via the v1.x `GetEyeTrackingData`.
3. **v1.x streaming is SteamVR-only** → the project renders through **generic
   OpenXR / SteamVR** (the `PICOOpenXR` plugin is disabled), and controllers are
   bound to the **Valve Index** OpenXR interaction profile.

## 🚀 Quick start

### Prerequisites

- **Unreal Engine 5.5** + Visual Studio 2022 (C++), Git LFS.
- **PICO 4 Enterprise** headset with eye tracking, on a compatible PUI.
- **PICO Business Streaming v1.2.x** installed on *both* the PC and the headset
  (v1.x — eye tracking was dropped in v2.x), plus **SteamVR**.
- The proprietary **BStreamingSDK** `.dll` / `.lib` dropped into
  `Source/ThirdParty/BStreamingSDK/lib/Win64/` (see that folder's README).
- Python via conda for the visualization (see below).

### Build

```powershell
git clone https://github.com/Alto-R/PICO-UE5-3DGS.git
cd PICO-UE5-3DGS
git lfs install
```

Right-click `PicoGS.uproject` → *Generate Visual Studio project files*, then
build **`PicoGSEditor` (Win64, Development)** or open the `.uproject` and let it
compile.

### Run & record eye data

1. Start **SteamVR**, then **Business Streaming (v1.x)** on the PC; connect the
   headset and enter the SteamVR stream (headset shows the SteamVR room).
2. Enable eye tracking on **both** ends: PC app → *Settings → General →
   "串流时支持眼动功能"*, and the **headset** eye tracking (run calibration once).
3. Open the project and play with **VR Preview**. Look around the scene.
4. Each session writes `Saved/EyeLogs/<yyyymmdd_hhmmss>.csv` — the
   `Eye-tracking backend active: BStreaming` log line confirms it is recording.

### Visualize

```bash
conda create -n gaze3d python=3.11 numpy pandas scipy matplotlib
conda run -n gaze3d python -m pip install open3d plyfile trimesh plotly
conda run -n gaze3d python visualization/visualize_gaze.py --cast
```

Open `visualization/gaze_3d.html` in a browser and rotate; the 3D gaze points
are also exported to `visualization/gaze_3d_hits.csv`.

## 👁️ Eye-gaze logging

`UEyeGazeLoggerComponent` (attached to `APicoVRCharacter`) selects a backend on
`BeginPlay`:

- **BStreaming** *(primary)* — loads `BStreamingSDK.dll`, calls
  `BStreamingSDK_Init`, and each tick reads `BStreamingSDK_GetEyeTrackingData`
  (`combined_eye_gaze_vector` → world-space pitch/yaw via the HMD pose).
- **OpenXR** *(fallback)* — `XR_EXT_eye_gaze_interaction` for native runtimes.

CSV schema (metres, degrees):

```text
index,Time,vr_X,vr_Y,vr_Z,vr_Pitch,vr_Yaw
  [,left_openness,right_openness,left_pupil,right_pupil,gaze_point_x,gaze_point_y,gaze_point_z,eye_status]
```

- Filter to `eye_status == 3` (valid frames; others are blinks / tracking loss).
- `gaze_point_*` is the **eye origin (~0)**, *not* a scene hit point.
- The dll is delay-loaded, so a missing SDK / stream degrades to the fallback
  instead of crashing.

## 🎯 3D gaze visualization

`visualization/visualize_gaze.py` reconstructs 3D gaze points:

- **Ray per frame**: origin = head position, direction from `vr_Pitch/vr_Yaw`.
- **3D gaze point** = ray × scene point cloud (KD-tree ray-march).
- **Alignment** (PLY → UE world, baked as defaults for `splat_50000`):
  `p_ue = conv(p_ply) + actor_loc`, with `--gs-conv colmap` (`UE = (z, x, -y)`)
  and the GS actor `--actor-loc` from the UE level. Override `--ply` /
  `--actor-loc` / `--gs-conv` for a different scene.

Outputs: interactive `gaze_3d.html`, `gaze_3d_hits.csv`, and a top-down + dwell
heatmap via `final_preview.py`. See [`visualization/README.md`](visualization/README.md).

## ⚙️ Key files

| Path | Purpose |
| --- | --- |
| `Source/PicoGS/Private/EyeGaze/EyeGazeLoggerComponent.cpp` | Eye-gaze + head-pose CSV logger (BStreaming / OpenXR backends) |
| `Source/PicoGS/Private/PicoVRCharacter/` | VRExpansion pawn + Enhanced Input |
| `Source/ThirdParty/BStreamingSDK/` | UE external module for the PICO v1.x SDK (headers + Build.cs; binaries git-ignored) |
| `Content/PICO/BP_PicoVRCharacter`, `.../Input/IMC_VR` | VR pawn blueprint + input mappings (Valve Index profile) |
| `Content/Maps/L_GSTest` | Test level with the Gaussian-Splatting actor |
| `Config/DefaultEngine.ini` | Eye-tracking + rendering settings |
| `visualization/visualize_gaze.py` | 3D gaze reconstruction + Plotly overlay |

## ❓ FAQ

**Eye CSV is empty / `GetEyeTrackingData` returns non-zero.**
Use **v1.x** Business Streaming (v2.x has no eye SDK), enable eye tracking on the
PC app **and** the headset, calibrate on the headset, and keep a live stream.

**Headset shows the SteamVR room but the scene only appears on the PC.**
Actually *enter the SteamVR stream* on the headset (not just connect), and play
with **VR Preview** so UE submits stereo to SteamVR.

**Controllers do nothing.**
Off `PICOOpenXR`, the `PICOTouch_*` keys are invalid. Rebind `IMC_VR` (IA_Move /
IA_Turn) to the **Valve Index** MotionController thumbstick keys.

**`pico_et_ft_bt_bridge.exe` fails with `0xc000012d`.**
Commit-limit exceeded — increase the Windows page file and close memory-heavy
apps; 16 GB is tight for UE + SteamVR + Business Streaming together.

**A build stalls at ~0 % CPU with no compilers spawned.**
Unreal Build Accelerator's memory gate. Set `bAllowUBALocalExecutor=false` in
`BuildConfiguration.xml` or free RAM / grow the page file.

## 🗂️ Repository layout

Tracked: C++ source, project & plugin config, vendored plugin source/runtime
libs, and **project-specific** `Content` (Maps/L_GSTest, the PICO VR pawn +
`IMC_VR`, `BP_Code`, `assets`, …) via **Git LFS**.

Not tracked (git-ignored): `Binaries/`, `Intermediate/`, `DerivedDataCache/`,
`Saved/`, marketplace template content (FirstPerson / Fab / Mannequin), the large
`splat_*.uasset` and `*.ply` assets, the proprietary `BStreamingSDK` binaries,
generated `visualization/*.html`, and `*.apk`.

## 📚 Acknowledgements

- [Xverse `XV3dGS`](http://www.xverse.cn/) — Gaussian Splatting for Unreal.
- `VRExpansionPlugin` — VR interaction (vendored; see its folder for license).
- PICO OpenXR / Business Streaming SDK — device + streaming integration.
- [`ni1o1/vr-gaze-pipeline`](https://github.com/ni1o1/vr-gaze-pipeline) — offline
  3D semantic gaze analysis (CSV schema reference).

## License

Project licensing is not finalized. Third-party plugins and SDKs keep their own
license terms in their respective folders.
