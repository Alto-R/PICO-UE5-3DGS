// PICO Business Streaming SDK v1.x public interface (subset we consume).
// Matches the exported symbols of the v1.x BStreamingSDK.dll
// (unreal/businessstreaming/SDK/lib) and the official enterprise docs.
// Eye tracking is only available in the v1.x SDK; v2.1/v2.2 dropped it.
#pragma once

#include "TrackingData.h"

#ifdef BSTREAMINGSDK_EXPORTS
#define BSTREAMINGSDK_API __declspec(dllexport)
#else
#define BSTREAMINGSDK_API __declspec(dllimport)
#endif

extern "C" {

    enum BStreamingSDKError
    {
        BStreamingSDKError_None = 0,
        BStreamingSDKError_SDKNotInit = -1,
        BStreamingSDKError_UnknownMsg = -2,
        BStreamingSDKError_BadRecvSocket = -3,
        BStreamingSDKError_MemoryOverflow = -4,
        BStreamingSDKError_ActionMismatch = -5,
        BStreamingSDKError_HMDNotConnected = -6,
    };

    // Establishes communication with the Business Streaming host process.
    // userData is a reserved field; pass nullptr.
    BSTREAMINGSDK_API int BStreamingSDK_Init(void* userData);
    BSTREAMINGSDK_API int BStreamingSDK_Deinit();

    // Direct poll of the latest combined/per-eye gaze data. Returns 0 on success.
    // Requires the eye-tracking switch enabled on BOTH the PC and the headset
    // (PICO 4E), plus an active streaming session.
    BSTREAMINGSDK_API int BStreamingSDK_GetEyeTrackingData(PxrEyeTrackingData& etData);

    // Legacy control/query helpers (kept for completeness; unused here).
    BSTREAMINGSDK_API int BStreamingSDK_ControlMsg(const char* msg);
    BSTREAMINGSDK_API int BStreamingSDK_QueryMsg(const char* msg, char* res, unsigned len);

}
