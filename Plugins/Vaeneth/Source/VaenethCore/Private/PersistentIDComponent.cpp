// Copyright 2026 Vaeneth. All Rights Reserved.

#include "PersistentIDComponent.h"

FGuid UPersistentIDComponent::GetPersistentID() const
{
	return PersistentGuid;
}

void UPersistentIDComponent::OnComponentCreated()
{
	Super::OnComponentCreated();
	EnsurePersistentID();
}

void UPersistentIDComponent::PostDuplicate(bool bDuplicateForPIE)
{
	Super::PostDuplicate(bDuplicateForPIE);

	if (!PersistentGuid.IsValid())
	{
		RegeneratePersistentID();
	}
}

TStructOnScope<FActorComponentInstanceData> UPersistentIDComponent::GetComponentInstanceData() const
{
	return MakeStructOnScope<FActorComponentInstanceData, FPersistentIDComponentInstanceData>(this);
}

void UPersistentIDComponent::ApplyComponentInstanceData(FPersistentIDComponentInstanceData* ComponentInstanceData)
{
	if (ComponentInstanceData && ComponentInstanceData->PersistentGuid.IsValid())
	{
		PersistentGuid = ComponentInstanceData->PersistentGuid;
	}
}

#if WITH_EDITOR
void UPersistentIDComponent::PostEditImport()
{
	Super::PostEditImport();
	EnsurePersistentID();
}
#endif

void UPersistentIDComponent::EnsurePersistentID()
{
	if (!PersistentGuid.IsValid())
	{
		RegeneratePersistentID();
	}
}

void UPersistentIDComponent::RegenerateID()
{
	RegeneratePersistentID();
}

void UPersistentIDComponent::RegeneratePersistentID()
{
#if WITH_EDITOR
	Modify();
#endif

	PersistentGuid = FGuid::NewGuid();

#if WITH_EDITOR
	MarkPackageDirty();
#endif
}
