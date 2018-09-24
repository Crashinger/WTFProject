// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#include "WTFProjectCharacter.h"
#include "Components/TextRenderComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/InputComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Controller.h"
#include "Engine/World.h"
#include "Objects/Stone.h"
#include "GameFramework/PlayerController.h"
#include "Camera/CameraComponent.h"

DEFINE_LOG_CATEGORY_STATIC(SideScrollerCharacter, Log, All);

//////////////////////////////////////////////////////////////////////////
// AWTFProjectCharacter

//#pragma optimize("", off)
AWTFProjectCharacter::AWTFProjectCharacter()
{
	// Use only Yaw from the controller and ignore the rest of the rotation.
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = true;
	bUseControllerRotationRoll = false;

	// Set the size of our collision capsule.
	GetCapsuleComponent()->SetCapsuleHalfHeight(96.0f);
	GetCapsuleComponent()->SetCapsuleRadius(40.0f);

	// Create a camera boom attached to the root (capsule)
	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);
	CameraBoom->TargetArmLength = 500.0f;
	CameraBoom->SocketOffset = FVector(0.0f, 0.0f, 75.0f);
	CameraBoom->bAbsoluteRotation = true;
	CameraBoom->bDoCollisionTest = false;
	CameraBoom->RelativeRotation = FRotator(0.0f, -90.0f, 0.0f);


	// Create an orthographic camera (no perspective) and attach it to the boom
	SideViewCameraComponent = CreateDefaultSubobject<UCameraComponent>(TEXT("SideViewCamera"));
	SideViewCameraComponent->ProjectionMode = ECameraProjectionMode::Orthographic;
	SideViewCameraComponent->OrthoWidth = 2048.0f;
	SideViewCameraComponent->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);

	// Prevent all automatic rotation behavior on the camera, character, and camera component
	CameraBoom->bAbsoluteRotation = true;
	SideViewCameraComponent->bUsePawnControlRotation = false;
	SideViewCameraComponent->bAutoActivate = true;
	if (GetCharacterMovement())
	{
		GetCharacterMovement()->bOrientRotationToMovement = false;

		// Configure character movement
		GetCharacterMovement()->GravityScale = 2.0f;
		GetCharacterMovement()->AirControl = 0.80f;
		GetCharacterMovement()->JumpZVelocity = 1000.f;
		GetCharacterMovement()->GroundFriction = 3.0f;
		GetCharacterMovement()->MaxWalkSpeed = 600.0f;
		GetCharacterMovement()->MaxFlySpeed = 600.0f;

		// Lock character motion onto the XZ plane, so the character can't move in or out of the screen
		GetCharacterMovement()->bConstrainToPlane = true;
		GetCharacterMovement()->SetPlaneConstraintNormal(FVector(0.0f, -1.0f, 0.0f));

		// Behave like a traditional 2D platformer character, with a flat bottom instead of a curved capsule bottom
		// Note: This can cause a little floating when going up inclines; you can choose the tradeoff between better
		// behavior on the edge of a ledge versus inclines by setting this to true or false
		GetCharacterMovement()->bUseFlatBaseForFloorChecks = true;
	}

	GetSprite()->SetIsReplicated(true);
	bReplicates = true;

	GetSprite()->OnFinishedPlaying.AddDynamic(this, &AWTFProjectCharacter::UpdateAnimation);
	GetSprite()->SetLooping(false);
}

void AWTFProjectCharacter::Pick()
{
	if (CanPick())
	{
		TArray<AActor*> Stones;
		GetOverlappingActors(Stones, AStone::StaticClass());
		if (Stones.Num() > 0)
		{
			bool Picked = false;
			int i = 0;
			while (i < Stones.Num() && !Picked)
			{
				PickStone = Cast<AStone>(Stones[i]);
				if (PickStone && PickStone->CanBePicked())
					Picked = true;
				else
					i++;
			}
			if (Picked)
			{
				GetStone();
				if (GetCharacterMovement())
					GetCharacterMovement()->StopMovementImmediately();
				FMovementBlock BlockInfo;
				BlockInfo.bTimed = true;
				BlockInfo.Reason = EMovementBlockReason::MBR_Pick;
				BlockInfo.Time = 1.f;
				CurrentAnimationState = EAnimationState::AS_Pick;
				AddMovementBlock(BlockInfo);
				UpdateFlipbook();
			}
		}
	}
}

