// Copyright 2026 Vaeneth. All Rights Reserved.

#include "InputTranslatorComponent.h"

UInputTranslatorComponent::UInputTranslatorComponent()
{
	SetIsReplicatedByDefault(false);
}

bool UInputTranslatorComponent::EmitIntent(const FGameplayTag IntentTag)
{
	const float Timestamp = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0f;
	OnIntent.Broadcast(IntentTag, Timestamp);
	return true;
}
