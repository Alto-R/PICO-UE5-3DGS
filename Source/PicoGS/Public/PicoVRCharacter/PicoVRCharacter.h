// VR character with Enhanced Input bindings.
// Required by UE5.5 OpenXR: legacy axis mappings are no longer surfaced by the
// OpenXR runtime, so motion controller axes must flow through IA + IMC.

#pragma once

#include "CoreMinimal.h"
#include "VRCharacter.h"
#include "InputActionValue.h"
#include "PicoVRCharacter.generated.h"

class UInputAction;
class UInputMappingContext;
class UInputComponent;
class UEyeGazeLoggerComponent;

UCLASS()
class PICOGS_API APicoVRCharacter : public AVRCharacter
{
	GENERATED_BODY()

public:
	APicoVRCharacter(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	// True = continuous smooth turn (like flat-screen games).
	// False = discrete snap turn in SnapTurnDegrees increments.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "VR Locomotion|Turn")
	bool bUseSmoothTurn = true;

	// Degrees per second when bUseSmoothTurn is true.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "VR Locomotion|Turn", meta = (ClampMin = "30.0", ClampMax = "360.0"))
	float SmoothTurnSpeed = 120.f;

	// Snap-turn step (used only when bUseSmoothTurn = false).
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "VR Locomotion|Turn")
	float SnapTurnDegrees = 45.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "VR Locomotion|Turn")
	float TurnTriggerThreshold = 0.7f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "VR Locomotion|Turn")
	float TurnReleaseThreshold = 0.3f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "VR Locomotion")
	float MoveDeadzone = 0.15f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "VR Locomotion|Input")
	TObjectPtr<UInputAction> IA_Move = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "VR Locomotion|Input")
	TObjectPtr<UInputAction> IA_Turn = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "VR Locomotion|Input")
	TObjectPtr<UInputMappingContext> IMC_VR = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "EyeGaze")
	TObjectPtr<UEyeGazeLoggerComponent> EyeGazeLogger = nullptr;

protected:
	virtual void BeginPlay() override;
	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;

	void HandleMove(const FInputActionValue& Value);
	void HandleTurn(const FInputActionValue& Value);
	void HandleTurnReleased(const FInputActionValue& Value);

	bool bIsTurning = false;
};
