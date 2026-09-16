// Copyright 2026 Vaeneth. All Rights Reserved.

#pragma once

#include "Kismet/BlueprintFunctionLibrary.h"
#include "VaenethLocalizationLibrary.generated.h"

UCLASS()
class VAENETHCORE_API UVaenethLocalizationLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "Vaeneth|Localization")
	static bool CreateAndPopulateVaenethStringTables();
};
