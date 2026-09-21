// Copyright Vaeneth. All Rights Reserved.

#include "Save/SaveSubsystem.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/World.h"

USG_VaenethSave::USG_VaenethSave()
	: SaveVersion(CurrentSaveVersion)
	, Timestamp(FDateTime::Now())
	, PlayTime(0.0f)
	, LevelName(NAME_None)
{
}

void USaveSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	UE_LOG(LogTemp, Log, TEXT("SaveSubsystem initialized. Current Save Version: %d"), USG_VaenethSave::CurrentSaveVersion);
}

void USaveSubsystem::Deinitialize()
{
	RegisteredSaveables.Empty();
	Super::Deinitialize();
}

void USaveSubsystem::RegisterSaveable(UObject* Object)
{
	if (!IsValid(Object) || !Object->GetClass()->ImplementsInterface(UBPI_Saveable::StaticClass()))
	{
		UE_LOG(LogTemp, Warning, TEXT("SaveSubsystem: impossible d'enregistrer un objet non valide ou sans BPI_Saveable."));
		return;
	}

	RegisteredSaveables.AddUnique(Object);
	UE_LOG(LogTemp, Log, TEXT("SaveSubsystem: objet sauvegardable enregistré: %s"), *Object->GetName());
}

void USaveSubsystem::UnregisterSaveable(UObject* Object)
{
	RegisteredSaveables.RemoveAll([Object](const TWeakObjectPtr<UObject>& RegisteredObject)
	{
		return !RegisteredObject.IsValid() || RegisteredObject.Get() == Object;
	});
}

void USaveSubsystem::SaveGameAsync(const FString& SlotName)
{
	if (SlotName.IsEmpty())
	{
		UE_LOG(LogTemp, Warning, TEXT("SaveSubsystem::SaveGameAsync: SlotName est vide."));
		OnSaveCompleted.Broadcast(SlotName, false);
		return;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		UE_LOG(LogTemp, Warning, TEXT("SaveSubsystem::SaveGameAsync: World context invalide."));
		OnSaveCompleted.Broadcast(SlotName, false);
		return;
	}

	USG_VaenethSave* SaveObject = Cast<USG_VaenethSave>(UGameplayStatics::CreateSaveGameObject(USG_VaenethSave::StaticClass()));
	if (!SaveObject)
	{
		UE_LOG(LogTemp, Error, TEXT("SaveSubsystem::SaveGameAsync: Échec de création de USG_VaenethSave."));
		OnSaveCompleted.Broadcast(SlotName, false);
		return;
	}

	SaveObject->SaveVersion = USG_VaenethSave::CurrentSaveVersion;
	SaveObject->Timestamp = FDateTime::Now();
	SaveObject->LevelName = FName(*World->GetMapName());
	SaveObject->PlayTime = UGameplayStatics::GetRealTimeSeconds(World);

	// Récolte universelle de données auprès des acteurs et UObjects enregistrés implémentant BPI_Saveable.
	TArray<AActor*> SaveableActors;
	UGameplayStatics::GetAllActorsWithInterface(World, UBPI_Saveable::StaticClass(), SaveableActors);
	TArray<UObject*> SaveableObjects;
	SaveableObjects.Reserve(SaveableActors.Num() + RegisteredSaveables.Num());

	for (AActor* Actor : SaveableActors)
	{
		if (!IsValid(Actor))
		{
			continue;
		}

		SaveableObjects.AddUnique(Actor);
	}

	for (const TWeakObjectPtr<UObject>& RegisteredObject : RegisteredSaveables)
	{
		if (RegisteredObject.IsValid() && RegisteredObject->GetClass()->ImplementsInterface(UBPI_Saveable::StaticClass()))
		{
			SaveableObjects.AddUnique(RegisteredObject.Get());
		}
	}

	for (UObject* SaveableObject : SaveableObjects)
	{
		if (!IsValid(SaveableObject))
		{
			continue;
		}

		const FGuid Identifier = IBPI_Saveable::Execute_GetSaveIdentifier(SaveableObject);
		if (!Identifier.IsValid())
		{
			UE_LOG(LogTemp, Warning, TEXT("SaveSubsystem: l'objet '%s' implémente BPI_Saveable mais retourne un GUID invalide. Ignoré."), *SaveableObject->GetName());
			continue;
		}

		const FSaveBlob Blob = IBPI_Saveable::Execute_GatherSaveData(SaveableObject);
		SaveObject->Blobs.Add(Identifier, Blob);
	}

	UE_LOG(LogTemp, Log, TEXT("SaveSubsystem: Préparation de la sauvegarde avec %d blobs pour le slot '%s' (Version %d)."),
		SaveObject->Blobs.Num(), *SlotName, SaveObject->SaveVersion);

	FAsyncSaveGameToSlotDelegate SavedDelegate;
	SavedDelegate.BindUObject(this, &USaveSubsystem::OnAsyncSaveFinished);
	UGameplayStatics::AsyncSaveGameToSlot(SaveObject, SlotName, 0, SavedDelegate);
}

void USaveSubsystem::OnAsyncSaveFinished(const FString& SlotName, const int32 UserIndex, bool bSuccess)
{
	if (bSuccess)
	{
		UE_LOG(LogTemp, Log, TEXT("SaveSubsystem: Sauvegarde réussie dans le slot '%s'."), *SlotName);
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("SaveSubsystem: Échec de la sauvegarde dans le slot '%s'."), *SlotName);
	}

	OnSaveCompleted.Broadcast(SlotName, bSuccess);
}

