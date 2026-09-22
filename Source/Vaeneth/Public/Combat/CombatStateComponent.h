// Copyright Vaeneth. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameplayTagContainer.h"
#include "CombatStateComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FCombatStateChangedSignature, FGameplayTag, OldState, FGameplayTag, NewState);

/**
 * Owns the combat state of its actor and validates every requested transition.
 */
UCLASS(ClassGroup=(Combat), BlueprintType, Blueprintable, meta=(BlueprintSpawnableComponent))
class VAENETH_API UCombatStateComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UCombatStateComponent();

    virtual void BeginPlay() override;

    /** Requests a transition through the combat-state table. */
    UFUNCTION(BlueprintCallable, Category="Combat|State")
    bool RequestState(FGameplayTag NewState);

    /** Forces a valid combat state without consulting the transition table. */
    UFUNCTION(BlueprintCallable, Category="Combat|State")
    void ForceState(FGameplayTag NewState);

    UFUNCTION(BlueprintPure, Category="Combat|State")
    FGameplayTag GetCurrentState() const;

    UFUNCTION(BlueprintPure, Category="Combat|State")
    bool IsInState(FGameplayTag State) const;

    /** Extension point for future attack-cancel windows. False until P1.4. */
    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category="Combat|State")
    bool CanInterruptCurrentState();

    UPROPERTY(BlueprintAssignable, Category="Combat|State")
    FCombatStateChangedSignature OnCombatStateChanged;

protected:
    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="Combat|State")
    FGameplayTag CurrentState;

private:
    bool IsValidCombatState(FGameplayTag State) const;
    bool IsAllowedTransition(FGameplayTag FromState, FGameplayTag ToState);
    void ApplyStateChange(FGameplayTag NewState, bool bBroadcast);
    void SynchronizeGameplayTags() const;
    UObject* FindGameplayTagsComponent() const;
};
