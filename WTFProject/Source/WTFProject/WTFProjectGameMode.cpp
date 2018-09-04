// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#include "WTFProjectGameMode.h"
#include "WTFProjectCharacter.h"

AWTFProjectGameMode::AWTFProjectGameMode()
{
	// Set default pawn class to our character
	DefaultPawnClass = AWTFProjectCharacter::StaticClass();	
}