void USaveSubsystem::LoadGameAsync(const FString& SlotName)
{
	if (SlotName.IsEmpty())
	{
		UE_LOG(LogTemp, Warning, TEXT("SaveSubsystem::LoadGameAsync: SlotName est vide."));
		OnLoadCompleted.Broadcast(SlotName, false);
		return;
	}

	if (!UGameplayStatics::DoesSaveGameExist(SlotName, 0))
	{
		UE_LOG(LogTemp, Warning, TEXT("SaveSubsystem: Le slot de sauvegarde '%s' n'existe pas."), *SlotName);
		OnLoadCompleted.Broadcast(SlotName, false);
		return;
	}

	FAsyncLoadGameFromSlotDelegate LoadedDelegate;
	LoadedDelegate.BindUObject(this, &USaveSubsystem::OnAsyncLoadFinished);
	UGameplayStatics::AsyncLoadGameFromSlot(SlotName, 0, LoadedDelegate);
}

void USaveSubsystem::OnAsyncLoadFinished(const FString& SlotName, const int32 UserIndex, USaveGame* LoadedData)
{
	if (!LoadedData)
	{
		UE_LOG(LogTemp, Error, TEXT("SaveSubsystem: Données de sauvegarde nulles pour le slot '%s'."), *SlotName);
		OnLoadCompleted.Broadcast(SlotName, false);
		return;
	}

	USG_VaenethSave* SaveData = Cast<USG_VaenethSave>(LoadedData);
	if (!SaveData)
	{
		UE_LOG(LogTemp, Error, TEXT("SaveSubsystem: Le fichier de sauvegarde '%s' n'est pas de type USG_VaenethSave."), *SlotName);
		OnLoadCompleted.Broadcast(SlotName, false);
		return;
	}

	// Gestion stricte du versionnement
	if (SaveData->SaveVersion > USG_VaenethSave::CurrentSaveVersion)
	{
		UE_LOG(LogTemp, Error, TEXT("SaveSubsystem: Version de sauvegarde incompatible %d (Version actuelle: %d) dans le slot '%s'. Chargement refusé proprement."),
			SaveData->SaveVersion, USG_VaenethSave::CurrentSaveVersion, *SlotName);
		OnLoadCompleted.Broadcast(SlotName, false);
		return;
	}

	if (SaveData->SaveVersion < USG_VaenethSave::CurrentSaveVersion)
	{
		UE_LOG(LogTemp, Warning, TEXT("SaveSubsystem: Version de sauvegarde antérieure %d (Version actuelle: %d) dans le slot '%s'."),
			SaveData->SaveVersion, USG_VaenethSave::CurrentSaveVersion, *SlotName);
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		UE_LOG(LogTemp, Error, TEXT("SaveSubsystem: World context invalide lors du chargement."));
		OnLoadCompleted.Broadcast(SlotName, false);
		return;
	}

	// Application universelle des données aux acteurs et UObjects enregistrés via GUID.
	TArray<AActor*> SaveableActors;
	UGameplayStatics::GetAllActorsWithInterface(World, UBPI_Saveable::StaticClass(), SaveableActors);
	TArray<UObject*> SaveableObjects;
	SaveableObjects.Reserve(SaveableActors.Num() + RegisteredSaveables.Num());
	for (AActor* Actor : SaveableActors)
	{
		if (IsValid(Actor))
		{
			SaveableObjects.AddUnique(Actor);
		}
	}
	for (const TWeakObjectPtr<UObject>& RegisteredObject : RegisteredSaveables)
	{
		if (RegisteredObject.IsValid() && RegisteredObject->GetClass()->ImplementsInterface(UBPI_Saveable::StaticClass()))
		{
			SaveableObjects.AddUnique(RegisteredObject.Get());
		}
	}

	int32 AppliedCount = 0;
	for (UObject* SaveableObject : SaveableObjects)
	{
		if (!IsValid(SaveableObject))
		{
			continue;
		}

		const FGuid Identifier = IBPI_Saveable::Execute_GetSaveIdentifier(SaveableObject);
		if (!Identifier.IsValid())
		{
			continue;
		}

		if (const FSaveBlob* FoundBlob = SaveData->Blobs.Find(Identifier))
		{
			const bool bApplied = IBPI_Saveable::Execute_ApplySaveData(SaveableObject, *FoundBlob);
			if (bApplied)
			{
				AppliedCount++;
			}
		}
	}

	UE_LOG(LogTemp, Log, TEXT("SaveSubsystem: Données appliquées à %d / %d acteurs pour le slot '%s'."),
		AppliedCount, SaveableObjects.Num(), *SlotName);

	OnLoadCompleted.Broadcast(SlotName, true);
}

bool USaveSubsystem::DeleteSlot(const FString& SlotName)
{
	if (SlotName.IsEmpty())
	{
		return false;
	}
	return UGameplayStatics::DeleteGameInSlot(SlotName, 0);
}

bool USaveSubsystem::DoesSlotExist(const FString& SlotName) const
{
	if (SlotName.IsEmpty())
	{
		return false;
	}
	return UGameplayStatics::DoesSaveGameExist(SlotName, 0);
}
