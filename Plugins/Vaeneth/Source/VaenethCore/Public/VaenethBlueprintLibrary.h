// Copyright 2026 Vaeneth. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Templates/SubclassOf.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "VaenethBlueprintLibrary.generated.h"

class USoundBase;
class UGameInstanceSubsystem;

/**
 * Utility nodes that plug the gaps Blueprint leaves open.
 * Every function here exists because the engine holds information Blueprint
 * cannot reach. None of them require the user to write or read C++.
 */
UCLASS()
class VAENETHCORE_API UVaenethBlueprintLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/** Length of a sound in seconds. Returns 0 for sounds that loop indefinitely. */
	UFUNCTION(BlueprintPure, Category = "Vaeneth|Audio")
	static float GetSoundDuration(const USoundBase* Sound);

	/** True when the sound loops forever and therefore has no meaningful duration. */
	UFUNCTION(BlueprintPure, Category = "Vaeneth|Audio")
	static bool IsSoundLooping(const USoundBase* Sound);

	/** Resolves a Game Instance Subsystem with the world context filled in automatically. */
	UFUNCTION(BlueprintPure, Category = "Vaeneth|Core",
		meta = (WorldContext = "WorldContextObject"))
	static UGameInstanceSubsystem* GetVaenethSubsystem(
		const UObject* WorldContextObject,
		TSubclassOf<UGameInstanceSubsystem> SubsystemClass);

	/** The object the framework should use as its world context. Auto-filled by the engine. */
	UFUNCTION(BlueprintPure, Category = "Vaeneth|Core",
		meta = (WorldContext = "WorldContextObject"))
	static UObject* GetImplicitWorldContext(UObject* WorldContextObject);
};
