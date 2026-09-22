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

	// Copie hors PIE : nouvel ID. Duplication du monde par le PIE : on garde l'ID.
	if (!bDuplicateForPIE)
	{
		RegeneratePersistentID();
	}

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
	// Coller ou dupliquer (Ctrl+D) dans l'editeur : l'ID copie depuis l'original doit etre remplace.
	RegeneratePersistentID();
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
