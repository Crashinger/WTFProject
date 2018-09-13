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

UENUM(BlueprintType)
enum class EAnimationState: uint8
{
	AS_Idle UMETA(DisplayName = "Idle"),
	AS_Walk UMETA(DisplayName = "Walk"),
	AS_CarryIdle UMETA(DisplayName = "CarryIdle"),
	AS_CarryWalk UMETA(DisplayName = "CarryWalk"),
	AS_AimingUp UMETA(DisplayName = "AimingUp"),
	AS_AimingDown UMETA(DisplayName = "AimingDown"),
	AS_AimingFront UMETA(DisplayName = "AimingFront"),
	AS_Throw UMETA(DisplayName = "Throw"),
	AS_Pick UMETA(DisplayName = "Pick"),
	AS_Hit UMETA(DisplayName = "Hit"),
	AS_Jump UMETA(DisplayName = "Jump"),
	AS_Fall UMETA(DisplayName = "Fall")
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


	void UpdateAnimationState();

	/** Called to choose the correct animation to play based on the character's movement state */
	UFUNCTION()
	void UpdateAnimation();

	/** Called for side to side input */
	void MoveRight(float Value);

	void UpdateCharacter();

	/** Handle touch inputs. */
	void TouchStarted(const ETouchIndex::Type FingerIndex, const FVector Location);

	/** Handle touch stop event. */
	void TouchStopped(const ETouchIndex::Type FingerIndex, const FVector Location);

	// APawn interface
	virtual void SetupPlayerInputComponent(class UInputComponent* InputComponent) override;
	// End of APawn interface


	UFUNCTION(BlueprintCallable, Category = "Stone")
	void Throw();

	UFUNCTION(BlueprintCallable, Category = "Stone")
	FVector GetViewDirection() const;

public:
	AWTFProjectCharacter();

	/** Returns SideViewCameraComponent subobject **/
	FORCEINLINE class UCameraComponent* GetSideViewCameraComponent() const { return SideViewCameraComponent; }
	/** Returns CameraBoom subobject **/
	FORCEINLINE class USpringArmComponent* GetCameraBoom() const { return CameraBoom; }
};
