// Copyright Vaeneth. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "CombatSubsystem.generated.h"

    class AActor;
    class UDA_Attack;
    class UAnimInstance;
    class UAnimMontage;

UCLASS(BlueprintType)
class VAENETH_API UCombatSubsystem : public UWorldSubsystem
{
    GENERATED_BODY()

public:
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;

    /** Applies actor-local hit stop without changing global time dilation. */
    UFUNCTION(BlueprintCallable, Category="Combat|HitStop")
    void ApplyHitStop(const TArray<AActor*>& Actors, float Duration);

    /** Reads the hit-stop duration directly from C_HitStopByDamage. */
    UFUNCTION(BlueprintPure, Category="Combat|HitStop")
    float GetHitStopDurationForDamage(float Damage) const;

    /** Starts the P1.2 light test attack if the attacker can enter Attacking. */
    UFUNCTION(BlueprintCallable, Category="Combat|TestAttack")
    bool BeginTestLightAttack(AActor* Attacker);

    /** Loads the P1.2 light attack data asset. */
      UFUNCTION(BlueprintPure, Category="Combat|TestAttack")
      UDA_Attack* GetTestLightAttackData() const;

        /** Configures all tracks of an animation montage to use the requested slot. */
        UFUNCTION(BlueprintCallable, Category="Combat|Animation")
        static bool SetAnimMontageSlot(UAnimMontage* Montage, FName SlotName);

  private:
    struct FHitStopEntry
    {
        float EndTime = 0.0f;
        FTimerHandle TimerHandle;
    };

        TMap<TWeakObjectPtr<AActor>, FHitStopEntry> HitStopEntries;
        TMap<TWeakObjectPtr<UAnimMontage>, TWeakObjectPtr<AActor>> AttackMontageOwners;
        TMap<TWeakObjectPtr<UAnimMontage>, TWeakObjectPtr<UAnimInstance>> AttackMontageInstances;

        void RestoreHitStop(TWeakObjectPtr<AActor> Actor);
        void FinishTestLightAttack(TWeakObjectPtr<AActor> Attacker, UAnimMontage* Montage = nullptr);

        UFUNCTION()
        void HandleTestLightAttackMontageEnded(UAnimMontage* Montage, bool bInterrupted);
  };
