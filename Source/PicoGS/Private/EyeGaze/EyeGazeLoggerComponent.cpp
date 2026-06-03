#include "EyeGaze/EyeGazeLoggerComponent.h"

#include "Engine/Engine.h"
#include "Engine/World.h"
#include "EyeTrackerTypes.h"
#include "GameFramework/WorldSettings.h"
#include "HAL/FileManager.h"
#include "IXRTrackingSystem.h"
#include "Misc/DateTime.h"
#include "Misc/Paths.h"
#include "PICO_MovementFunctionLibrary.h"
#include "Serialization/Archive.h"

DEFINE_LOG_CATEGORY_STATIC(LogEyeGaze, Log, All);

UEyeGazeLoggerComponent::UEyeGazeLoggerComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = false;
}

void UEyeGazeLoggerComponent::BeginPlay()
{
	Super::BeginPlay();

	if (!bAutoStartOnBeginPlay)
	{
		return;
	}

	bool bSupported = false;
	UMovementFunctionLibraryPICO::IsEyeTrackerSupportedPICO(bSupported);
	if (!bSupported)
	{
		UE_LOG(LogEyeGaze, Warning, TEXT("Eye tracker not supported on this device; logger inactive."));
		return;
	}

	if (!UMovementFunctionLibraryPICO::StartEyeTrackingPICO())
	{
		UE_LOG(LogEyeGaze, Warning, TEXT("StartEyeTrackingPICO failed; logger inactive."));
		return;
	}
	bDeviceReady = true;

	StartLogging();
}

void UEyeGazeLoggerComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (bLogging)
	{
		StopLogging();
	}
	if (bDeviceReady)
	{
		UMovementFunctionLibraryPICO::StopEyeTrackingPICO();
		bDeviceReady = false;
	}
	Super::EndPlay(EndPlayReason);
}

bool UEyeGazeLoggerComponent::StartLogging()
{
	const FString SessionID = FDateTime::Now().ToString(TEXT("%Y%m%d_%H%M%S"));
	const FString Dir = FPaths::ProjectSavedDir() / LogSubdir;
	IFileManager::Get().MakeDirectory(*Dir, /*Tree=*/true);
	CsvPath = Dir / (SessionID + TEXT(".csv"));

	CsvFile.Reset(IFileManager::Get().CreateFileWriter(*CsvPath, FILEWRITE_AllowRead));
	if (!CsvFile.IsValid())
	{
		UE_LOG(LogEyeGaze, Error, TEXT("Failed to open CSV for write: %s"), *CsvPath);
		return false;
	}

	const FString Header = TEXT("index,Time,vr_X,vr_Y,vr_Z,vr_Pitch,vr_Yaw\n");
	const FTCHARToUTF8 HeaderUtf8(*Header);
	CsvFile->Serialize(const_cast<ANSICHAR*>(HeaderUtf8.Get()), HeaderUtf8.Length());

	SamplesWritten = 0;
	RowBuffer.Reset();
	bLogging = true;
	SetComponentTickEnabled(true);

	UE_LOG(LogEyeGaze, Log, TEXT("EyeLog START -> %s"), *CsvPath);
	return true;
}

void UEyeGazeLoggerComponent::StopLogging()
{
	SetComponentTickEnabled(false);
	FlushBuffer();
	if (CsvFile.IsValid())
	{
		CsvFile->Close();
		CsvFile.Reset();
	}
	bLogging = false;
	UE_LOG(LogEyeGaze, Log, TEXT("EyeLog STOP, %lld samples -> %s"), SamplesWritten, *CsvPath);
}

void UEyeGazeLoggerComponent::FlushBuffer()
{
	if (RowBuffer.Num() == 0 || !CsvFile.IsValid())
	{
		return;
	}
	const FString Block = FString::Join(RowBuffer, TEXT(""));
	const FTCHARToUTF8 Utf8(*Block);
	CsvFile->Serialize(const_cast<ANSICHAR*>(Utf8.Get()), Utf8.Length());
	RowBuffer.Reset();
}

void UEyeGazeLoggerComponent::TickComponent(float DeltaTime, ELevelTick TickType,
                                            FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (!bLogging || !CsvFile.IsValid())
	{
		return;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	FEyeDataPICO Left, Right;
	FEyeTrackerGazeData Gaze;
	const float W2M = World->GetWorldSettings() ? World->GetWorldSettings()->WorldToMeters : 100.f;
	if (!UMovementFunctionLibraryPICO::GetEyeTrackingDataPICO(Left, Right, /*QueryGazeData=*/true, Gaze, W2M))
	{
		return;
	}

	FVector HmdPos = FVector::ZeroVector;
	if (GEngine && GEngine->XRSystem.IsValid())
	{
		FQuat Q = FQuat::Identity;
		FVector P = FVector::ZeroVector;
		if (GEngine->XRSystem->GetCurrentPose(IXRTrackingSystem::HMDDeviceId, Q, P))
		{
			const FTransform T2W = GEngine->XRSystem->GetTrackingToWorldTransform();
			HmdPos = T2W.TransformPosition(P);
		}
	}

	const FRotator GazeRot = Gaze.GazeDirection.GetSafeNormal().Rotation();

	const double VrX = HmdPos.X / 100.0;
	const double VrY = HmdPos.Y / 100.0;
	const double VrZ = HmdPos.Z / 100.0;

	const FString TimeStr = FDateTime::UtcNow().ToIso8601();

	FString Row = FString::Printf(
		TEXT("%lld,%s,%.6f,%.6f,%.6f,%.4f,%.4f\n"),
		SamplesWritten, *TimeStr,
		VrX, VrY, VrZ,
		GazeRot.Pitch, GazeRot.Yaw);

	RowBuffer.Add(MoveTemp(Row));
	++SamplesWritten;

	if (RowBuffer.Num() >= FlushThresholdRows)
	{
		FlushBuffer();
	}
}
