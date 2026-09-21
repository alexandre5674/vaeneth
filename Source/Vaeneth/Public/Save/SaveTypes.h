// Copyright Vaeneth. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "GameFramework/SaveGame.h"
#include "GameplayTagContainer.h"
#include "SaveTypes.generated.h"

/**
 * FSaveBlob : Conteneur générique de données de sauvegarde pour un système ou acteur.
 */
USTRUCT(BlueprintType)
struct VAENETH_API FSaveBlob
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, Category = "Save")
	FGameplayTag SystemTag;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, Category = "Save")
	TMap<FName, float> Floats;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, Category = "Save")
	TMap<FName, int32> Ints;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, Category = "Save")
	TMap<FName, bool> Bools;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, Category = "Save")
	TMap<FName, FVector> Vectors;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, Category = "Save")
	TMap<FName, FString> Strings;
};

/**
 * Interface BPI_Saveable : point d'entrée universel pour la sauvegarde.
 */
UINTERFACE(BlueprintType, Blueprintable)
class VAENETH_API UBPI_Saveable : public UInterface
{
	GENERATED_BODY()
};

class VAENETH_API IBPI_Saveable
{
	GENERATED_BODY()

public:
	/** Récolte les données de l'acteur ou composant sous forme de blob. */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Save")
	FSaveBlob GatherSaveData();

	/** Applique les données sauvegardées à l'acteur ou composant. */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Save")
	bool ApplySaveData(const FSaveBlob& Data);

	/** Retourne l'identifiant persistant stable (GUID issu de BPC_PersistentID). */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Save")
	FGuid GetSaveIdentifier();
};

/**
 * SG_VaenethSave : Objet SaveGame racine versionné stockant les blobs par GUID.
 */
UCLASS(BlueprintType)
class VAENETH_API USG_VaenethSave : public USaveGame
{
	GENERATED_BODY()

public:
	USG_VaenethSave();

	static constexpr int32 CurrentSaveVersion = 1;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, SaveGame, Category = "Save")
	int32 SaveVersion = 1;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, SaveGame, Category = "Save")
	FDateTime Timestamp;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, SaveGame, Category = "Save")
	float PlayTime = 0.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, SaveGame, Category = "Save")
	FName LevelName;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, SaveGame, Category = "Save")
	TMap<FGuid, FSaveBlob> Blobs;
};
