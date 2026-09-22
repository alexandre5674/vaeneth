// Copyright Vaeneth. All Rights Reserved.

#include "Combat/CombatStateComponent.h"

#include "GameFramework/Pawn.h"
#include "Kismet/GameplayStatics.h"
#include "HAL/IConsoleManager.h"
#include "UObject/UnrealType.h"

namespace CombatStateTags
{
    static const FName Idle(TEXT("State.Combat.Idle"));
    static const FName Attacking(TEXT("State.Combat.Attacking"));
    static const FName Dodging(TEXT("State.Combat.Dodging"));
    static const FName Blocking(TEXT("State.Combat.Blocking"));
    static const FName Parrying(TEXT("State.Combat.Parrying"));
    static const FName Staggered(TEXT("State.Combat.Staggered"));
    static const FName Launched(TEXT("State.Combat.Launched"));
    static const FName Dead(TEXT("State.Combat.Dead"));
}

namespace CombatStateConsole
{
    static FGameplayTag MakeTag(const FString& TagName)
    {
        return FGameplayTag::RequestGameplayTag(FName(*TagName), false);
    }

    static FString StateToString(const FGameplayTag& State)
    {
        return State.IsValid() ? State.ToString() : TEXT("<Invalid>");
    }

    static UCombatStateComponent* FindPlayerCombatState(UWorld* World)
    {
        if (!World)
        {
            return nullptr;
        }

        APawn* PlayerPawn = UGameplayStatics::GetPlayerPawn(World, 0);
        return PlayerPawn ? PlayerPawn->FindComponentByClass<UCombatStateComponent>() : nullptr;
    }
}

UCombatStateComponent::UCombatStateComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
    CurrentState = FGameplayTag::RequestGameplayTag(CombatStateTags::Idle, false);
}

void UCombatStateComponent::BeginPlay()
{
    Super::BeginPlay();

    if (!CurrentState.IsValid())
    {
        CurrentState = FGameplayTag::RequestGameplayTag(CombatStateTags::Idle, false);
    }

    SynchronizeGameplayTags();
}

bool UCombatStateComponent::RequestState(FGameplayTag NewState)
{
    if (!IsValidCombatState(NewState) || IsInState(FGameplayTag::RequestGameplayTag(CombatStateTags::Dead, false)))
    {
        return false;
    }

    if (IsInState(NewState))
    {
        return true;
    }

    if (!IsAllowedTransition(CurrentState, NewState))
    {
        return false;
    }

    ApplyStateChange(NewState, true);
    return true;
}

void UCombatStateComponent::ForceState(FGameplayTag NewState)
{
    if (!IsValidCombatState(NewState) || IsInState(NewState))
    {
        return;
    }

    ApplyStateChange(NewState, true);
}

FGameplayTag UCombatStateComponent::GetCurrentState() const
{
    return CurrentState;
}

bool UCombatStateComponent::IsInState(FGameplayTag State) const
{
    return CurrentState.IsValid() && State.IsValid() && CurrentState == State;
}

bool UCombatStateComponent::CanInterruptCurrentState_Implementation()
{
    return false;
}

bool UCombatStateComponent::IsValidCombatState(FGameplayTag State) const
{
    if (!State.IsValid())
    {
        return false;
    }

    static const TArray<FName> ValidStateNames =
    {
        CombatStateTags::Idle,
        CombatStateTags::Attacking,
        CombatStateTags::Dodging,
        CombatStateTags::Blocking,
        CombatStateTags::Parrying,
        CombatStateTags::Staggered,
        CombatStateTags::Launched,
        CombatStateTags::Dead
    };

    return ValidStateNames.Contains(State.GetTagName());
}

bool UCombatStateComponent::IsAllowedTransition(FGameplayTag FromState, FGameplayTag ToState)
{
    const FGameplayTag Idle = FGameplayTag::RequestGameplayTag(CombatStateTags::Idle, false);
    const FGameplayTag Attacking = FGameplayTag::RequestGameplayTag(CombatStateTags::Attacking, false);
    const FGameplayTag Dodging = FGameplayTag::RequestGameplayTag(CombatStateTags::Dodging, false);
    const FGameplayTag Blocking = FGameplayTag::RequestGameplayTag(CombatStateTags::Blocking, false);
    const FGameplayTag Parrying = FGameplayTag::RequestGameplayTag(CombatStateTags::Parrying, false);
    const FGameplayTag Staggered = FGameplayTag::RequestGameplayTag(CombatStateTags::Staggered, false);
    const FGameplayTag Launched = FGameplayTag::RequestGameplayTag(CombatStateTags::Launched, false);
    const FGameplayTag Dead = FGameplayTag::RequestGameplayTag(CombatStateTags::Dead, false);

    if (FromState == Dead)
    {
        return false;
    }

    if (FromState == Idle)
    {
        return ToState == Attacking || ToState == Dodging || ToState == Blocking;
    }

    if (FromState == Attacking)
    {
        if (ToState == Idle)
        {
            return true;
        }

        if ((ToState == Attacking || ToState == Dodging) && CanInterruptCurrentState())
        {
            return true;
        }

        return false;
    }

    if (FromState == Dodging)
    {
        return ToState == Idle;
    }

    if (FromState == Blocking)
    {
        return ToState == Idle || ToState == Parrying;
    }

    if (FromState == Parrying || FromState == Staggered || FromState == Launched)
    {
        return ToState == Idle;
    }

    return false;
}

void UCombatStateComponent::ApplyStateChange(FGameplayTag NewState, bool bBroadcast)
{
    const FGameplayTag OldState = CurrentState;
    CurrentState = NewState;
    SynchronizeGameplayTags();

    if (bBroadcast && OldState != NewState)
    {
        UE_LOG(LogTemp, Log, TEXT("CombatState dispatcher: %s -> %s"), *OldState.ToString(), *NewState.ToString());
        OnCombatStateChanged.Broadcast(OldState, NewState);
    }
}

