// Copyright 2026 Vaeneth. All Rights Reserved.

#include "VaenethMusicLibrary.h"

#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "Subsystems/GameInstanceSubsystem.h"

namespace
{
	const TCHAR* MusicSubsystemPath =
		TEXT("/Vaeneth/Music/Core/BP_VN_MusicSubsystem.BP_VN_MusicSubsystem_C");

	// Blueprint "float" parameters are doubles in UE5. Using float here silently
	// corrupts every value passed through ProcessEvent, so every real parameter
	// below must stay a double and match the Blueprint declaration order exactly.
	struct FP_State        { uint8 NewState; bool bForce; };
	struct FP_MusicSet     { UPrimaryDataAsset* NewMusicSet; };
	struct FP_Stop         { double FadeOutTime; };
	struct FP_Stinger      { USoundBase* Stinger; double Volume; };
	struct FP_Intensity    { double NewIntensity; };
	struct FP_Threat       { int32 Delta; };
	struct FP_Volume       { double NewVolume; };
	struct FP_PushDuck     { double DuckAmount; double AttackTime; };
	struct FP_PopDuck      { double ReleaseTime; };
	struct FP_Ducked       { bool bDuck; };
	struct FP_EnterZone    { AActor* Zone; uint8 State; int32 Priority; double Intensity; bool bApplyIntensity; };
	struct FP_LeaveZone    { AActor* Zone; };
	struct FP_Override     { uint8 State; int32 Priority; };
	struct FP_Settings     { UPrimaryDataAsset* SettingsAsset; };
	struct FP_Listener     { UObject* Listener; };
	struct FP_ShowOverlay  { bool bShow; };
}

UGameInstanceSubsystem* UVaenethMusicLibrary::GetMusicSubsystem(const UObject* WorldContextObject)
{
	if (!WorldContextObject || !GEngine)
	{
		return nullptr;
	}

	const UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::ReturnNull);
	if (!World)
	{
		return nullptr;
	}

	UGameInstance* GameInstance = World->GetGameInstance();
	if (!GameInstance)
	{
		return nullptr;
	}

	UClass* SubsystemClass = LoadClass<UGameInstanceSubsystem>(nullptr, MusicSubsystemPath);
	if (!SubsystemClass)
	{
		UE_LOG(LogTemp, Error, TEXT("Vaeneth: could not load %s. Was the plugin folder renamed? It must stay named 'Vaeneth'."), MusicSubsystemPath);
		return nullptr;
	}
	return GameInstance->GetSubsystemBase(SubsystemClass);
}

void UVaenethMusicLibrary::CallSubsystemFunction(const UObject* WorldContextObject, FName FunctionName, void* Params)
{
	UGameInstanceSubsystem* Subsystem = GetMusicSubsystem(WorldContextObject);
	if (!Subsystem)
	{
		return;
	}

	// The subsystem has no world of its own, so hand it the caller's context first.
	if (UFunction* CacheFunc = Subsystem->FindFunction(TEXT("CacheWorldContext")))
	{
		struct { const UObject* Ctx; } CacheParams { WorldContextObject };
		Subsystem->ProcessEvent(CacheFunc, &CacheParams);
	}

	if (UFunction* Func = Subsystem->FindFunction(FunctionName))
	{
		Subsystem->ProcessEvent(Func, Params);
	}
}

void UVaenethMusicLibrary::SetMusicState(const UObject* Ctx, uint8 NewState, bool bForce)
{ FP_State P { NewState, bForce }; CallSubsystemFunction(Ctx, TEXT("SetMusicState"), &P); }

void UVaenethMusicLibrary::SetMusicSet(const UObject* Ctx, UPrimaryDataAsset* MusicSet)
{ FP_MusicSet P { MusicSet }; CallSubsystemFunction(Ctx, TEXT("SetMusicSet"), &P); }

void UVaenethMusicLibrary::StopMusic(const UObject* Ctx, double FadeOutTime)
{ FP_Stop P { FadeOutTime }; CallSubsystemFunction(Ctx, TEXT("StopMusic"), &P); }

