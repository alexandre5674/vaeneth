// Copyright Vaeneth. All Rights Reserved.

#include "Core/Camera/CameraDirectorComponent.h"

#include "Camera/CameraComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "EventBusSubsystem.h"

UCameraDirectorComponent::UCameraDirectorComponent()
{
    PrimaryComponentTick.bCanEverTick = true;
    SetIsReplicatedByDefault(false);
}

void UCameraDirectorComponent::BeginPlay()
{
    Super::BeginPlay();

    if (AActor* Owner = GetOwner())
    {
        CameraBoom = Owner->FindComponentByClass<USpringArmComponent>();
        Camera = Owner->FindComponentByClass<UCameraComponent>();
    }

    SubscribeToCameraEvents();

    UDA_CameraProfile* InitialProfile = DefaultProfile ? DefaultProfile : ExplorationProfile;
    if (InitialProfile)
    {
        SetProfile(InitialProfile, 0.0f);
    }
}

void UCameraDirectorComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    UnsubscribeFromCameraEvents();
    Super::EndPlay(EndPlayReason);
}

void UCameraDirectorComponent::SetProfile(UDA_CameraProfile* Profile, float BlendTime)
{
    if (!Profile)
    {
        return;
    }

    if (!bHasProfile)
    {
        AppliedProfile = Profile->Profile;
        StartProfile = AppliedProfile;
        bHasProfile = true;
    }
    else
    {
        StartProfile = AppliedProfile;
    }

    TargetProfile = Profile->Profile;
    BlendElapsed = 0.0f;
    BlendDuration = FMath::Max(0.0f, BlendTime);
    bBlending = true;
}

void UCameraDirectorComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

    if (!bHasProfile || !CameraBoom.IsValid() || !Camera.IsValid())
    {
        return;
    }

    const float WorldDeltaSeconds = GetWorld() ? GetWorld()->GetDeltaSeconds() : DeltaTime;
    float Alpha = 1.0f;
    if (bBlending)
    {
        if (BlendDuration <= KINDA_SMALL_NUMBER)
        {
            Alpha = 1.0f;
        }
        else
        {
            BlendElapsed += WorldDeltaSeconds;
            Alpha = FMath::Clamp(BlendElapsed / BlendDuration, 0.0f, 1.0f);
        }
    }

    const float EasedAlpha = FMath::InterpEaseInOut(0.0f, 1.0f, Alpha, 2.0f);
    FCameraProfile BlendedProfile;
    BlendedProfile.ArmLength = FMath::Lerp(StartProfile.ArmLength, TargetProfile.ArmLength, EasedAlpha);
    BlendedProfile.SocketOffset = FMath::Lerp(StartProfile.SocketOffset, TargetProfile.SocketOffset, EasedAlpha);
    BlendedProfile.FOV = FMath::Lerp(StartProfile.FOV, TargetProfile.FOV, EasedAlpha);
    BlendedProfile.LagSpeed = FMath::Lerp(StartProfile.LagSpeed, TargetProfile.LagSpeed, EasedAlpha);
    BlendedProfile.RotationLagSpeed = FMath::Lerp(StartProfile.RotationLagSpeed, TargetProfile.RotationLagSpeed, EasedAlpha);
    BlendedProfile.ShakeScale = FMath::Lerp(StartProfile.ShakeScale, TargetProfile.ShakeScale, EasedAlpha);

    ApplyProfile(BlendedProfile);
    AppliedProfile = BlendedProfile;

    if (Alpha >= 1.0f)
    {
        AppliedProfile = TargetProfile;
        bBlending = false;
    }
}

void UCameraDirectorComponent::SubscribeToCameraEvents()
{
    if (!GetWorld() || !GetWorld()->GetGameInstance())
    {
        return;
    }

    EventBus = GetWorld()->GetGameInstance()->GetSubsystem<UEventBusSubsystem>();
    if (!EventBus.IsValid())
    {
        return;
    }

    SillageStartHandle = EventBus->SubscribeToTagNative(
        FGameplayTag::RequestGameplayTag(FName("Event.Player.SillageStart")),
        FGameEventNativeDelegate::FDelegate::CreateUObject(this, &UCameraDirectorComponent::HandleSillageStart));

    SillageEndHandle = EventBus->SubscribeToTagNative(
        FGameplayTag::RequestGameplayTag(FName("Event.Player.SillageEnd")),
        FGameEventNativeDelegate::FDelegate::CreateUObject(this, &UCameraDirectorComponent::HandleSillageEnd));
}

void UCameraDirectorComponent::UnsubscribeFromCameraEvents()
{
    if (!EventBus.IsValid())
    {
        return;
    }

    if (SillageStartHandle.IsValid())
    {
        EventBus->UnsubscribeNative(SillageStartHandle);
        SillageStartHandle.Reset();
    }

    if (SillageEndHandle.IsValid())
    {
        EventBus->UnsubscribeNative(SillageEndHandle);
        SillageEndHandle.Reset();
    }
}

void UCameraDirectorComponent::HandleSillageStart(const FGameEvent& Event)
{
    SetProfile(SillageProfile, DefaultBlendTime);
}

void UCameraDirectorComponent::HandleSillageEnd(const FGameEvent& Event)
{
    SetProfile(ExplorationProfile ? ExplorationProfile : DefaultProfile, DefaultBlendTime);
}

void UCameraDirectorComponent::ApplyProfile(const FCameraProfile& Profile)
{
    if (USpringArmComponent* SpringArm = CameraBoom.Get())
    {
        SpringArm->TargetArmLength = Profile.ArmLength;
        SpringArm->SocketOffset = Profile.SocketOffset;
        SpringArm->CameraLagSpeed = Profile.LagSpeed;
        SpringArm->CameraRotationLagSpeed = Profile.RotationLagSpeed;
    }

    if (UCameraComponent* CameraComponent = Camera.Get())
    {
        CameraComponent->FieldOfView = Profile.FOV;
    }
}
