// Copyright Vaeneth. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "EventBusSubsystem.generated.h"

USTRUCT(BlueprintType)
struct VAENETH_API FGameEvent
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Event")
    FGameplayTag EventTag;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Event")
    TObjectPtr<AActor> Instigator = nullptr;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Event")
    TObjectPtr<AActor> Target = nullptr;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Event")
    float Magnitude = 0.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Event")
    FVector Location = FVector::ZeroVector;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Event")
    TMap<FName, float> Payload;
};

DECLARE_DYNAMIC_DELEGATE_OneParam(FGameEventDynamicDelegate, FGameEvent, Event);
DECLARE_MULTICAST_DELEGATE_OneParam(FGameEventNativeDelegate, const FGameEvent&);

UCLASS(BlueprintType)
class VAENETH_API UEventBusSubsystem : public UGameInstanceSubsystem
{
    GENERATED_BODY()

public:
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;

    UFUNCTION(BlueprintCallable, Category="Vaeneth|Events")
    void BroadcastEvent(const FGameEvent& Event);

    // Blueprint uses an int64 token because FDelegateHandle is not Blueprint-reflectable.
    UFUNCTION(BlueprintCallable, Category="Vaeneth|Events")
    int64 SubscribeToTag(FGameplayTag Tag, FGameEventDynamicDelegate Delegate);

    UFUNCTION(BlueprintCallable, Category="Vaeneth|Events")
    void Unsubscribe(int64 Handle);

    UFUNCTION(BlueprintCallable, Category="Vaeneth|Events|Tests")
    bool RunPerformanceTest(int32 EventCount, float& ElapsedMilliseconds);

    FDelegateHandle SubscribeToTagNative(FGameplayTag Tag, FGameEventNativeDelegate::FDelegate Delegate);
    void UnsubscribeNative(FDelegateHandle Handle);

private:
    struct FDynamicSubscription
    {
        int64 Handle = 0;
        FGameplayTag Tag;
        FGameEventDynamicDelegate Delegate;
    };

    TArray<FDynamicSubscription> DynamicSubscriptions;
    TMap<FDelegateHandle, FGameplayTag> NativeSubscriptionTags;
    TMap<FGameplayTag, FGameEventNativeDelegate> NativeDelegates;
    int64 NextDynamicHandle = 1;
};
