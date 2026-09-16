// Copyright Vaeneth. All Rights Reserved.

#include "EventBusSubsystem.h"

void UEventBusSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
    UE_LOG(LogTemp, Log, TEXT("EventBusSubsystem initialized"));
}

void UEventBusSubsystem::Deinitialize()
{
    DynamicSubscriptions.Reset();
    NativeSubscriptionTags.Reset();
    NativeDelegates.Reset();
    UE_LOG(LogTemp, Log, TEXT("EventBusSubsystem deinitialized"));
    Super::Deinitialize();
}

void UEventBusSubsystem::BroadcastEvent(const FGameEvent& Event)
{
    for (int32 Index = DynamicSubscriptions.Num() - 1; Index >= 0; --Index)
    {
        FDynamicSubscription& Subscription = DynamicSubscriptions[Index];
        if (!Subscription.Delegate.IsBound())
        {
            DynamicSubscriptions.RemoveAtSwap(Index);
            continue;
        }

        if (Subscription.Tag == Event.EventTag)
        {
            Subscription.Delegate.Execute(Event);
        }
    }

    if (FGameEventNativeDelegate* NativeDelegate = NativeDelegates.Find(Event.EventTag))
    {
        NativeDelegate->Broadcast(Event);
    }
}

int64 UEventBusSubsystem::SubscribeToTag(FGameplayTag Tag, FGameEventDynamicDelegate Delegate)
{
    if (!Delegate.IsBound())
    {
        return 0;
    }

    FDynamicSubscription& Subscription = DynamicSubscriptions.AddDefaulted_GetRef();
    Subscription.Handle = NextDynamicHandle++;
    Subscription.Tag = Tag;
    Subscription.Delegate = MoveTemp(Delegate);
    return Subscription.Handle;
}

void UEventBusSubsystem::Unsubscribe(int64 Handle)
{
    DynamicSubscriptions.RemoveAll([Handle](const FDynamicSubscription& Subscription)
    {
        return Subscription.Handle == Handle;
    });
}

bool UEventBusSubsystem::RunPerformanceTest(int32 EventCount, float& ElapsedMilliseconds)
{
    if (EventCount <= 0)
    {
        ElapsedMilliseconds = 0.0f;
        return false;
    }
    FGameEvent TestEvent;
    for (const FDynamicSubscription& Subscription : DynamicSubscriptions)
    {
        if (Subscription.Tag.IsValid())
        {
            TestEvent.EventTag = Subscription.Tag;
            break;
        }
    }

    const double StartSeconds = FPlatformTime::Seconds();
    for (int32 Index = 0; Index < EventCount; ++Index)
    {
        BroadcastEvent(TestEvent);
    }

    ElapsedMilliseconds = static_cast<float>((FPlatformTime::Seconds() - StartSeconds) * 1000.0);
    UE_LOG(LogTemp, Log, TEXT("EventBus performance test: %d events in %.4f ms"), EventCount, ElapsedMilliseconds);
    return true;
}

FDelegateHandle UEventBusSubsystem::SubscribeToTagNative(FGameplayTag Tag, FGameEventNativeDelegate::FDelegate Delegate)
{
    FDelegateHandle Handle = NativeDelegates.FindOrAdd(Tag).Add(Delegate);
    NativeSubscriptionTags.Add(Handle, Tag);
    return Handle;
}

void UEventBusSubsystem::UnsubscribeNative(FDelegateHandle Handle)
{
    if (FGameplayTag* Tag = NativeSubscriptionTags.Find(Handle))
    {
        if (FGameEventNativeDelegate* Delegate = NativeDelegates.Find(*Tag))
        {
            Delegate->Remove(Handle);
        }

        NativeSubscriptionTags.Remove(Handle);
    }
}
