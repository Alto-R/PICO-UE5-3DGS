import numpy as np, pandas as pd, glob, os, matplotlib
matplotlib.use("Agg"); import matplotlib.pyplot as plt
from plyfile import PlyData
C0 = 0.28209479177387814
LOC = np.array([-2630.020437, 1010.975633, 130.047728]) / 100.0
v = PlyData.read(r"D:\1-PKU\PKU\1 Master\Projects\3dgauss\splat_50000.ply")["vertex"].data
xyz = np.stack([v["x"], v["y"], v["z"]], 1).astype(np.float32)
rgb = np.clip(0.5 + C0 * np.stack([v["f_dc_0"], v["f_dc_1"], v["f_dc_2"]], 1), 0, 1)
al = 1 / (1 + np.exp(-v["opacity"].astype(np.float32))); k = al > 0.3
xyz, rgb = xyz[k], rgb[k]
x, y, z = xyz[:, 0], xyz[:, 1], xyz[:, 2]
cloud = np.stack([z, x, -y], 1) + LOC            # colmap conv
i = np.random.default_rng(0).choice(len(cloud), min(60000, len(cloud)), False)
cloud, rgb = cloud[i], rgb[i]
h = pd.read_csv("gaze_3d_hits.csv").to_numpy()
csvs = [f for f in sorted(glob.glob(r"D:\1-PKU\PKU\1 Master\Projects\3dgauss\unreal\PicoGS\Saved\EyeLogs\*.csv"), key=os.path.getmtime) if os.path.getsize(f) > 200]
df = pd.concat([pd.read_csv(f) for f in csvs]); df = df[df.eye_status == 3]
head = df[["vr_X", "vr_Y", "vr_Z"]].to_numpy(float)
fig, ax = plt.subplots(1, 2, figsize=(20, 9))
ax[0].scatter(cloud[:, 0], cloud[:, 1], s=1, c=rgb, alpha=.45)
ax[0].plot(head[:, 0], head[:, 1], '-', c='deepskyblue', lw=1)
ax[0].scatter(h[:, 0], h[:, 1], s=5, c='red', alpha=.5, label="gaze hits")
ax[0].set_title("TOP-DOWN X-Y: scene + head path + gaze hits (colmap)")
ax[0].set_aspect('equal'); ax[0].legend()
ax[1].scatter(cloud[:, 0], cloud[:, 1], s=1, c='lightgray', alpha=.3)
hb = ax[1].hexbin(h[:, 0], h[:, 1], gridsize=60, cmap='inferno', mincnt=1, alpha=.9)
plt.colorbar(hb, ax=ax[1], label="gaze count")
ax[1].set_title("Gaze density heatmap (dwell)"); ax[1].set_aspect('equal')
plt.tight_layout(); plt.savefig("final_preview.png", dpi=80)
print("saved final_preview.png; hits:", len(h))
