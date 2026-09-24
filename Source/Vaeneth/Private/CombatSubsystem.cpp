// Copyright Vaeneth. All Rights Reserved.

#include "CombatSubsystem.h"

#include "Combat/CombatStateComponent.h"
#include "Combat/Data/DA_Attack.h"
#include "Curves/CurveFloat.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "TimerManager.h"

namespace CombatSubsystemPaths
{
    static const TCHAR* HitStopCurve = TEXT("/Game/Vaeneth/Data/Curves/C_HitStopByDamage.C_HitStopByDamage");
    static const TCHAR* TestLightAttack = TEXT("/Game/Vaeneth/Combat/Data/Attacks/DA_Attack_Light01.DA_Attack_Light01");
}

namespace CombatSubsystemTags
{
    static const FName Attacking(TEXT("State.Combat.Attacking"));
}

void UCombatSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
    UE_LOG(LogTemp, Log, TEXT("CombatSubsystem initialized"));
}

void UCombatSubsystem::Deinitialize()
{
    if (UWorld* World = GetWorld())
    {
        for (TPair<TWeakObjectPtr<AActor>, FHitStopEntry>& Pair : HitStopEntries)
        {
            World->GetTimerManager().ClearTimer(Pair.Value.TimerHandle);
            if (AActor* Actor = Pair.Key.Get())
            {
                Actor->CustomTimeDilation = 1.0f;
            }
        }

    }

      HitStopEntries.Empty();
      AttackMontageOwners.Empty();
      AttackMontageInstances.Empty();

    UE_LOG(LogTemp, Log, TEXT("CombatSubsystem deinitialized"));
    Super::Deinitialize();
}

void UCombatSubsystem::ApplyHitStop(const TArray<AActor*>& Actors, const float Duration)
{
    UWorld* World = GetWorld();
    if (!World || Duration <= 0.0f)
    {
        return;
    }

    const float Now = World->GetTimeSeconds();
    const float RequestedEndTime = Now + Duration;

    for (AActor* Actor : Actors)
    {
        if (!IsValid(Actor))
        {
            continue;
        }

        const TWeakObjectPtr<AActor> WeakActor(Actor);
        FHitStopEntry& Entry = HitStopEntries.FindOrAdd(WeakActor);
        Entry.EndTime = FMath::Max(Entry.EndTime, RequestedEndTime);

        Actor->CustomTimeDilation = 0.02f;

        World->GetTimerManager().ClearTimer(Entry.TimerHandle);
        const float Remaining = FMath::Max(0.001f, Entry.EndTime - Now);
        World->GetTimerManager().SetTimer(
            Entry.TimerHandle,
            FTimerDelegate::CreateUObject(this, &UCombatSubsystem::RestoreHitStop, WeakActor),
            Remaining,
            false);
    }
}

float UCombatSubsystem::GetHitStopDurationForDamage(const float Damage) const
{
    UCurveFloat* Curve = LoadObject<UCurveFloat>(nullptr, CombatSubsystemPaths::HitStopCurve);
    if (!Curve)
    {
        UE_LOG(LogTemp, Warning, TEXT("CombatSubsystem: curve C_HitStopByDamage introuvable; repli a 0.05 s."));
        return 0.05f;
    }

    return FMath::Max(0.0f, Curve->GetFloatValue(Damage));
}

bool UCombatSubsystem::BeginTestLightAttack(AActor* Attacker)
{
    if (!IsValid(Attacker))
    {
        return false;
    }

    UDA_Attack* AttackData = GetTestLightAttackData();
    UAnimMontage* Montage = AttackData ? AttackData->Montage.LoadSynchronous() : nullptr;
    if (!Montage)
    {
        UE_LOG(LogTemp, Warning, TEXT("CombatSubsystem: montage du coup de test introuvable."));
        return false;
    }

    USkeletalMeshComponent* Mesh = Attacker->FindComponentByClass<USkeletalMeshComponent>();
    UAnimInstance* AnimInstance = Mesh ? Mesh->GetAnimInstance() : nullptr;
    if (!AnimInstance)
    {
        UE_LOG(LogTemp, Warning, TEXT("CombatSubsystem: AnimInstance absente sur %s."), *Attacker->GetName());
        return false;
    }

    UCombatStateComponent* CombatState = Attacker->FindComponentByClass<UCombatStateComponent>();
    if (!CombatState)
    {
        UE_LOG(LogTemp, Warning, TEXT("CombatSubsystem: CombatState absent sur %s."), *Attacker->GetName());
        return false;
    }

    const FGameplayTag Attacking = FGameplayTag::RequestGameplayTag(CombatSubsystemTags::Attacking, false);
    if (!Attacking.IsValid() || CombatState->IsInState(Attacking) || !CombatState->RequestState(Attacking))
    {
        UE_LOG(LogTemp, Log, TEXT("CombatSubsystem: coup de test refuse pour %s."), *Attacker->GetName());
        return false;
    }

      const TWeakObjectPtr<AActor> WeakAttacker(Attacker);

      AttackMontageOwners.Add(Montage, WeakAttacker);
      AttackMontageInstances.Add(Montage, AnimInstance);
      AnimInstance->OnMontageEnded.AddDynamic(this, &UCombatSubsystem::HandleTestLightAttackMontageEnded);

      const float PlayedLength = AnimInstance->Montage_Play(Montage, 1.0f);
      if (PlayedLength <= 0.0f)
      {
          FinishTestLightAttack(WeakAttacker, Montage);
          UE_LOG(LogTemp, Warning, TEXT("CombatSubsystem: lecture du montage refusee pour %s."), *Attacker->GetName());
          return false;
      }

    UE_LOG(LogTemp, Log, TEXT("CombatSubsystem: coup de test accepte pour %s, duree %.3f s."), *Attacker->GetName(), PlayedLength);
    return true;
}

