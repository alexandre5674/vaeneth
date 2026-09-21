// Copyright Vaeneth. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Save/SaveTypes.h"
#include "FractureSubsystem.generated.h"

USTRUCT(BlueprintType)
struct VAENETH_API FFractureRecord
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, Category = "Fracture")
    FGuid UniqueID;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, Category = "Fracture")
    FName LevelName = NAME_None;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, Category = "Fracture")
    FVector Location = FVector::ZeroVector;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, Category = "Fracture")
    FRotator Rotation = FRotator::ZeroRotator;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, Category = "Fracture")
    FGameplayTag SizeTag;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, Category = "Fracture")
    float CreationTimestamp = 0.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, Category = "Fracture")
    bool bSealed = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, Category = "Fracture")
    int32 DeathContext = 0;
};

USTRUCT()
struct FFractureSavePayload
{
    GENERATED_BODY()

    UPROPERTY()
    TArray<FFractureRecord> Records;
};

UCLASS(BlueprintType)
class VAENETH_API UFractureSubsystem : public UGameInstanceSubsystem, public IBPI_Saveable
{
    GENERATED_BODY()

public:
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;

    UFUNCTION(BlueprintCallable, Category = "Vaeneth|Fractures")
    FGuid RegisterFracture(FFractureRecord Record);

    UFUNCTION(BlueprintPure, Category = "Vaeneth|Fractures")
    TArray<FFractureRecord> GetFracturesForLevel(FName Level) const;

    UFUNCTION(BlueprintCallable, Category = "Vaeneth|Fractures")
    bool SealFracture(FGuid ID);

    UFUNCTION(BlueprintCallable, Category = "Vaeneth|Fractures")
    void SpawnFracturesForCurrentLevel();

    UFUNCTION(BlueprintPure, Category = "Vaeneth|Fractures")
    int32 GetFractureCount() const;

    virtual FSaveBlob GatherSaveData_Implementation() override;
    virtual bool ApplySaveData_Implementation(const FSaveBlob& Data) override;
    virtual FGuid GetSaveIdentifier_Implementation() override;

private:
    void HandlePostLoadMap(UWorld* LoadedWorld);
    AActor* SpawnFractureActor(const FFractureRecord& Record, bool bBroadcastOpened);
    void DestroySpawnedFractures();
    FName GetCurrentLevelName() const;
    float ResolveFractureScale(int32 DeathContext) const;

    UPROPERTY()
    TMap<FGuid, FFractureRecord> Fractures;

    TMap<FGuid, TWeakObjectPtr<AActor>> SpawnedFractures;
};