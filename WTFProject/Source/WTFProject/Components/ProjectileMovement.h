// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "ProjectileMovement.generated.h"

/**
 * 
 */
UCLASS()
class WTFPROJECT_API UProjectileMovement : public UProjectileMovementComponent
{
	GENERATED_BODY()

public:
	virtual void HandleImpact(const FHitResult& Hit, float TimeSlice=0.f, const FVector& MoveDelta = FVector::ZeroVector) override;
	
	
};
