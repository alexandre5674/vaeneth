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
      ActiveAttacks.Empty();

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
    if (!Attacking.IsValid())
    {
        return false;
    }

    const TWeakObjectPtr<AActor> WeakAttacker(Attacker);

    // Deja en attaque : seul un enchainement de combo est possible, pendant la fenetre
    // d'annulation, et seulement si le coup courant declare une suite.
    const bool bIsComboLink = CombatState->IsInState(Attacking);
    UDA_Attack* AttackData = nullptr;
    UAnimMontage* CurrentMontage = nullptr;

    if (bIsComboLink)
    {
        const FActiveAttack* Active = ActiveAttacks.Find(WeakAttacker);
        UDA_Attack* CurrentData = Active ? Active->AttackData.LoadSynchronous() : nullptr;
        CurrentMontage = Active ? Active->Montage.Get() : nullptr;

        if (!CombatState->IsCancelWindowOpen() || !CurrentData || CurrentData->NextComboAttack.IsNull())
        {
            UE_LOG(LogTemp, Log, TEXT("CombatSubsystem: coup de test refuse pour %s (pas de suite de combo disponible)."), *Attacker->GetName());
            return false;
        }

        AttackData = CurrentData->NextComboAttack.LoadSynchronous();
    }
    else
    {
        AttackData = GetTestLightAttackData();
    }

    UAnimMontage* Montage = AttackData ? AttackData->Montage.LoadSynchronous() : nullptr;
    if (!Montage)
    {
        UE_LOG(LogTemp, Warning, TEXT("CombatSubsystem: montage introuvable pour %s."), AttackData ? *AttackData->GetName() : TEXT("<aucune donnee>"));
        return false;
    }

    // Le suivi se fait par montage : un coup qui s'enchaine sur lui-meme serait pris
    // pour la fin de l'attaque quand l'ancienne instance se termine.
    if (bIsComboLink && Montage == CurrentMontage)
    {
        UE_LOG(LogTemp, Warning, TEXT("CombatSubsystem: %s s'enchaine sur le meme montage, refuse."), *AttackData->GetName());
        return false;
    }

    if (!bIsComboLink && !CombatState->RequestState(Attacking))
    {
        UE_LOG(LogTemp, Log, TEXT("CombatSubsystem: coup de test refuse pour %s."), *Attacker->GetName());
        return false;
    }

    // La fenetre du coup precedent est consommee ; le nouveau montage ouvrira la sienne.
    CombatState->SetCancelWindowOpen(false);

    // Enregistre AVANT Montage_Play : la fin du montage interrompu doit trouver le nouveau
    // montage comme attaque active et etre ignoree.
    FActiveAttack& Entry = ActiveAttacks.FindOrAdd(WeakAttacker);
    Entry.Montage = Montage;
    Entry.AnimInstance = AnimInstance;
    Entry.AttackData = AttackData;
    AnimInstance->OnMontageEnded.AddUniqueDynamic(this, &UCombatSubsystem::HandleTestLightAttackMontageEnded);

    const float PlayedLength = AnimInstance->Montage_Play(Montage, 1.0f);
    if (PlayedLength <= 0.0f)
    {
        FinishTestLightAttack(WeakAttacker);
        UE_LOG(LogTemp, Warning, TEXT("CombatSubsystem: lecture du montage refusee pour %s."), *Attacker->GetName());
        return false;
    }

    UE_LOG(LogTemp, Log, TEXT("CombatSubsystem: %s %s pour %s, duree %.3f s."),
        bIsComboLink ? TEXT("enchainement") : TEXT("coup"),
        *AttackData->GetName(), *Attacker->GetName(), PlayedLength);
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

UDA_Attack* UCombatSubsystem::GetActiveAttackData(AActor* Attacker) const
{
    if (const FActiveAttack* Active = ActiveAttacks.Find(TWeakObjectPtr<AActor>(Attacker)))
    {
        if (UDA_Attack* AttackData = Active->AttackData.LoadSynchronous())
        {
            return AttackData;
        }
    }

    return GetTestLightAttackData();
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

void UCombatSubsystem::FinishTestLightAttack(const TWeakObjectPtr<AActor> Attacker)
{
    // Nettoyage AVANT le passage en Idle : le changement d'etat est diffuse et peut
    // relancer une attaque (entree bufferisee) qu'il ne faut pas effacer ensuite.
    if (const FActiveAttack* Entry = ActiveAttacks.Find(Attacker))
    {
        if (UAnimInstance* AnimInstance = Entry->AnimInstance.Get())
        {
            AnimInstance->OnMontageEnded.RemoveDynamic(this, &UCombatSubsystem::HandleTestLightAttackMontageEnded);
        }
    }

    ActiveAttacks.Remove(Attacker);

    if (AActor* ValidAttacker = Attacker.Get())
    {
        if (UCombatStateComponent* CombatState = ValidAttacker->FindComponentByClass<UCombatStateComponent>())
        {
            // Ne retombe en Idle que si l'attaque est encore l'etat courant. Un montage
            // interrompu par une annulation (esquive) termine son blend-out APRES que le
            // nouvel etat a ete pose : sans ce garde, l'attaque annulee ecraserait Dodging.
            const FGameplayTag Attacking = FGameplayTag::RequestGameplayTag(CombatSubsystemTags::Attacking, false);
            if (CombatState->IsInState(Attacking))
            {
                CombatState->RequestState(FGameplayTag::RequestGameplayTag(FName(TEXT("State.Combat.Idle")), false));
            }
        }
    }
}

void UCombatSubsystem::HandleTestLightAttackMontageEnded(UAnimMontage* Montage, const bool bInterrupted)
{
    if (!Montage)
    {
        return;
    }

    // Seule la fin du montage ACTIF termine l'attaque. Le montage d'un coup remplace par
    // la suite du combo se termine aussi (interrompu) : il est ignore.
    TWeakObjectPtr<AActor> Attacker;
    bool bFound = false;
    for (const TPair<TWeakObjectPtr<AActor>, FActiveAttack>& Pair : ActiveAttacks)
    {
        if (Pair.Value.Montage.Get() == Montage)
        {
            Attacker = Pair.Key;
            bFound = true;
            break;
        }
    }

    if (!bFound)
    {
        UE_LOG(LogTemp, Log, TEXT("CombatSubsystem: montage %s remplace par la suite du combo, fin ignoree."), *Montage->GetName());
        return;
    }

    UE_LOG(LogTemp, Log, TEXT("CombatSubsystem: montage %s termine pour %s, interrompu=%s."),
        *Montage->GetName(),
        Attacker.IsValid() ? *Attacker->GetName() : TEXT("<invalid>"),
        bInterrupted ? TEXT("true") : TEXT("false"));
    FinishTestLightAttack(Attacker);
}