bool AWTFProjectCharacter::CanPick()
{
	return !GetCharacterMovement() || !GetCharacterMovement()->IsFalling();
}

void AWTFProjectCharacter::GetStone()
{
	if (PickStone)
		PickStone->Destroy();
	Ammo++;
}

bool AWTFProjectCharacter::CanThrow()
{
	bool Res = !GetCharacterMovement() || !GetCharacterMovement()->IsFalling();
	Res &= Ammo > 0;
	Res &= !bThrowing;
	return Res;
}

void AWTFProjectCharacter::Throw()
{
	if (CanThrow())
	{
		bThrowing = true;
		StopAim();
		FMovementBlock BlockInfo;
		BlockInfo.Reason = EMovementBlockReason::MBR_Throw;
		BlockInfo.bTimed = true;
		BlockInfo.Time = ThrowTimer;
		ThrowTimerCurrent = ThrowTimer;
		AddMovementBlock(BlockInfo);
		CurrentAnimationState = EAnimationState::AS_Throw;
		UpdateFlipbook();
	}
}

void AWTFProjectCharacter::StopThrow()
{
	bThrowing = false;
	UWorld* World = GetWorld();
	if (World && StoneClass)
	{
		FActorSpawnParameters Params;
		Params.Instigator = this;
		FRotator Rotation = GetViewDirection().Rotation();
		FVector Location = GetActorLocation() + StoneSpawnLocation;
		Ammo--;
		World->SpawnActor(StoneClass, &Location, &Rotation, Params);
	}
}

void AWTFProjectCharacter::Aim()
{
	if (CanAim())
	{
		bIsAiming = true;
		FMovementBlock BlockInfo;
		BlockInfo.Reason = EMovementBlockReason::MBR_Aim;
		BlockInfo.bTimed = false;
		AddMovementBlock(BlockInfo);
	}
}

void AWTFProjectCharacter::StopAim()
{
	bIsAiming = false;
	RemoveSpecificMovementBlock(EMovementBlockReason::MBR_Aim);
}

bool AWTFProjectCharacter::IsAiming()
{
	return bIsAiming;
}

bool AWTFProjectCharacter::CanAim()
{
	bool Res = !GetCharacterMovement() || !GetCharacterMovement()->IsFalling();
	Res &= Ammo > 0 && !bThrowing;
	return Res;
}

void AWTFProjectCharacter::CharJump()
{
	if (CanMove() && GetCharacterMovement() && !GetCharacterMovement()->IsFalling())
	{
		Jump();
		CurrentAnimationState = EAnimationState::AS_Jump;
		UpdateFlipbook();
	}
}

void AWTFProjectCharacter::AddMovementBlock(FMovementBlock BlockInfo)
{
	int i = 0;
	bool Added = false;
	while (i < MovementBlocks.Num() && !Added)
	{
		if (MovementBlocks[i].Reason == BlockInfo.Reason)
		{
			MovementBlocks[i] = BlockInfo;
			Added = true;
		}
		else
			i++;
	}

	if (!Added)
		MovementBlocks.Add(BlockInfo);
}

void AWTFProjectCharacter::RemoveSpecificMovementBlock(EMovementBlockReason Block)
{
	int i = 0;
	while (i < MovementBlocks.Num())
	{
		if (MovementBlocks[i].Reason == Block)
		{
			MovementBlocks.RemoveAt(i);
		}
		else
			i++;
	}
}

