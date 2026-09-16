// Copyright 2026 Vaeneth. All Rights Reserved.

#include "VaenethBlueprintLibrary.h"

#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "Sound/SoundBase.h"
#include "Sound/SoundNode.h"
#include "Subsystems/GameInstanceSubsystem.h"

float UVaenethBlueprintLibrary::GetSoundDuration(const USoundBase* Sound)
{
	if (!Sound)
	{
		return 0.0f;
	}

	const float Duration = Sound->GetDuration();

	// Looping sounds report a sentinel duration. Report 0 so callers can treat
	// "no meaningful length" uniformly instead of scheduling on a huge number.
	if (Duration >= INDEFINITELY_LOOPING_DURATION)
	{
		return 0.0f;
	}

	return Duration;
}

bool UVaenethBlueprintLibrary::IsSoundLooping(const USoundBase* Sound)
{
	return Sound ? Sound->IsLooping() : false;
}

UGameInstanceSubsystem* UVaenethBlueprintLibrary::GetVaenethSubsystem(
	const UObject* WorldContextObject,
	TSubclassOf<UGameInstanceSubsystem> SubsystemClass)
{
	if (!WorldContextObject || !SubsystemClass)
	{
		return nullptr;
	}

	const UWorld* World = GEngine
		? GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::ReturnNull)
		: nullptr;

	if (!World)
	{
		return nullptr;
	}

	const UGameInstance* GameInstance = World->GetGameInstance();
	return GameInstance ? GameInstance->GetSubsystemBase(SubsystemClass) : nullptr;
}

UObject* UVaenethBlueprintLibrary::GetImplicitWorldContext(UObject* WorldContextObject)
{
	return WorldContextObject;
}
