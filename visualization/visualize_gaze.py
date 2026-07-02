"""
3D visualization of recorded eye-gaze over the Gaussian-Splatting scene,
done IN UE WORLD METRES (the frame the eye-log CSV already lives in).

The GS PLY is in its own reconstruction frame; XV3dGS placed it into the UE
level via a GS actor Transform (Location/Rotation/Scale). We bring the PLY into
UE world by:  p_ue = ActorRot * (ActorScale * conv(p_ply)) + ActorLoc
where conv() maps the PLY axis convention to UE local axes (default 'up_swap':
UE=(x, z, -y), i.e. PLY -Y is up). The CSV head pose + gaze ray are used as-is.

Get the GS actor Transform from the UE editor (select the Gaussian Splat actor
-> Details -> Transform) and pass it in:
    --actor-loc X Y Z      (UE Location, centimetres)
    --actor-rot P Y R      (UE Rotation, degrees: Pitch Yaw Roll)
    --actor-scale S        (uniform scale; or --actor-scale3 SX SY SZ)

Run inside the conda env:
    conda run -n gaze3d python visualize_gaze.py --cast [options]
"""
import argparse, glob, os
import numpy as np
import pandas as pd
from plyfile import PlyData

C0 = 0.28209479177387814
HERE = os.path.dirname(os.path.abspath(__file__))


def gs_conv(preset, p):
    """Map PLY-native xyz -> UE local axes (metres)."""
    x, y, z = p[:, 0], p[:, 1], p[:, 2]
    if preset == "identity":
        return p.copy()
    if preset == "up_swap":            # PLY -Y is up  ->  UE (x, z, -y)
        return np.stack([x, z, -y], 1)
    if preset == "up_swap_negx":
        return np.stack([-x, z, -y], 1)
    if preset == "colmap":             # PLY -Y up, 90deg yaw -> UE (z, x, -y)
        return np.stack([z, x, -y], 1)
    if preset == "colmap2":
        return np.stack([-z, x, -y], 1)
    raise ValueError(preset)


def ue_rot_matrix(pitch, yaw, roll):
    """UE FRotator (deg) -> 3x3 rotation (UE left-handed: X fwd,Y right,Z up)."""
    p, y, r = np.deg2rad([pitch, yaw, roll])
    cp, sp = np.cos(p), np.sin(p); cy, sy = np.cos(y), np.sin(y); cr, sr = np.cos(r), np.sin(r)
    Rz = np.array([[cy, -sy, 0], [sy, cy, 0], [0, 0, 1]])      # yaw about Z
    Ry = np.array([[cp, 0, sp], [0, 1, 0], [-sp, 0, cp]])      # pitch about Y
    Rx = np.array([[1, 0, 0], [0, cr, -sr], [0, sr, cr]])      # roll about X
    return Rz @ Ry @ Rx


def load_ply(path, max_points, opacity_thresh):
    v = PlyData.read(path)["vertex"].data
    xyz = np.stack([v["x"], v["y"], v["z"]], 1).astype(np.float32)
    rgb = np.clip(0.5 + C0 * np.stack([v["f_dc_0"], v["f_dc_1"], v["f_dc_2"]], 1), 0, 1)
    if "opacity" in v.dtype.names:
        a = 1 / (1 + np.exp(-v["opacity"].astype(np.float32)))
        keep = a > opacity_thresh
        xyz, rgb = xyz[keep], rgb[keep]
    if max_points and len(xyz) > max_points:
        idx = np.random.default_rng(0).choice(len(xyz), max_points, replace=False)
        xyz, rgb = xyz[idx], rgb[idx]
    return xyz, rgb


def load_gaze(csv_glob):
    files = [f for f in sorted(glob.glob(csv_glob), key=os.path.getmtime) if os.path.getsize(f) > 200]
    if not files:
        raise SystemExit(f"no usable CSV matched {csv_glob}")
    df = pd.concat([pd.read_csv(f) for f in files], ignore_index=True)
    df = df[df["eye_status"] == 3].reset_index(drop=True)
    org = df[["vr_X", "vr_Y", "vr_Z"]].to_numpy(float)             # UE metres
    p, y = np.deg2rad(df["vr_Pitch"].to_numpy(float)), np.deg2rad(df["vr_Yaw"].to_numpy(float))
    d = np.stack([np.cos(p) * np.cos(y), np.cos(p) * np.sin(y), np.sin(p)], 1)
    return files, org, d


