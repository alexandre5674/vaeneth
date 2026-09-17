// Copyright 2026 Vaeneth. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameplayTagContainer.h"
#include "InputTranslatorComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FInputTranslatorIntentSignature, FGameplayTag, Tag, float, Timestamp);

/** Translates physical input into gameplay intent tags for the solo combat pipeline. */
UCLASS(Blueprintable, ClassGroup = (Vaeneth), meta = (BlueprintSpawnableComponent))
class VAENETHCORE_API UInputTranslatorComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UInputTranslatorComponent();

	/** Input buffering window in gameplay seconds. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input", meta = (ClampMin = "0.0"))
	float InputBufferWindow = 0.20f;

	/** Broadcast whenever an intent is emitted. Timestamp is gameplay time in seconds. */
	UPROPERTY(BlueprintAssignable, Category = "Input")
	FInputTranslatorIntentSignature OnIntent;

	/** Emit an intent using gameplay time in seconds. */
	UFUNCTION(BlueprintCallable, Category = "Input")
	bool EmitIntent(FGameplayTag IntentTag);
};
