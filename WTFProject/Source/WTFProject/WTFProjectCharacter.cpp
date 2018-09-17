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
#pragma optimize("", off)
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

	// 	TextComponent = CreateDefaultSubobject<UTextRenderComponent>(TEXT("IncarGear"));
	// 	TextComponent->SetRelativeScale3D(FVector(3.0f, 3.0f, 3.0f));
	// 	TextComponent->SetRelativeLocation(FVector(35.0f, 5.0f, 20.0f));
	// 	TextComponent->SetRelativeRotation(FRotator(0.0f, 90.0f, 0.0f));
	// 	TextComponent->SetupAttachment(RootComponent);

	// Enable replication on the Sprite component so animations show up when networked
	GetSprite()->SetIsReplicated(true);
	bReplicates = true;
	GetSprite()->OnFinishedPlaying.AddDynamic(this, &AWTFProjectCharacter::UpdateAnimation);
	GetSprite()->SetLooping(false);
}

void AWTFProjectCharacter::Pick()
{
	FMovementBlock BlockInfo;
	BlockInfo.bTimed = true;
	BlockInfo.Reason = EMovementBlockReason::MBR_Pick;
	BlockInfo.Time = 1.f;
	CurrentAnimationState = EAnimationState::AS_Pick;
	MovementBlocks.Add(BlockInfo);
	UpdateFlipbook();
}

void AWTFProjectCharacter::Throw()
{
	UWorld* World = GetWorld();
	bIsAiming = false;
	if (World && StoneClass)
	{
		FActorSpawnParameters Params;
		Params.Instigator = this;
		FRotator Rotation = GetViewDirection().Rotation();
		FVector Location = GetActorLocation() + StoneSpawnLocation;
		World->SpawnActor(StoneClass, &Location, &Rotation, Params);
		CurrentAnimationState = EAnimationState::AS_Throw;
		UpdateFlipbook();
	}
}

void AWTFProjectCharacter::Aim()
{
	bIsAiming = true;
}

void AWTFProjectCharacter::CharJump()
{
	if (CanMove() && !GetCharacterMovement()->IsFalling())
	{
		Jump();
		CurrentAnimationState = EAnimationState::AS_Jump;
		UpdateFlipbook();
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
		if (GetCharacterMovement()->IsFalling())
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
	UpdateCharacter();	
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

void AWTFProjectCharacter::UpdateCharacter()
{
	// Update animation to match the motion
	UpdateAnimationState();

	// Now setup the rotation of the controller based on the direction we are travelling
	const FVector PlayerVelocity = GetVelocity();	
	float TravelDirection = PlayerVelocity.X;
	// Set the rotation so that the character faces his direction of travel.
	if (GetController())
	{
		APlayerController* PlController = Cast<APlayerController>(GetController());
		if (bIsAiming)
		{
			FVector Direction;
			FVector Location;
			if (PlController->DeprojectMousePositionToWorld(Location, Direction))
			{
				Location.Y = 0.f;
				Direction = (Location - GetActorLocation()).GetSafeNormal();
				if (Direction.X >= 0)
				{
					GetController()->SetControlRotation(FRotator(0.0f, 0.0f, 0.0f));
				}
				else
				{
					GetController()->SetControlRotation(FRotator(0.0, 180.0f, 0.0f));
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
#pragma optimize("", on)