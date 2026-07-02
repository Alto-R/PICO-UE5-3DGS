#include "EyeGaze/EyeGazeLoggerComponent.h"

#include "Engine/Engine.h"
#include "Engine/World.h"
#include "EyeTrackerFunctionLibrary.h"
#include "EyeTrackerTypes.h"
#include "IEyeTracker.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/WorldSettings.h"
#include "HAL/FileManager.h"
#include "IXRTrackingSystem.h"
#include "Misc/DateTime.h"
#include "Misc/Paths.h"
#include "Serialization/Archive.h"

#if PLATFORM_WINDOWS
#include "BStreamingSDK.h"
#include "HAL/PlatformProcess.h"
#endif

DEFINE_LOG_CATEGORY_STATIC(LogEyeGaze, Log, All);

#if PLATFORM_WINDOWS
namespace
{
	// OpenXR/PICO right-handed (metres, -Z forward) → UE left-handed direction.
	FORCEINLINE FVector OpenXRDirToUE(const FVector& V)
	{
		return FVector(-V.Z, V.X, V.Y);
	}
}
#endif // PLATFORM_WINDOWS

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

	LogRuntimeDiagnostics();

	// Prefer the PC-side Business Streaming SDK (the app always runs on PC and
	// streams to the headset, so this is the path that delivers gaze under
	// streaming). Fall back to the generic OpenXR eye tracker.
	if (TryStartBStreamingBackend())
	{
		Backend = EEyeBackend::BStreaming;
	}
	else if (TryStartOpenXRBackend())
	{
		Backend = EEyeBackend::OpenXR;
	}
	else
	{
		UE_LOG(LogEyeGaze, Warning, TEXT("No eye-tracking backend available; logger inactive. See [Diag] lines above for runtime/extension support."));
		return;
	}

	const TCHAR* BackendName =
		Backend == EEyeBackend::BStreaming ? TEXT("BStreaming (Business Streaming SDK)")
		: TEXT("generic OpenXR (XR_EXT_eye_gaze_interaction)");
	UE_LOG(LogEyeGaze, Log, TEXT("Eye-tracking backend active: %s"), BackendName);

	StartLogging();
}

void UEyeGazeLoggerComponent::LogRuntimeDiagnostics()
{
	if (GEngine && GEngine->XRSystem.IsValid())
	{
		UE_LOG(LogEyeGaze, Log, TEXT("[Diag] XR system='%s'  runtime='%s'"),
			*GEngine->XRSystem->GetSystemName().ToString(),
			*GEngine->XRSystem->GetVersionString());
	}
	else
	{
		UE_LOG(LogEyeGaze, Warning, TEXT("[Diag] No active XR tracking system (running flat / no HMD)."));
	}

	EEyeTrackerStatus GenericStatus = EEyeTrackerStatus::NotConnected;
	if (GEngine && GEngine->EyeTrackingDevice.IsValid())
	{
		GenericStatus = GEngine->EyeTrackingDevice->GetEyeTrackerStatus();
	}
	UE_LOG(LogEyeGaze, Log,
		TEXT("[Diag] Generic OpenXR eye tracker: connected=%s status=%d (0=NotConnected,1=NotTracking,2=Tracking)"),
		UEyeTrackerFunctionLibrary::IsEyeTrackerConnected() ? TEXT("YES") : TEXT("no"),
		(int32)GenericStatus);
}

bool UEyeGazeLoggerComponent::TryStartBStreamingBackend()
{
#if PLATFORM_WINDOWS
	// Delay-loaded DLL: load explicitly so a missing SDK degrades gracefully
	// instead of crashing on first import-stub call.
	if (!BStreamingDllHandle)
	{
		BStreamingDllHandle = FPlatformProcess::GetDllHandle(TEXT("BStreamingSDK.dll"));
	}
	if (!BStreamingDllHandle)
	{
		UE_LOG(LogEyeGaze, Warning, TEXT("[Diag] BStreamingSDK.dll not found next to the executable; falling back."));
		return false;
	}

	// Init establishes communication with the Business Streaming host process.
	// It fails when that process isn't running (e.g. plain PIE with no stream),
	// which is exactly when we want to fall back to the device backends.
	const int InitRc = BStreamingSDK_Init(nullptr);
	if (InitRc != BStreamingSDKError_None)
	{
		UE_LOG(LogEyeGaze, Warning,
			TEXT("BStreamingSDK_Init failed (rc=%d). Is Business Streaming running? Falling back."),
			InitRc);
		return false;
	}
	bBStreamingInitialized = true;
	bDeviceReady = true;

	UE_LOG(LogEyeGaze, Log,
		TEXT("[Diag] BStreaming initialized. Ensure eye tracking is switched ON on BOTH the PC app and the headset (PICO 4E)."));
	return true;
#else
	return false;
#endif
}