def march_hits(org, d, cloud, tmax, tstep, hit_radius):
    from scipy.spatial import cKDTree
    tree = cKDTree(cloud)
    ts = np.arange(0.2, tmax, tstep)
    hits = np.full((len(org), 3), np.nan)
    for i in range(len(org)):
        pts = org[i] + np.outer(ts, d[i])
        dist, _ = tree.query(pts, k=1)
        j = int(np.argmax(dist < hit_radius))
        if dist[j] < hit_radius:
            hits[i] = pts[j]
    return hits


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--ply", default=r"D:\1-PKU\PKU\1 Master\Projects\3dgauss\splat_50000.ply")
    ap.add_argument("--csv", default=os.path.join(HERE, "..", "Saved", "EyeLogs", "*.csv"))
    ap.add_argument("--out", default=os.path.join(HERE, "gaze_3d.html"))
    ap.add_argument("--gs-conv", default="colmap", choices=["identity", "up_swap", "up_swap_negx", "colmap", "colmap2"])
    # GS actor Transform from the UE editor (splat_50000 actor, L_GSTest):
    ap.add_argument("--actor-loc", nargs=3, type=float, default=[-2630.020437, 1010.975633, 130.047728], help="UE Location (cm)")
    ap.add_argument("--actor-rot", nargs=3, type=float, default=[0, 0, 0], help="UE Rotation Pitch Yaw Roll (deg)")
    ap.add_argument("--actor-scale", type=float, default=1.0)
    ap.add_argument("--actor-scale3", nargs=3, type=float, default=None)
    ap.add_argument("--max-points", type=int, default=80000)
    ap.add_argument("--opacity", type=float, default=0.3)
    ap.add_argument("--ray-len", type=float, default=3.0)
    ap.add_argument("--ray-stride", type=int, default=3)
    ap.add_argument("--cast", action="store_true")
    ap.add_argument("--hit-radius", type=float, default=0.4)
    args = ap.parse_args()

    print("Loading PLY ...")
    ply_native, rgb = load_ply(args.ply, args.max_points, args.opacity)

    # PLY-native -> UE world metres
    local = gs_conv(args.gs_conv, ply_native)
    scale = np.array(args.actor_scale3) if args.actor_scale3 else np.array([args.actor_scale] * 3)
    R = ue_rot_matrix(*args.actor_rot)
    loc_m = np.array(args.actor_loc) / 100.0
    cloud = (R @ (local * scale).T).T + loc_m
    print(f"  cloud points: {len(cloud)}")

    print("Loading gaze CSV ...")
    files, org, d = load_gaze(args.csv)
    print(f"  files: {[os.path.basename(f) for f in files]}   valid frames: {len(org)}")

    bb = lambda a: {k: (round(float(a[:, i].min()), 2), round(float(a[:, i].max()), 2)) for i, k in enumerate("xyz")}
    print("cloud bbox (UE m):", bb(cloud))
    print("head  bbox (UE m):", bb(org))

    import plotly.graph_objects as go
    fig = go.Figure()
    col = ["rgb(%d,%d,%d)" % tuple((c * 255).astype(int)) for c in rgb]
    fig.add_trace(go.Scatter3d(x=cloud[:, 0], y=cloud[:, 1], z=cloud[:, 2], mode="markers",
                               marker=dict(size=1.2, color=col, opacity=0.55), name="GS scene"))
    fig.add_trace(go.Scatter3d(x=org[:, 0], y=org[:, 1], z=org[:, 2], mode="lines+markers",
                               line=dict(color="deepskyblue", width=3),
                               marker=dict(size=2, color="deepskyblue"), name="head path"))
    xs, ys, zs = [], [], []
    for i in range(0, len(org), max(1, args.ray_stride)):
        a = org[i]; b = org[i] + d[i] * args.ray_len
        xs += [a[0], b[0], None]; ys += [a[1], b[1], None]; zs += [a[2], b[2], None]
    fig.add_trace(go.Scatter3d(x=xs, y=ys, z=zs, mode="lines",
                               line=dict(color="orange", width=2), name="gaze rays", opacity=0.6))
    if args.cast:
        print("Ray-marching gaze hits ...")
        hits = march_hits(org, d, cloud, args.ray_len * 8, 0.1, args.hit_radius)
        m = ~np.isnan(hits[:, 0]); print(f"  hits: {int(m.sum())}/{len(hits)}")
        fig.add_trace(go.Scatter3d(x=hits[m, 0], y=hits[m, 1], z=hits[m, 2], mode="markers",
                                   marker=dict(size=4, color="red"), name="gaze hits"))
        # export 3D gaze points (UE world metres) for downstream analysis
        out_csv = os.path.splitext(args.out)[0] + "_hits.csv"
        pd.DataFrame(hits[m], columns=["gaze3d_x", "gaze3d_y", "gaze3d_z"]).to_csv(out_csv, index=False)
        print("Wrote", out_csv)
    fig.update_layout(scene=dict(aspectmode="data"),
                      title=f"Eye-gaze over GS scene (UE world m, {len(org)} frames)", showlegend=True)
    fig.write_html(args.out, include_plotlyjs="cdn")
    print("Wrote", args.out)


if __name__ == "__main__":
    main()