UDA_Attack* UCombatSubsystem::GetTestLightAttackData() const
{
    UDA_Attack* AttackData = LoadObject<UDA_Attack>(nullptr, CombatSubsystemPaths::TestLightAttack);
    if (!AttackData)
    {
        UE_LOG(LogTemp, Warning, TEXT("CombatSubsystem: DA_Attack_Light01 introuvable."));
    }

    return AttackData;
}

bool UCombatSubsystem::SetAnimMontageSlot(UAnimMontage* Montage, const FName SlotName)
{
    if (!Montage || SlotName.IsNone() || Montage->SlotAnimTracks.Num() == 0)
    {
        return false;
    }

    bool bChanged = false;
    Montage->Modify();

    for (FSlotAnimationTrack& Track : Montage->SlotAnimTracks)
    {
        if (Track.SlotName != SlotName)
        {
            Track.SlotName = SlotName;
            bChanged = true;
        }
    }

    if (bChanged)
    {
        Montage->MarkPackageDirty();
    }

    return true;
}

void UCombatSubsystem::RestoreHitStop(const TWeakObjectPtr<AActor> Actor)
{
    UWorld* World = GetWorld();
    if (!World)
    {
        return;
    }

    FHitStopEntry* Entry = HitStopEntries.Find(Actor);
    if (!Entry)
    {
        return;
    }

    const float Remaining = Entry->EndTime - World->GetTimeSeconds();
    if (Remaining > KINDA_SMALL_NUMBER)
    {
        World->GetTimerManager().SetTimer(
            Entry->TimerHandle,
            FTimerDelegate::CreateUObject(this, &UCombatSubsystem::RestoreHitStop, Actor),
            Remaining,
            false);
        return;
    }

    if (AActor* ValidActor = Actor.Get())
    {
        ValidActor->CustomTimeDilation = 1.0f;
    }

    HitStopEntries.Remove(Actor);
}

  void UCombatSubsystem::FinishTestLightAttack(const TWeakObjectPtr<AActor> Attacker, UAnimMontage* Montage)
{
    if (AActor* ValidAttacker = Attacker.Get())
    {
        if (UCombatStateComponent* CombatState = ValidAttacker->FindComponentByClass<UCombatStateComponent>())
        {
            CombatState->RequestState(FGameplayTag::RequestGameplayTag(FName(TEXT("State.Combat.Idle")), false));
        }
    }

      for (auto It = AttackMontageOwners.CreateIterator(); It; ++It)
      {
          if (It.Value() == Attacker)
          {
              if (UAnimInstance* AnimInstance = AttackMontageInstances.FindRef(It.Key()).Get())
              {
                  AnimInstance->OnMontageEnded.RemoveDynamic(this, &UCombatSubsystem::HandleTestLightAttackMontageEnded);
              }

              AttackMontageInstances.Remove(It.Key());
              It.RemoveCurrent();
          }
      }

      if (Montage)
      {
          AttackMontageInstances.Remove(Montage);
      }
}

void UCombatSubsystem::HandleTestLightAttackMontageEnded(UAnimMontage* Montage, const bool bInterrupted)
{
    if (!Montage)
    {
        return;
    }

      const TWeakObjectPtr<AActor>* Attacker = AttackMontageOwners.Find(Montage);
      if (Attacker)
      {
          UE_LOG(LogTemp, Log, TEXT("CombatSubsystem: montage de test termine pour %s, interrompu=%s."),
              Attacker->IsValid() ? *Attacker->Get()->GetName() : TEXT("<invalid>"),
              bInterrupted ? TEXT("true") : TEXT("false"));
          FinishTestLightAttack(*Attacker, Montage);
      }
}