bool UEyeGazeLoggerComponent::TryStartOpenXRBackend()
{
	// Bind gaze resolution to our owning player so GetGazeData can return
	// world-space gaze relative to the active camera.
	if (const APawn* OwnerPawn = Cast<APawn>(GetOwner()))
	{
		if (APlayerController* PC = Cast<APlayerController>(OwnerPawn->GetController()))
		{
			UEyeTrackerFunctionLibrary::SetEyeTrackedPlayer(PC);
		}
	}

	return UEyeTrackerFunctionLibrary::IsEyeTrackerConnected();
}

void UEyeGazeLoggerComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (bLogging)
	{
		StopLogging();
	}
	bDeviceReady = false;

#if PLATFORM_WINDOWS
	if (bBStreamingInitialized)
	{
		BStreamingSDK_Deinit();
		bBStreamingInitialized = false;
	}
	if (BStreamingDllHandle)
	{
		FPlatformProcess::FreeDllHandle(BStreamingDllHandle);
		BStreamingDllHandle = nullptr;
	}
#endif

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

	FString Header = TEXT("index,Time,vr_X,vr_Y,vr_Z,vr_Pitch,vr_Yaw");
	if (bLogEyeExtensions)
	{
		Header += TEXT(",left_openness,right_openness,left_pupil,right_pupil,gaze_point_x,gaze_point_y,gaze_point_z,eye_status");
	}
	Header += TEXT("\n");
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

	FRotator GazeRot;
	FEyeExtras Extras;
	if (!GetGazeRotation(World, GazeRot, Extras))
	{
		return;
	}

	if (!bDiagLoggedFirstSample)
	{
		UE_LOG(LogEyeGaze, Log, TEXT("[Diag] First gaze sample captured via %s backend."),
			Backend == EEyeBackend::BStreaming ? TEXT("BStreaming") : TEXT("OpenXR"));
		bDiagLoggedFirstSample = true;
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

	const double VrX = HmdPos.X / 100.0;
	const double VrY = HmdPos.Y / 100.0;
	const double VrZ = HmdPos.Z / 100.0;

	const FString TimeStr = FDateTime::UtcNow().ToIso8601();

	FString Row = FString::Printf(
		TEXT("%lld,%s,%.6f,%.6f,%.6f,%.4f,%.4f"),
		SamplesWritten, *TimeStr,
		VrX, VrY, VrZ,
		GazeRot.Pitch, GazeRot.Yaw);

	if (bLogEyeExtensions)
	{
		// Empty cell for any field the active backend did not provide (NaN).
		auto Cell = [](float V) { return FMath::IsFinite(V) ? FString::Printf(TEXT("%.6f"), V) : FString(); };
		const FString StatusCell = (Extras.Status >= 0) ? FString::Printf(TEXT("%d"), Extras.Status) : FString();
		Row += FString::Printf(TEXT(",%s,%s,%s,%s,%s,%s,%s,%s"),
			*Cell(Extras.LeftOpenness), *Cell(Extras.RightOpenness),
			*Cell(Extras.LeftPupil), *Cell(Extras.RightPupil),
			*Cell((float)Extras.GazePoint.X), *Cell((float)Extras.GazePoint.Y), *Cell((float)Extras.GazePoint.Z),
			*StatusCell);
	}
	Row += TEXT("\n");

	RowBuffer.Add(MoveTemp(Row));
	++SamplesWritten;

	if (RowBuffer.Num() >= FlushThresholdRows)
	{
		FlushBuffer();
	}
}

