// Copyright 2026 Vaeneth. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "VaenethMusicLibrary.generated.h"

class AActor;
class USoundBase;
class UPrimaryDataAsset;
class UGameInstanceSubsystem;

/**
 * Public music API.
 *
 * These nodes carry the WorldContext metadata, so Unreal fills the context in
 * automatically and the pin never appears in Blueprint. A Blueprint-authored
 * function library cannot do this, which is why every call previously needed
 * Self wired by hand.
 *
 * All gameplay logic still lives in the Blueprint subsystem. These are thin
 * forwarders, so the behaviour stays fully editable without touching C++.
 */
UCLASS()
class VAENETHCORE_API UVaenethMusicLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	// ---- Playback ----------------------------------------------------------

	UFUNCTION(BlueprintCallable, Category = "Vaeneth|Music",
		meta = (WorldContext = "WorldContextObject", AdvancedDisplay = "bForce"))
	static void SetMusicState(const UObject* WorldContextObject, uint8 NewState, bool bForce = false);

	UFUNCTION(BlueprintCallable, Category = "Vaeneth|Music",
		meta = (WorldContext = "WorldContextObject"))
	static void SetMusicSet(const UObject* WorldContextObject, UPrimaryDataAsset* MusicSet);

	UFUNCTION(BlueprintCallable, Category = "Vaeneth|Music",
		meta = (WorldContext = "WorldContextObject"))
	static void StopMusic(const UObject* WorldContextObject, double FadeOutTime = 2.0);

	UFUNCTION(BlueprintCallable, Category = "Vaeneth|Music",
		meta = (WorldContext = "WorldContextObject"))
	static void PlayStinger(const UObject* WorldContextObject, USoundBase* Stinger, double Volume = 1.0);

	// ---- Intensity ---------------------------------------------------------

	UFUNCTION(BlueprintCallable, Category = "Vaeneth|Music",
		meta = (WorldContext = "WorldContextObject"))
	static void SetIntensity(const UObject* WorldContextObject, double Intensity);

	/** +1 when an enemy engages, -1 when it disengages or dies. */
	UFUNCTION(BlueprintCallable, Category = "Vaeneth|Music",
		meta = (WorldContext = "WorldContextObject"))
	static void ModifyThreatCount(const UObject* WorldContextObject, int32 Delta);

	// ---- Volume and ducking ------------------------------------------------

	UFUNCTION(BlueprintCallable, Category = "Vaeneth|Music",
		meta = (WorldContext = "WorldContextObject"))
	static void SetMasterVolume(const UObject* WorldContextObject, double Volume);

	/** Stacked: nested dialogue and cutscenes do not fight each other. */
	UFUNCTION(BlueprintCallable, Category = "Vaeneth|Music",
		meta = (WorldContext = "WorldContextObject"))
	static void BeginDialogue(const UObject* WorldContextObject, double DuckAmount = 0.1, double FadeDownTime = 0.4);

	UFUNCTION(BlueprintCallable, Category = "Vaeneth|Music",
		meta = (WorldContext = "WorldContextObject"))
	static void EndDialogue(const UObject* WorldContextObject, double FadeUpTime = 1.2);

	UFUNCTION(BlueprintCallable, Category = "Vaeneth|Music",
		meta = (WorldContext = "WorldContextObject"))
	static void SetDucked(const UObject* WorldContextObject, bool bDuck);

	// ---- Zones -------------------------------------------------------------

	UFUNCTION(BlueprintCallable, Category = "Vaeneth|Music",
		meta = (WorldContext = "WorldContextObject"))
	static void EnterZone(const UObject* WorldContextObject, AActor* Zone, uint8 State,
		int32 Priority, double Intensity, bool bApplyIntensity);

	UFUNCTION(BlueprintCallable, Category = "Vaeneth|Music",
		meta = (WorldContext = "WorldContextObject"))
	static void LeaveZone(const UObject* WorldContextObject, AActor* Zone);

	// ---- Narrative override ------------------------------------------------

	UFUNCTION(BlueprintCallable, Category = "Vaeneth|Music",
		meta = (WorldContext = "WorldContextObject"))
	static void SetStateOverride(const UObject* WorldContextObject, uint8 State, int32 Priority = 50);

	UFUNCTION(BlueprintCallable, Category = "Vaeneth|Music",
		meta = (WorldContext = "WorldContextObject"))
	static void ClearStateOverride(const UObject* WorldContextObject);

	// ---- Lifecycle ---------------------------------------------------------

	UFUNCTION(BlueprintCallable, Category = "Vaeneth|Music",
		meta = (WorldContext = "WorldContextObject"))
	static void ApplySettings(const UObject* WorldContextObject, UPrimaryDataAsset* SettingsAsset);

	/** Call this immediately before Open Level so the music survives the change. */
	UFUNCTION(BlueprintCallable, Category = "Vaeneth|Music",
		meta = (WorldContext = "WorldContextObject"))
	static void NotifyLevelTravel(const UObject* WorldContextObject);

	// ---- Listeners ---------------------------------------------------------

	UFUNCTION(BlueprintCallable, Category = "Vaeneth|Music",
		meta = (WorldContext = "WorldContextObject"))
	static void RegisterMusicListener(const UObject* WorldContextObject, UObject* Listener);

	UFUNCTION(BlueprintCallable, Category = "Vaeneth|Music",
		meta = (WorldContext = "WorldContextObject"))
	static void UnregisterMusicListener(const UObject* WorldContextObject, UObject* Listener);

	// ---- Debug -------------------------------------------------------------

	UFUNCTION(BlueprintCallable, Category = "Vaeneth|Music",
		meta = (WorldContext = "WorldContextObject"))
	static void ShowDebugOverlay(const UObject* WorldContextObject, bool bShow);

private:
	static UGameInstanceSubsystem* GetMusicSubsystem(const UObject* WorldContextObject);
	static void CallSubsystemFunction(const UObject* WorldContextObject, FName FunctionName, void* Params);
};
