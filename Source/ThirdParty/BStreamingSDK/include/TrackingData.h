// PICO Business Streaming SDK v1.x tracking data structures.
// Transcribed from the official enterprise docs
// (business.picoxr.com/cn/doc -> 企业软件SDK使用说明 -> 企业串流 -> v1.x).
// Field order matches the documented struct layout (all int32/float, 4-byte).
#pragma once

#include <cstdint>

typedef struct PxrEyeTrackingData_
{
    int32_t leftEyePoseStatus;         // left eye pose data status
    int32_t rightEyePoseStatus;        // right eye pose data status
    int32_t combinedEyePoseStatus;     // combined pose data status
    float   leftEyeGazePoint[3];       // left eye gaze origin
    float   rightEyeGazePoint[3];      // right eye gaze origin
    float   combinedEyeGazePoint[3];   // combined gaze origin
    float   leftEyeGazeVector[3];      // left eye gaze direction
    float   rightEyeGazeVector[3];     // right eye gaze direction
    float   combinedEyeGazeVector[3];  // combined gaze direction
    float   leftEyeOpenness;           // 0.0 closed .. 1.0 fully open
    float   rightEyeOpenness;
    float   leftEyePupilDilation;      // millimetres
    float   rightEyePupilDilation;     // millimetres
    float   leftEyePositionGuide[3];   // metres, combined coordinate frame
    float   rightEyePositionGuide[3];
    float   foveatedGazeDirection[3];  // metres, combined coordinate frame
    int32_t foveatedGazeTrackingState;
} PxrEyeTrackingData;
