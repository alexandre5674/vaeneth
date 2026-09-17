// Copyright 2026 Vaeneth. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ComponentInstanceDataCache.h"
#include "PersistentIDComponent.generated.h"

struct FPersistentIDComponentInstanceData;

/**
 * Stable identity for actors that participate in save persistence.
 *
 * The GUID is serialized with SaveGame data and is independent of the actor
 * name, array order, and world transform.
 */
UCLASS(Blueprintable, ClassGroup = (Vaeneth), meta = (BlueprintSpawnableComponent))
class VAENETHCORE_API UPersistentIDComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	/** Stable identity used by save-game records. */
	UPROPERTY(VisibleAnywhere, SaveGame, BlueprintReadOnly, Category = "Persistence")
	FGuid PersistentGuid;

	/** Return this component's stable persistent identity. */
	UFUNCTION(BlueprintPure, Category = "Persistence")
	FGuid GetPersistentID() const;

	/** Generate a new identity manually in the editor. */
	UFUNCTION(BlueprintCallable, CallInEditor, Category = "Persistence")
	void RegenerateID();

	virtual void OnComponentCreated() override;
	virtual void PostDuplicate(bool bDuplicateForPIE) override;
	virtual TStructOnScope<FActorComponentInstanceData> GetComponentInstanceData() const override;
	void ApplyComponentInstanceData(struct FPersistentIDComponentInstanceData* ComponentInstanceData);

#if WITH_EDITOR
	/** Covers editor copy/paste and actor duplication paths that skip PostDuplicate on components. */
	virtual void PostEditImport() override;
#endif

private:
	void EnsurePersistentID();
	void RegeneratePersistentID();
};

/** Component instance data used to preserve the GUID during Blueprint reconstruction. */
USTRUCT()
struct FPersistentIDComponentInstanceData : public FActorComponentInstanceData
{
	GENERATED_BODY()

	FPersistentIDComponentInstanceData() = default;
	FPersistentIDComponentInstanceData(const UPersistentIDComponent* SourceComponent)
		: FActorComponentInstanceData(SourceComponent)
		, PersistentGuid(SourceComponent->PersistentGuid)
	{
	}

	virtual ~FPersistentIDComponentInstanceData() = default;

	virtual bool ContainsData() const override
	{
		return true;
	}

	virtual void ApplyToComponent(UActorComponent* Component, const ECacheApplyPhase CacheApplyPhase) override
	{
		Super::ApplyToComponent(Component, CacheApplyPhase);

		if (CacheApplyPhase == ECacheApplyPhase::PostUserConstructionScript)
		{
			CastChecked<UPersistentIDComponent>(Component)->ApplyComponentInstanceData(this);
		}
	}

	UPROPERTY()
	FGuid PersistentGuid;
};
