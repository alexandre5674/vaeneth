// Copyright Vaeneth. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"
#include "Animation/AnimMontage.h"
#include "Camera/CameraShakeBase.h"
#include "NiagaraSystem.h"
#include "Sound/SoundBase.h"
#include "DA_Attack.generated.h"

UCLASS(BlueprintType)
class VAENETH_API UDA_Attack : public UPrimaryDataAsset
{
    GENERATED_BODY()

public:
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Attack")
    FGameplayTag AttackTag;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Attack")
    TSoftObjectPtr<UAnimMontage> Montage;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Attack")
    float BaseDamage = 0.0f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Attack")
    float PoiseDamage = 0.0f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Attack")
    FGameplayTag DamageType;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Attack")
    FGameplayTag ReactionType;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Attack")
    float EtherCost = 0.0f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Attack")
    FGameplayTagContainer CanCancelInto;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Attack")
    float HitStopDuration = 0.0f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Attack")
    TSoftClassPtr<UCameraShakeBase> CameraShake;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Attack")
    TSoftObjectPtr<UNiagaraSystem> ImpactVFX;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Attack")
    TSoftObjectPtr<USoundBase> ImpactSFX;
};
