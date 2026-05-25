#include "PicoVRCharacter/PicoVRCharacter.h"

#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputAction.h"
#include "InputMappingContext.h"
#include "Engine/LocalPlayer.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "VRBaseCharacterMovementComponent.h"

APicoVRCharacter::APicoVRCharacter(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	PrimaryActorTick.bCanEverTick = true;

	if (UCharacterMovementComponent* Movement = GetCharacterMovement())
	{
		Movement->MaxWalkSpeed = 300.f;
		Movement->bOrientRotationToMovement = false;
		Movement->bUseControllerDesiredRotation = false;
	}
}

void APicoVRCharacter::BeginPlay()
{
	Super::BeginPlay();
	UE_LOG(LogTemp, Warning, TEXT("APicoVRCharacter BeginPlay (Enhanced Input)"));

	if (APlayerController* PC = Cast<APlayerController>(GetController()))
	{
		if (ULocalPlayer* LP = PC->GetLocalPlayer())
		{
			if (UEnhancedInputLocalPlayerSubsystem* Sub = LP->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>())
			{
				if (IMC_VR)
				{
					Sub->AddMappingContext(IMC_VR, 0);
					UE_LOG(LogTemp, Warning, TEXT("APicoVRCharacter: IMC_VR added on pawn"));
				}
				else
				{
					UE_LOG(LogTemp, Warning, TEXT("APicoVRCharacter: IMC_VR pawn slot empty (using Project Default Mapping Contexts)"));
				}
			}
		}
	}
}

void APicoVRCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	UEnhancedInputComponent* EIC = Cast<UEnhancedInputComponent>(PlayerInputComponent);
	if (!EIC)
	{
		UE_LOG(LogTemp, Error, TEXT("APicoVRCharacter: PlayerInputComponent is not UEnhancedInputComponent"));
		return;
	}

	if (IA_Move)
	{
		EIC->BindAction(IA_Move, ETriggerEvent::Triggered, this, &APicoVRCharacter::HandleMove);
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("APicoVRCharacter: IA_Move not assigned on pawn defaults"));
	}

	if (IA_Turn)
	{
		EIC->BindAction(IA_Turn, ETriggerEvent::Triggered, this, &APicoVRCharacter::HandleTurn);
		EIC->BindAction(IA_Turn, ETriggerEvent::Completed, this, &APicoVRCharacter::HandleTurnReleased);
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("APicoVRCharacter: IA_Turn not assigned on pawn defaults"));
	}
}

void APicoVRCharacter::HandleMove(const FInputActionValue& Value)
{
	const FVector2D Axis = Value.Get<FVector2D>();
	if (Axis.SizeSquared() < MoveDeadzone * MoveDeadzone)
	{
		return;
	}
	AddMovementInput(GetVRRightVector(), Axis.X);
	AddMovementInput(GetVRForwardVector(), Axis.Y);
}

void APicoVRCharacter::HandleTurn(const FInputActionValue& Value)
{
	const float V = Value.Get<float>();

	// Smooth (continuous) turn — rotate every frame proportional to stick.
	if (bUseSmoothTurn)
	{
		if (FMath::Abs(V) < MoveDeadzone)
		{
			return;
		}
		const float DeltaTime = GetWorld() ? GetWorld()->GetDeltaSeconds() : 0.f;
		const float YawDelta = V * SmoothTurnSpeed * DeltaTime;

		// VR-aware rotation: pivots around the HMD position so the camera
		// stays put while the capsule yaws. Falls back to a plain world
		// rotation if SetActorRotationVR is not available on this base.
		FRotator NewRot = GetActorRotation();
		NewRot.Yaw += YawDelta;
		SetActorRotationVR(NewRot, true, false);
		return;
	}

	// Snap (discrete) turn — single rotation step per stick deflection.
	if (FMath::Abs(V) < TurnReleaseThreshold)
	{
		bIsTurning = false;
		return;
	}
	if (bIsTurning || !VRMovementReference)
	{
		return;
	}
	if (V > TurnTriggerThreshold)
	{
		VRMovementReference->PerformMoveAction_SnapTurn(SnapTurnDegrees);
		bIsTurning = true;
	}
	else if (V < -TurnTriggerThreshold)
	{
		VRMovementReference->PerformMoveAction_SnapTurn(-SnapTurnDegrees);
		bIsTurning = true;
	}
}

void APicoVRCharacter::HandleTurnReleased(const FInputActionValue& /*Value*/)
{
	bIsTurning = false;
}