FVector AWTFProjectCharacter::GetViewDirection() const
{
	FVector Res = FVector(0.f, 0.f, 0.f);
	APlayerController* PlController = Cast<APlayerController>(GetController());
	if (PlController)
	{
		FVector Direction;
		if (PlController->DeprojectMousePositionToWorld(Res, Direction))
			Res.Y = 0.f;
	}
	Res = (Res - GetActorLocation()).GetSafeNormal();
	return Res;
}


bool AWTFProjectCharacter::CanMove() const
{
	bool Res = true;
	if (MovementBlocks.Num() > 0)
		Res = false;
	return Res;
}

void AWTFProjectCharacter::UpdateMovementBlocks(float DeltaTime)
{
	int i = 0;
	bool RemoveCurrent;
	while (i < MovementBlocks.Num())
	{
		RemoveCurrent = false;
		if (MovementBlocks[i].bTimed)
		{
			MovementBlocks[i].Time -= DeltaTime;
			if (MovementBlocks[i].Time <= 0.f)
				RemoveCurrent = true;
		}
		if (RemoveCurrent)
			MovementBlocks.RemoveAt(i);
		else
			i++;
	}
}

void AWTFProjectCharacter::UpdateAnimationState()
{
	EAnimationState OldAnimationState = CurrentAnimationState;

	const FVector PlayerVelocity = GetVelocity();
	const float PlayerSpeedSqr = PlayerVelocity.SizeSquared();

	//Кривовато
	if (PlayerSpeedSqr > 0.0f)
	{
		if (GetCharacterMovement() && GetCharacterMovement()->IsFalling())
		{
			if (CurrentAnimationState != EAnimationState::AS_Jump)
				CurrentAnimationState = EAnimationState::AS_Fall;
		}
		else
			CurrentAnimationState = EAnimationState::AS_Walk;
	}
	else if (CurrentAnimationState == EAnimationState::AS_Walk || CurrentAnimationState == EAnimationState::AS_Fall || CurrentAnimationState == EAnimationState::AS_Jump)
	{
		CurrentAnimationState = EAnimationState::AS_Idle;
	}

	if (OldAnimationState != CurrentAnimationState)
	{
		UpdateAnimation();
	}
}

void AWTFProjectCharacter::UpdateFlipbook()
{
	if (AnimationStates.Contains(CurrentAnimationState) && AnimationStates[CurrentAnimationState].Animations.Num() > 0)
	{
		int Indx = FMath::Rand() % AnimationStates[CurrentAnimationState].Animations.Num();

		if (AnimationStates[CurrentAnimationState].Animations[Indx])
			GetSprite()->SetFlipbook(AnimationStates[CurrentAnimationState].Animations[Indx]);
	}
	GetSprite()->PlayFromStart();
}

void AWTFProjectCharacter::UpdateAnimation()
{
	if (
		CurrentAnimationState == EAnimationState::AS_Throw ||
		CurrentAnimationState == EAnimationState::AS_Pick ||
		CurrentAnimationState == EAnimationState::AS_Hit
		)
	{
		CurrentAnimationState = EAnimationState::AS_Idle;
	}
	else if (CurrentAnimationState == EAnimationState::AS_Jump)
	{
		CurrentAnimationState = EAnimationState::AS_Fall;
	}

	if (
		CurrentAnimationState != EAnimationState::AS_AimingDown &&
		CurrentAnimationState != EAnimationState::AS_AimingFront &&
		CurrentAnimationState != EAnimationState::AS_AimingUp
		)
	UpdateFlipbook();
}

void AWTFProjectCharacter::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	UpdateMovementBlocks(DeltaSeconds);
	UpdateCharacter(DeltaSeconds);
}

void AWTFProjectCharacter::BeginPlay()
{
	Super::BeginPlay();
	GetSprite()->PlayFromStart();
	MovementBlocks.Empty();
}


//////////////////////////////////////////////////////////////////////////
// Input