bool UEyeGazeLoggerComponent::GetGazeRotation(UWorld* World, FRotator& OutGazeRot, FEyeExtras& OutExtras) const
{
	if (Backend == EEyeBackend::BStreaming)
	{
		return QueryBStreamingGaze(World, OutGazeRot, OutExtras);
	}

	if (Backend == EEyeBackend::OpenXR)
	{
		FEyeTrackerGazeData Gaze;
		if (!UEyeTrackerFunctionLibrary::GetGazeData(Gaze))
		{
			return false;
		}
		OutGazeRot = Gaze.GazeDirection.GetSafeNormal().Rotation();
		return true;
	}

	return false;
}

bool UEyeGazeLoggerComponent::QueryBStreamingGaze(UWorld* World, FRotator& OutGazeRot, FEyeExtras& OutExtras) const
{
#if PLATFORM_WINDOWS
	if (!bBStreamingInitialized)
	{
		return false;
	}

	PxrEyeTrackingData Data;
	FMemory::Memzero(&Data, sizeof(Data));
	const int Rc = BStreamingSDK_GetEyeTrackingData(Data);

	// One-time probe so we can tell "switches off / no data" from "API not served".
	if (!bBStreamingProbeLogged)
	{
		bBStreamingProbeLogged = true;
		UE_LOG(LogEyeGaze, Log,
			TEXT("[Diag] First GetEyeTrackingData rc=%d  combinedVec=(%.4f,%.4f,%.4f)  poseStatus L/R/C=%d/%d/%d  openness L/R=%.2f/%.2f"),
			Rc,
			Data.combinedEyeGazeVector[0], Data.combinedEyeGazeVector[1], Data.combinedEyeGazeVector[2],
			Data.leftEyePoseStatus, Data.rightEyePoseStatus, Data.combinedEyePoseStatus,
			Data.leftEyeOpenness, Data.rightEyeOpenness);
	}

	if (Rc != BStreamingSDKError_None)
	{
		// Periodic reminder (~ every 5 s at 72 Hz) while nothing comes through.
		if ((BStreamingFailCount++ % 360) == 0)
		{
			UE_LOG(LogEyeGaze, Warning,
				TEXT("[Diag] GetEyeTrackingData rc=%d (no data). Check eye-tracking switches are ON on the PC app AND the headset, and that a stream is active."),
				Rc);
		}
		return false;
	}

	const FVector RawDir(Data.combinedEyeGazeVector[0], Data.combinedEyeGazeVector[1], Data.combinedEyeGazeVector[2]);
	if (RawDir.IsNearlyZero())
	{
		if ((BStreamingFailCount++ % 360) == 0)
		{
			UE_LOG(LogEyeGaze, Warning,
				TEXT("[Diag] GetEyeTrackingData rc=0 but gaze vector is zero (status L/R/C=%d/%d/%d). Eye tracking likely not enabled/calibrated on the headset."),
				Data.leftEyePoseStatus, Data.rightEyePoseStatus, Data.combinedEyePoseStatus);
		}
		return false;
	}

	// Head-local (OpenXR frame) → UE local → world via the HMD orientation.
	const FVector LocalDir = OpenXRDirToUE(RawDir);
	FVector WorldDir = LocalDir;
	if (GEngine && GEngine->XRSystem.IsValid())
	{
		FQuat Q = FQuat::Identity;
		FVector P = FVector::ZeroVector;
		if (GEngine->XRSystem->GetCurrentPose(IXRTrackingSystem::HMDDeviceId, Q, P))
		{
			const FTransform T2W = GEngine->XRSystem->GetTrackingToWorldTransform();
			const FQuat HmdWorldQuat = T2W.GetRotation() * Q;
			WorldDir = HmdWorldQuat.RotateVector(LocalDir);
		}
	}
	// NOTE: if pitch/yaw read inverted on-device, flip the offending axis of
	// RawDir here after empirical verification.
	OutGazeRot = WorldDir.GetSafeNormal().Rotation();

	// Per-eye extras straight from the struct.
	OutExtras.LeftOpenness = Data.leftEyeOpenness;
	OutExtras.RightOpenness = Data.rightEyeOpenness;
	OutExtras.LeftPupil = Data.leftEyePupilDilation;
	OutExtras.RightPupil = Data.rightEyePupilDilation;
	OutExtras.GazePoint = FVector(Data.combinedEyeGazePoint[0], Data.combinedEyeGazePoint[1], Data.combinedEyeGazePoint[2]);
	OutExtras.Status = Data.combinedEyePoseStatus;
	return true;
#else
	return false;
#endif
}
