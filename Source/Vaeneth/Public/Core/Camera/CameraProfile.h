// Copyright Vaeneth. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "CameraProfile.generated.h"

USTRUCT(BlueprintType)
struct VAENETH_API FCameraProfile
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Camera")
    float ArmLength = 200.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Camera")
    FVector SocketOffset = FVector::ZeroVector;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Camera")
    float FOV = 68.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Camera")
    float LagSpeed = 12.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Camera")
    float RotationLagSpeed = 14.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Camera")
    float ShakeScale = 1.0f;
};

UCLASS(BlueprintType)
class VAENETH_API UDA_CameraProfile : public UPrimaryDataAsset
{
    GENERATED_BODY()

public:
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Camera")
    FCameraProfile Profile;
};