void AWTFProjectCharacter::SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent)
{
	// Note: the 'Jump' action and the 'MoveRight' axis are bound to actual keys/buttons/sticks in DefaultInput.ini (editable from Project Settings..Input)
	APlayerController* PlController = Cast<APlayerController>(GetController());
	if (PlController)
	{
		PlController->bShowMouseCursor = true;
	}

	PlayerInputComponent->BindAction("Jump", IE_Pressed, this, &AWTFProjectCharacter::CharJump);
	PlayerInputComponent->BindAction("Jump", IE_Released, this, &ACharacter::StopJumping);
	PlayerInputComponent->BindAction("Throw", IE_Pressed, this, &AWTFProjectCharacter::Aim);
	PlayerInputComponent->BindAction("Throw", IE_Released, this, &AWTFProjectCharacter::Throw);
	PlayerInputComponent->BindAction("Pick", IE_Pressed, this, &AWTFProjectCharacter::Pick);
	PlayerInputComponent->BindAxis("MoveRight", this, &AWTFProjectCharacter::MoveRight);

	PlayerInputComponent->BindTouch(IE_Pressed, this, &AWTFProjectCharacter::TouchStarted);
	PlayerInputComponent->BindTouch(IE_Released, this, &AWTFProjectCharacter::TouchStopped);
}

void AWTFProjectCharacter::MoveRight(float Value)
{
	/*UpdateChar();*/

	// Apply the input to the character motion
	if (CanMove())
		AddMovementInput(FVector(1.0f, 0.0f, 0.0f), Value);
}

void AWTFProjectCharacter::TouchStarted(const ETouchIndex::Type FingerIndex, const FVector Location)
{
	// Jump on any touch
	if (CanMove())
	{
		Jump();
	}
}

void AWTFProjectCharacter::TouchStopped(const ETouchIndex::Type FingerIndex, const FVector Location)
{
	// Cease jumping once touch stopped
	StopJumping();
}

void AWTFProjectCharacter::UpdateCharacter(float DeltaSeconds)
{
	UpdateAnimationState();

	const FVector PlayerVelocity = GetVelocity();	
	float TravelDirection = PlayerVelocity.X;

	if (IsAiming() && GetCharacterMovement() && GetCharacterMovement()->IsFalling())
	{
		StopAim();
	}

	if (bThrowing)
	{
		ThrowTimerCurrent -= DeltaSeconds;
		if (ThrowTimerCurrent <= 0.f)
		{
			StopThrow();
		}
	}

	APlayerController* PlController = Cast<APlayerController>(GetController());
	if (PlController)
	{
		if (IsAiming())
		{
			FVector Direction;
			FVector Location;
			if (PlController->DeprojectMousePositionToWorld(Location, Direction))
			{
				Location.Y = 0.f;
				Direction = (Location - GetActorLocation()).GetSafeNormal();
				if (Direction.X >= 0)
				{
					PlController->SetControlRotation(FRotator(0.0f, 0.0f, 0.0f));
				}
				else
				{
					PlController->SetControlRotation(FRotator(0.0, 180.0f, 0.0f));
				}
				float AimAngle = FMath::UnwindDegrees(FMath::RadiansToDegrees(acosf(FVector::DotProduct(GetActorForwardVector(), Direction)))) * FMath::Sign(Direction.Z);

				EAnimationState OldState = CurrentAnimationState;
				if (AimAngle > 30.f)
				{
					CurrentAnimationState = EAnimationState::AS_AimingUp;
				}
				else if (AimAngle < -30.f)
				{
					CurrentAnimationState = EAnimationState::AS_AimingDown;
				}
				else
				{
					CurrentAnimationState = EAnimationState::AS_AimingFront;
				}
				if (OldState != CurrentAnimationState)
				{
					UpdateFlipbook();
				}
			}
		}
		else if (TravelDirection < 0.0f)
		{
			GetController()->SetControlRotation(FRotator(0.0, 180.0f, 0.0f));
		}
		else if (TravelDirection > 0.0f)
		{
			GetController()->SetControlRotation(FRotator(0.0f, 0.0f, 0.0f));
		}
	}
}
//#pragma optimize("", on)