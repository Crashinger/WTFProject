// Fill out your copyright notice in the Description page of Project Settings.

#include "ProjectileMovement.h"

void UProjectileMovement::HandleImpact(const FHitResult & Hit, float TimeSlice, const FVector & MoveDelta)
{
	Super::HandleImpact(Hit, TimeSlice, MoveDelta);
	ProjectileGravityScale = 1.f;
}
