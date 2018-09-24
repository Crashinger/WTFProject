// Fill out your copyright notice in the Description page of Project Settings.

#include "Stone.h"
#include "WTFProjectCharacter.h"

AStone::AStone()
{
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = true;

	CollisionSphere = CreateDefaultSubobject<USphereComponent>(TEXT("Collision Sphere"));
	CollisionSphere->OnComponentBeginOverlap.AddDynamic(this, &AStone::OnHit);
	RootComponent = CollisionSphere;

	Sprite = CreateDefaultSubobject<UPaperSpriteComponent>(TEXT("Sprite"));
	Sprite->SetupAttachment(RootComponent);

	MovementComponent = CreateDefaultSubobject<UProjectileMovement>(TEXT("ProjectileMovementComponent"));
	MovementComponent->SetUpdatedComponent(CollisionSphere);

	MovementComponent->bConstrainToPlane = true;
	MovementComponent->SetPlaneConstraintNormal(FVector(0.0f, -1.0f, 0.0f));
}

bool AStone::CanBePicked()
{
	return !bCanDealDamage;
}

void AStone::OnHit(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult &SweepResult)
{
	AWTFProjectCharacter* HitChar = Cast<AWTFProjectCharacter>(OtherActor);
	if (bCanDealDamage && (!HitChar || HitChar != GetInstigator()))
	{
		bCanDealDamage = false;
		MovementComponent->HandleImpact(SweepResult);
	}
}
