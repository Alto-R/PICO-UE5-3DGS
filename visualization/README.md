# Eye-gaze 3D visualization

Overlays recorded eye-gaze (`Saved/EyeLogs/*.csv`) onto the Gaussian-Splatting
scene and reconstructs 3D gaze points by ray-casting each gaze ray against the
scene point cloud.

## Environment

```bash
conda create -n gaze3d python=3.11 numpy pandas scipy matplotlib
conda run -n gaze3d python -m pip install open3d plyfile trimesh plotly
```

## Run

```bash
conda run -n gaze3d python visualize_gaze.py --cast
```

Outputs (in this folder):
- `gaze_3d.html` — interactive 3D (scene point cloud + head path + gaze rays +
  red 3D gaze hit points). Regenerable, git-ignored.
- `gaze_3d_hits.csv` — reconstructed 3D gaze points in UE world metres.
- `final_preview.py` → `final_preview.png` — top-down + dwell heatmap.

## Coordinate alignment (baked into script defaults)

The eye-log CSV is in UE world metres. The GS PLY is brought into UE world by
`p_ue = conv(p_ply) + actor_loc`:
- `--gs-conv colmap` : UE = (z, x, -y)
- `--actor-loc` : the `splat_50000` GS actor Transform Location (cm) from the
  UE level (L_GSTest). Rotation 0, Scale 1.

Change `--ply` / `--actor-loc` / `--gs-conv` for a different scene.

Per-frame gaze ray: origin = `vr_X/Y/Z`, direction from `vr_Pitch/vr_Yaw`.
Only `eye_status==3` frames are used (blinks / tracking-loss filtered).
The `gaze_point_*` CSV columns are the eye origin (~0), not scene hit points.
