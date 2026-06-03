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

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType,
	                           FActorComponentTickFunction* ThisTickFunction) override;

private:
	bool StartLogging();
	void StopLogging();
	void FlushBuffer();

	bool bDeviceReady = false;
	bool bLogging = false;
	int64 SamplesWritten = 0;

	FString CsvPath;
	TUniquePtr<FArchive> CsvFile;
	TArray<FString> RowBuffer;
};