void UVaenethMusicLibrary::PlayStinger(const UObject* Ctx, USoundBase* Stinger, double Volume)
{ FP_Stinger P { Stinger, Volume }; CallSubsystemFunction(Ctx, TEXT("PlayStinger"), &P); }

void UVaenethMusicLibrary::SetIntensity(const UObject* Ctx, double Intensity)
{ FP_Intensity P { Intensity }; CallSubsystemFunction(Ctx, TEXT("SetIntensity"), &P); }

void UVaenethMusicLibrary::ModifyThreatCount(const UObject* Ctx, int32 Delta)
{ FP_Threat P { Delta }; CallSubsystemFunction(Ctx, TEXT("ModifyThreatCount"), &P); }

void UVaenethMusicLibrary::SetMasterVolume(const UObject* Ctx, double Volume)
{ FP_Volume P { Volume }; CallSubsystemFunction(Ctx, TEXT("SetMasterVolume"), &P); }

void UVaenethMusicLibrary::BeginDialogue(const UObject* Ctx, double DuckAmount, double FadeDownTime)
{ FP_PushDuck P { DuckAmount, FadeDownTime }; CallSubsystemFunction(Ctx, TEXT("PushDuck"), &P); }

void UVaenethMusicLibrary::EndDialogue(const UObject* Ctx, double FadeUpTime)
{ FP_PopDuck P { FadeUpTime }; CallSubsystemFunction(Ctx, TEXT("PopDuck"), &P); }

void UVaenethMusicLibrary::SetDucked(const UObject* Ctx, bool bDuck)
{ FP_Ducked P { bDuck }; CallSubsystemFunction(Ctx, TEXT("SetDucked"), &P); }

void UVaenethMusicLibrary::EnterZone(const UObject* Ctx, AActor* Zone, uint8 State,
	int32 Priority, double Intensity, bool bApplyIntensity)
{ FP_EnterZone P { Zone, State, Priority, Intensity, bApplyIntensity }; CallSubsystemFunction(Ctx, TEXT("RegisterZone"), &P); }

void UVaenethMusicLibrary::LeaveZone(const UObject* Ctx, AActor* Zone)
{
	FP_LeaveZone P { Zone };
	CallSubsystemFunction(Ctx, TEXT("UnregisterZone"), &P);
	CallSubsystemFunction(Ctx, TEXT("ResolveActiveZones"), nullptr);
}

void UVaenethMusicLibrary::SetStateOverride(const UObject* Ctx, uint8 State, int32 Priority)
{ FP_Override P { State, Priority }; CallSubsystemFunction(Ctx, TEXT("SetStateOverride"), &P); }

void UVaenethMusicLibrary::ClearStateOverride(const UObject* Ctx)
{ CallSubsystemFunction(Ctx, TEXT("ClearStateOverride"), nullptr); }

void UVaenethMusicLibrary::ApplySettings(const UObject* Ctx, UPrimaryDataAsset* SettingsAsset)
{ FP_Settings P { SettingsAsset }; CallSubsystemFunction(Ctx, TEXT("ApplySettings"), &P); }

void UVaenethMusicLibrary::NotifyLevelTravel(const UObject* Ctx)
{ CallSubsystemFunction(Ctx, TEXT("NotifyLevelTravel"), nullptr); }

void UVaenethMusicLibrary::RegisterMusicListener(const UObject* Ctx, UObject* Listener)
{ FP_Listener P { Listener }; CallSubsystemFunction(Ctx, TEXT("RegisterMusicListener"), &P); }

void UVaenethMusicLibrary::UnregisterMusicListener(const UObject* Ctx, UObject* Listener)
{ FP_Listener P { Listener }; CallSubsystemFunction(Ctx, TEXT("UnregisterMusicListener"), &P); }

void UVaenethMusicLibrary::ShowDebugOverlay(const UObject* Ctx, bool bShow)
{ FP_ShowOverlay P { bShow }; CallSubsystemFunction(Ctx, TEXT("ShowDebugOverlay"), &P); }
