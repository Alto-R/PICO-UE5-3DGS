// Per-frame head-pose + gaze-angle logger for PICO/OpenXR. Writes one CSV row
// per tick between BeginPlay and EndPlay. Schema matches the input format of
// the offline pipeline at https://github.com/ni1o1/vr-gaze-pipeline so the CSV
// can be consumed directly. All scene-side analysis (multi-ray cone, fixation
// detection, semantic labelling) happens offline in Python.
//
// Design: docs/plans/2026-05-25-eye-gaze-logging-design.md

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "EyeGazeLoggerComponent.generated.h"

class FArchive;

UCLASS(ClassGroup = (VR), meta = (BlueprintSpawnableComponent))
class PICOGS_API UEyeGazeLoggerComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UEyeGazeLoggerComponent();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "EyeGaze")
	bool bAutoStartOnBeginPlay = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "EyeGaze|IO", meta = (ClampMin = "1"))
	int32 FlushThresholdRows = 60;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "EyeGaze|IO")
	FString LogSubdir = TEXT("EyeLogs");

	// When true (and the BStreaming backend is active), append per-eye openness,
	// pupil dilation, combined gaze point and status to each CSV row. The first
	// seven columns are unchanged so the offline pipeline stays compatible.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "EyeGaze|IO")
	bool bLogEyeExtensions = true;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType,
	                           FActorComponentTickFunction* ThisTickFunction) override;

private:
	// Optional per-eye extras filled by the BStreaming backend. NaN means "not
	// provided by this backend"; such fields are written as empty CSV cells.
	struct FEyeExtras
	{
		float LeftOpenness = NAN;
		float RightOpenness = NAN;
		float LeftPupil = NAN;
		float RightPupil = NAN;
		FVector GazePoint = FVector(NAN, NAN, NAN);
		int32 Status = -1;
	};

	bool StartLogging();
	void StopLogging();
	void FlushBuffer();

	// Logs the active XR runtime and which eye-tracking channels report support.
	void LogRuntimeDiagnostics();

	// Backend selection. BStreaming is the PC-side path (Business Streaming SDK
	// over the stream) and is preferred because the app always runs on PC and
	// streams to the headset. OpenXR is the generic fallback
	// (XR_EXT_eye_gaze_interaction) for native/other runtimes.
	bool TryStartBStreamingBackend();
	bool TryStartOpenXRBackend();
	bool GetGazeRotation(class UWorld* World, FRotator& OutGazeRot, FEyeExtras& OutExtras) const;

	// Query the latest BStreaming eye-tracking data into world-space gaze.
	bool QueryBStreamingGaze(class UWorld* World, FRotator& OutGazeRot, FEyeExtras& OutExtras) const;

	enum class EEyeBackend : uint8
	{
		None,
		BStreaming,
		OpenXR
	};
	EEyeBackend Backend = EEyeBackend::None;

	// Handle to the delay-loaded BStreamingSDK.dll (Win64 only).
	void* BStreamingDllHandle = nullptr;
	bool bBStreamingInitialized = false;

	bool bDeviceReady = false;
	bool bLogging = false;
	bool bDiagLoggedFirstSample = false;
	// Diagnostics for the BStreaming eye query (mutable: set inside const query).
	mutable bool bBStreamingProbeLogged = false;
	mutable int64 BStreamingFailCount = 0;
	int64 SamplesWritten = 0;

	FString CsvPath;
	TUniquePtr<FArchive> CsvFile;
	TArray<FString> RowBuffer;
};
