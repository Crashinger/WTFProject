// Fill out your copyright notice in the Description page of Project Settings.

#include "Stone.h"

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
}

void AStone::OnHit(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult &SweepResult)
{
	if (CanDealDamage)
	{
		CanDealDamage = false;
		MovementComponent->HandleImpact(SweepResult);
	}
}