UObject* UCombatStateComponent::FindGameplayTagsComponent() const
{
    const AActor* Owner = GetOwner();
    if (!Owner)
    {
        return nullptr;
    }

    for (UActorComponent* Component : Owner->GetComponents())
    {
        if (Component && Component->GetClass()->GetName() == TEXT("BPC_GameplayTags_C"))
        {
            return Component;
        }
    }

    return nullptr;
}

void UCombatStateComponent::SynchronizeGameplayTags() const
{
    UObject* GameplayTagsComponent = FindGameplayTagsComponent();
    if (!GameplayTagsComponent)
    {
        UE_LOG(LogTemp, Warning, TEXT("CombatStateComponent on %s: BPC_GameplayTags_C not found."),
            GetOwner() ? *GetOwner()->GetName() : TEXT("<NoOwner>"));
        return;
    }

    UFunction* RemoveTagFunction = GameplayTagsComponent->FindFunction(FName(TEXT("RemoveTag")));
    UFunction* AddTagFunction = GameplayTagsComponent->FindFunction(FName(TEXT("AddTag")));
    if (!RemoveTagFunction || !AddTagFunction)
    {
        UE_LOG(LogTemp, Warning, TEXT("CombatStateComponent on %s: BPC_GameplayTags functions are unavailable."),
            GetOwner() ? *GetOwner()->GetName() : TEXT("<NoOwner>"));
        return;
    }

    const TArray<FGameplayTag> CombatTags =
    {
        FGameplayTag::RequestGameplayTag(CombatStateTags::Idle, false),
        FGameplayTag::RequestGameplayTag(CombatStateTags::Attacking, false),
        FGameplayTag::RequestGameplayTag(CombatStateTags::Dodging, false),
        FGameplayTag::RequestGameplayTag(CombatStateTags::Blocking, false),
        FGameplayTag::RequestGameplayTag(CombatStateTags::Parrying, false),
        FGameplayTag::RequestGameplayTag(CombatStateTags::Staggered, false),
        FGameplayTag::RequestGameplayTag(CombatStateTags::Launched, false),
        FGameplayTag::RequestGameplayTag(CombatStateTags::Dead, false)
    };

    auto InvokeGameplayTagFunction = [](UObject* Object, UFunction* Function, const FGameplayTag& Tag)
    {
        if (!Object || !Function)
        {
            return;
        }

        FProperty* TagProperty = Function->FindPropertyByName(FName(TEXT("Tag")));
        if (!TagProperty)
        {
            return;
        }

        TArray<uint8> Parameters;
        Parameters.SetNumZeroed(Function->ParmsSize);
        TagProperty->CopyCompleteValue(
            TagProperty->ContainerPtrToValuePtr<void>(Parameters.GetData()),
            &Tag);
        Object->ProcessEvent(Function, Parameters.GetData());
    };

    for (const FGameplayTag& CombatTag : CombatTags)
    {
        InvokeGameplayTagFunction(GameplayTagsComponent, RemoveTagFunction, CombatTag);
    }

    InvokeGameplayTagFunction(GameplayTagsComponent, AddTagFunction, CurrentState);
}

static FAutoConsoleCommandWithWorldAndArgs GVaenethDebugCombatStateCommand(
    TEXT("vaeneth.DebugCombatState"),
    TEXT("Demande un etat de combat au joueur. Argument: State.Combat.<Etat>"),
    FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
    {
        UCombatStateComponent* CombatState = CombatStateConsole::FindPlayerCombatState(World);
        if (!CombatState || Args.Num() == 0)
        {
            UE_LOG(LogTemp, Warning, TEXT("vaeneth.DebugCombatState : joueur, composant ou argument introuvable."));
            return;
        }

        const FGameplayTag RequestedState = CombatStateConsole::MakeTag(Args[0]);
        const FGameplayTag BeforeState = CombatState->GetCurrentState();
        const bool bAccepted = CombatState->RequestState(RequestedState);
        const FGameplayTag AfterState = CombatState->GetCurrentState();

        UE_LOG(LogTemp, Log, TEXT("vaeneth.DebugCombatState : avant=%s demande=%s accepte=%s apres=%s"),
            *CombatStateConsole::StateToString(BeforeState),
            *CombatStateConsole::StateToString(RequestedState),
            bAccepted ? TEXT("true") : TEXT("false"),
            *CombatStateConsole::StateToString(AfterState));
    }));

static FAutoConsoleCommandWithWorldAndArgs GVaenethDebugCombatForceCommand(
    TEXT("vaeneth.DebugCombatForce"),
    TEXT("Force un etat de combat sur le joueur. Argument: State.Combat.<Etat>"),
    FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
    {
        UCombatStateComponent* CombatState = CombatStateConsole::FindPlayerCombatState(World);
        if (!CombatState || Args.Num() == 0)
        {
            UE_LOG(LogTemp, Warning, TEXT("vaeneth.DebugCombatForce : joueur, composant ou argument introuvable."));
            return;
        }

        const FGameplayTag RequestedState = CombatStateConsole::MakeTag(Args[0]);
        const FGameplayTag BeforeState = CombatState->GetCurrentState();
        CombatState->ForceState(RequestedState);
        const FGameplayTag AfterState = CombatState->GetCurrentState();

        UE_LOG(LogTemp, Log, TEXT("vaeneth.DebugCombatForce : avant=%s demande=%s apres=%s"),
            *CombatStateConsole::StateToString(BeforeState),
            *CombatStateConsole::StateToString(RequestedState),
            *CombatStateConsole::StateToString(AfterState));
    }));
