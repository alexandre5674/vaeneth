// Copyright Vaeneth. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Save/SaveTypes.h"
#include "SaveSubsystem.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnSaveSlotCompleted, const FString&, SlotName, bool, bSuccess);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnLoadSlotCompleted, const FString&, SlotName, bool, bSuccess);

/**
 * USaveSubsystem : Sous-système asynchrone universel de sauvegarde et chargement.
 * Ne dépend d'aucun système concret : itère sur les acteurs implémentant BPI_Saveable.
 */
UCLASS(BlueprintType)
class VAENETH_API USaveSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	/** Sauvegarde asynchrone de tous les acteurs implémentant BPI_Saveable dans le slot spécifié. */
	UFUNCTION(BlueprintCallable, Category = "Vaeneth|Save")
	void SaveGameAsync(const FString& SlotName);

	/** Chargement asynchrone du slot spécifié et application aux acteurs implémentant BPI_Saveable. */
	UFUNCTION(BlueprintCallable, Category = "Vaeneth|Save")
	void LoadGameAsync(const FString& SlotName);

	/** Supprime le fichier de sauvegarde associé au slot. */
	UFUNCTION(BlueprintCallable, Category = "Vaeneth|Save")
	bool DeleteSlot(const FString& SlotName);

	/** Vérifie si une sauvegarde existe pour le slot spécifié. */
	UFUNCTION(BlueprintPure, Category = "Vaeneth|Save")
	bool DoesSlotExist(const FString& SlotName) const;

	/** Enregistre un UObject implémentant BPI_Saveable auprès du collecteur générique. */
	UFUNCTION(BlueprintCallable, Category = "Vaeneth|Save")
	void RegisterSaveable(UObject* Object);

	/** Retire un UObject précédemment enregistré. */
	UFUNCTION(BlueprintCallable, Category = "Vaeneth|Save")
	void UnregisterSaveable(UObject* Object);

	/** Événement diffusé à la fin d'une sauvegarde asynchrone. */
	UPROPERTY(BlueprintAssignable, Category = "Vaeneth|Save")
	FOnSaveSlotCompleted OnSaveCompleted;

	/** Événement diffusé à la fin d'un chargement asynchrone. */
	UPROPERTY(BlueprintAssignable, Category = "Vaeneth|Save")
	FOnLoadSlotCompleted OnLoadCompleted;

	private:
	void OnAsyncSaveFinished(const FString& SlotName, const int32 UserIndex, bool bSuccess);
	void OnAsyncLoadFinished(const FString& SlotName, const int32 UserIndex, USaveGame* LoadedData);

	UPROPERTY()
	TArray<TWeakObjectPtr<UObject>> RegisteredSaveables;
	};
