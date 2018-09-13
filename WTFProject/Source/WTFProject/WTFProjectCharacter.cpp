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

void AWTFProjectCharacter::Throw()
{
	UWorld* World = GetWorld();
	if (World && StoneClass)
	{
		FActorSpawnParameters Params;
		Params.Instigator = this;
		FRotator Rotation = GetViewDirection().Rotation();
		FVector Location = GetActorLocation() + StoneSpawnLocation;
		World->SpawnActor(StoneClass, &Location, &Rotation, Params);
	}
}

FVector AWTFProjectCharacter::GetViewDirection() const
{
	FVector Res = FVector(0.f, 0.f, 0.f);
	APlayerController* Controller = Cast<APlayerController>(GetController());
	if (Controller)
	{
		FVector Direction;
		if (Controller->DeprojectMousePositionToWorld(Res, Direction))
			Res.Y = 0.f;
	}
	Res = (Res - GetActorLocation()).GetSafeNormal();
	return Res;
}

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

//////////////////////////////////////////////////////////////////////////
// Animation

void AWTFProjectCharacter::UpdateAnimationState()
{
	EAnimationState OldAnimationState = CurrentAnimationState;

	const FVector PlayerVelocity = GetVelocity();
	const float PlayerSpeedSqr = PlayerVelocity.SizeSquared();

	if (PlayerSpeedSqr > 0.0f)
	{
		CurrentAnimationState = EAnimationState::AS_Walk;
	}
	else
	{
		CurrentAnimationState = EAnimationState::AS_Idle;
	}

	if (OldAnimationState != CurrentAnimationState)
	{
		UpdateAnimation();
	}
}

void AWTFProjectCharacter::UpdateAnimation()
{
	if (AnimationStates.Contains(CurrentAnimationState) && AnimationStates[CurrentAnimationState].Animations.Num() > 0)
	{
		int Indx = FMath::Rand() % AnimationStates[CurrentAnimationState].Animations.Num();

		if (AnimationStates[CurrentAnimationState].Animations[Indx])
			GetSprite()->SetFlipbook(AnimationStates[CurrentAnimationState].Animations[Indx]);
	}
	GetSprite()->PlayFromStart();
}

void AWTFProjectCharacter::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	
	UpdateCharacter();	
}

void AWTFProjectCharacter::BeginPlay()
{
	Super::BeginPlay();
	GetSprite()->PlayFromStart();
}


//////////////////////////////////////////////////////////////////////////
// Input

void AWTFProjectCharacter::SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent)
{
	// Note: the 'Jump' action and the 'MoveRight' axis are bound to actual keys/buttons/sticks in DefaultInput.ini (editable from Project Settings..Input)
	APlayerController* Controller = Cast<APlayerController>(GetController());
	if (Controller)
	{
		Controller->bShowMouseCursor = true;
	}

	PlayerInputComponent->BindAction("Jump", IE_Pressed, this, &ACharacter::Jump);
	PlayerInputComponent->BindAction("Jump", IE_Released, this, &ACharacter::StopJumping);
	PlayerInputComponent->BindAction("Throw", IE_Pressed, this, &AWTFProjectCharacter::Throw);
	PlayerInputComponent->BindAxis("MoveRight", this, &AWTFProjectCharacter::MoveRight);

	PlayerInputComponent->BindTouch(IE_Pressed, this, &AWTFProjectCharacter::TouchStarted);
	PlayerInputComponent->BindTouch(IE_Released, this, &AWTFProjectCharacter::TouchStopped);
}

void AWTFProjectCharacter::MoveRight(float Value)
{
	/*UpdateChar();*/

	// Apply the input to the character motion
	AddMovementInput(FVector(1.0f, 0.0f, 0.0f), Value);
}

void AWTFProjectCharacter::TouchStarted(const ETouchIndex::Type FingerIndex, const FVector Location)
{
	// Jump on any touch
	Jump();
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
	if (Controller != nullptr)
	{
		if (TravelDirection < 0.0f)
		{
			Controller->SetControlRotation(FRotator(0.0, 180.0f, 0.0f));
		}
		else if (TravelDirection > 0.0f)
		{
			Controller->SetControlRotation(FRotator(0.0f, 0.0f, 0.0f));
		}
	}
}
//#pragma optimize("", on)