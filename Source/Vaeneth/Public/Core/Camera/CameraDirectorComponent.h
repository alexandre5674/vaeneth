// Copyright Vaeneth. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Core/Camera/CameraProfile.h"
#include "CameraDirectorComponent.generated.h"

class UCameraComponent;
class UEventBusSubsystem;
class USpringArmComponent;
struct FGameEvent;

UCLASS(Blueprintable, ClassGroup=(Camera), meta=(BlueprintSpawnableComponent))
class VAENETH_API UCameraDirectorComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UCameraDirectorComponent();

    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

    UFUNCTION(BlueprintCallable, Category="Vaeneth|Camera")
    void SetProfile(UDA_CameraProfile* Profile, float BlendTime = 0.4f);

    UFUNCTION(BlueprintPure, Category="Vaeneth|Camera")
    FCameraProfile GetCurrentProfile() const { return AppliedProfile; }

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Camera|Profiles")
    TObjectPtr<UDA_CameraProfile> DefaultProfile;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Camera|Profiles")
    TObjectPtr<UDA_CameraProfile> ExplorationProfile;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Camera|Profiles")
    TObjectPtr<UDA_CameraProfile> SillageProfile;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Camera|Profiles", meta=(ClampMin="0.0"))
    float DefaultBlendTime = 0.4f;

private:
    void SubscribeToCameraEvents();
    void UnsubscribeFromCameraEvents();
    void HandleSillageStart(const FGameEvent& Event);
    void HandleSillageEnd(const FGameEvent& Event);
    void ApplyProfile(const FCameraProfile& Profile);

    TWeakObjectPtr<USpringArmComponent> CameraBoom;
    TWeakObjectPtr<UCameraComponent> Camera;
    TWeakObjectPtr<UEventBusSubsystem> EventBus;
    FDelegateHandle SillageStartHandle;
    FDelegateHandle SillageEndHandle;

    FCameraProfile AppliedProfile;
    FCameraProfile StartProfile;
    FCameraProfile TargetProfile;
    float BlendElapsed = 0.0f;
    float BlendDuration = 0.0f;
    bool bHasProfile = false;
    bool bBlending = false;
};
