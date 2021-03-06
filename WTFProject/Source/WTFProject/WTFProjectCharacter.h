// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "PaperCharacter.h"
#include "PaperFlipbookComponent.h"
#include "WTFProjectCharacter.generated.h"

class UTextRenderComponent;
class AStone;

/**
 * This class is the default character for WTFProject, and it is responsible for all
 * physical interaction between the player and the world.
 *
 * The capsule component (inherited from ACharacter) handles collision with the world
 * The CharacterMovementComponent (inherited from ACharacter) handles movement of the collision capsule
 * The Sprite component (inherited from APaperCharacter) handles the visuals
 */


enum class ESimpleAnimationState : uint8
{
	SAS_Idle UMETA(DisplayName = "Idle"),
	SAS_Walk UMETA(DisplayName = "Walk"),
	SAS_Jump UMETA(DisplayName = "Jump"),
	SAS_Fall UMETA(DisplayName = "Fall"),
	SAS_Aim UMETA(DisplayName = "Aim"),
	SAS_Throw UMETA(DisplayName = "Throw"),
	SAS_Pick UMETA(DisplayName = "Pick")
};

UENUM(BlueprintType)
enum class EAnimationState: uint8
{
	AS_Idle UMETA(DisplayName = "Idle"),
	AS_Walk UMETA(DisplayName = "Walk"),
	AS_CarryIdle UMETA(DisplayName = "CarryIdle"),
	AS_CarryWalk UMETA(DisplayName = "CarryWalk"),
	AS_CarryFall UMETA(DisplayName = "CarryFall"),
	AS_CarryJump UMETA(DisplayName = "CarryJump"),
	AS_AimingUp UMETA(DisplayName = "AimingUp"),
	AS_AimingDown UMETA(DisplayName = "AimingDown"),
	AS_AimingFront UMETA(DisplayName = "AimingFront"),
	AS_WalkAimingUp UMETA(DisplayName = "WalkAimingUp"),
	AS_WalkAimingDown UMETA(DisplayName = "WalkAimingDown"),
	AS_WalkAimingFront UMETA(DisplayName = "WalkAimingFront"),
	AS_ThrowUp UMETA(DisplayName = "ThrowUp"),
	AS_ThrowDown UMETA(DisplayName = "ThrowDown"),
	AS_ThrowFront UMETA(DisplayName = "ThrowForward"),
	AS_Pick UMETA(DisplayName = "Pick"),
	AS_Hit UMETA(DisplayName = "Hit"),
	AS_Jump UMETA(DisplayName = "Jump"),
	AS_Fall UMETA(DisplayName = "Fall")
};

UENUM(BlueprintType)
enum class EMovementBlockReason : uint8
{
	MBR_Pick UMETA(DisplayName = "Pick"),
	MBR_Aim UMETA(DisplayName = "Aim"),
	MBR_Throw UMETA(DisplayName = "Throw")
};

USTRUCT(BlueprintType)
struct FMovementBlock
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MovementBlock")
	EMovementBlockReason Reason = EMovementBlockReason::MBR_Aim;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MovementBlock")
	bool bTimed = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MovementBlock")
	float Time = 0.f;
};

USTRUCT(BlueprintType)
struct FAnimations
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Animaton")
	TArray<UPaperFlipbook*> Animations;
};

UCLASS(config=Game)
class AWTFProjectCharacter : public APaperCharacter
{
	GENERATED_BODY()

	/** Side view camera */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category=Camera, meta=(AllowPrivateAccess="true"))
	class UCameraComponent* SideViewCameraComponent;

	/** Camera boom positioning the camera beside the character */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
	class USpringArmComponent* CameraBoom;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Stone, meta = (AllowPrivateAccess = "true"))
	class UPaperSpriteComponent* StoneSpriteComponent;

	//UTextRenderComponent* TextComponent;
	virtual void Tick(float DeltaSeconds) override;
	virtual void BeginPlay() override;

	FVector StoneSpawnLocation = FVector(0.f, 0.f, 0.f);

protected:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stone")
	TSubclassOf<AStone> StoneClass = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Animations")
	TMap<EAnimationState, FAnimations> AnimationStates;

	EAnimationState CurrentAnimationState = EAnimationState::AS_Idle;

private:
	bool bIsAiming = false;
	FVector AimDirection;
	FVector ThrowDirection;

	TArray<FMovementBlock> MovementBlocks;

	int Ammo = 5;

	float ThrowTimerCurrent = 0.f;
	bool bThrowing = false;

	AStone* PickStone = nullptr;

	bool bIsReversing = false;

public:
	float ThrowTimer = 0.5f;

protected:
	bool CanMove() const;
	void UpdateMovementBlocks(float DeltaTime);

	UFUNCTION()
	void UpdateAnimation();
	void UpdateFlipbook(bool SameFrame);
	void UpdateAnimationState();
	void SetAnimationState(ESimpleAnimationState NewState);

	void MoveRight(float Value);
	void CharJump();

	void UpdateCharacter(float DeltaSeconds);

	void TouchStarted(const ETouchIndex::Type FingerIndex, const FVector Location);
	void TouchStopped(const ETouchIndex::Type FingerIndex, const FVector Location);

	virtual void SetupPlayerInputComponent(class UInputComponent* InputComponent) override;

	void Pick();
	bool CanPick();
	void GetStone();

	bool CanThrow();
	void Throw();
	void StopThrow();

	void Aim();
	void StopAim();
	bool IsAiming();
	bool CanAim();

	void AddMovementBlock(FMovementBlock BlockInfo);
	void RemoveSpecificMovementBlock(EMovementBlockReason Block);

	void AttachStone();
	void DetachStone();

	FVector GetViewDirection() const;

	void SetCharacterDirectionRight(bool IsRight);

public:

	AWTFProjectCharacter();

	FORCEINLINE class UCameraComponent* GetSideViewCameraComponent() const { return SideViewCameraComponent; }
	FORCEINLINE class USpringArmComponent* GetCameraBoom() const { return CameraBoom; }

};
