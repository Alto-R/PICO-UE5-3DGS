# BStreamingSDK (PICO Business Streaming SDK, v1.x)

PC-side SDK used to pull eye-tracking data over the PICO Business Streaming link
(`BStreamingSDK_GetEyeTrackingData`). Consumed by `UEyeGazeLoggerComponent`.

**Proprietary** — obtained from the PICO enterprise portal (business.picoxr.com,
"企业串流 v1.x SDK"). Eye tracking is only available in the **v1.x** SDK; v2.1/2.2
removed it. Requires the v1.x Business Streaming PC app + headset app running,
with eye tracking enabled on both ends, streaming via SteamVR.

```
include/BStreamingSDK.h    interface (Init/Deinit/GetEyeTrackingData/...)
include/TrackingData.h     PxrEyeTrackingData struct (transcribed from docs)
lib/Win64/BStreamingSDK.*  import lib + runtime dll (Win64)  <-- NOT in git
BStreamingSDK.Build.cs     UE external module (delay-loaded dll)
```

The `lib/Win64/` binaries (`BStreamingSDK.lib`, `BStreamingSDK.dll`) are the
PICO-proprietary v1.x SDK and are **git-ignored**. Download the v1.x Business
Streaming SDK and drop `BStreamingSDK.lib` + `BStreamingSDK.dll` into
`lib/Win64/` before building.
