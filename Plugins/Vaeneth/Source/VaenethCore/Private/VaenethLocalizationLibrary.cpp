// Copyright 2026 Vaeneth. All Rights Reserved.

#include "VaenethLocalizationLibrary.h"

#if WITH_EDITOR
#include "AssetToolsModule.h"
#include "Factories/StringTableFactory.h"
#include "Internationalization/StringTable.h"
#include "Internationalization/StringTableCore.h"
#include "Misc/PackageName.h"
#include "UObject/Package.h"
#include "UObject/SavePackage.h"
#include "UObject/UObjectGlobals.h"
#endif

bool UVaenethLocalizationLibrary::CreateAndPopulateVaenethStringTables()
{
#if WITH_EDITOR
	const FString FolderPath = TEXT("/Game/Vaeneth/Data/Localization");
	const TArray<FString> TableNames =
	{
		TEXT("ST_UI"),
		TEXT("ST_Dialogue"),
		TEXT("ST_Items"),
		TEXT("ST_System")
	};

	IAssetTools& AssetTools = FAssetToolsModule::GetModule().Get();

	for (const FString& TableName : TableNames)
	{
		const FString ObjectPath = FString::Printf(TEXT("%s/%s.%s"), *FolderPath, *TableName, *TableName);
		UStringTable* StringTable = LoadObject<UStringTable>(nullptr, *ObjectPath);

		if (!StringTable)
		{
			UStringTableFactory* Factory = NewObject<UStringTableFactory>();
			StringTable = Cast<UStringTable>(AssetTools.CreateAsset(
				TableName,
				FolderPath,
				UStringTable::StaticClass(),
				Factory,
				TEXT("VaenethLocalizationLibrary")));
		}

		if (!StringTable)
		{
			return false;
		}

		StringTable->Modify();

		if (TableName == TEXT("ST_UI"))
		{
			StringTable->GetMutableStringTable()->SetSourceString(TEXT("UI.Health.Label"), TEXT("Vie"));
			StringTable->GetMutableStringTable()->SetSourceString(TEXT("UI.Ether.Label"), TEXT("Ether"));
			StringTable->GetMutableStringTable()->SetSourceString(TEXT("UI.Prompt.Interact"), TEXT("Interagir"));
		}

		StringTable->MarkPackageDirty();

		FString PackageFilename;
		if (!FPackageName::TryConvertLongPackageNameToFilename(
			StringTable->GetOutermost()->GetName(),
			PackageFilename,
			FPackageName::GetAssetPackageExtension()))
		{
			return false;
		}

		FSavePackageArgs SaveArgs;
		SaveArgs.TopLevelFlags = RF_Public | RF_Standalone;
		if (!UPackage::SavePackage(
			StringTable->GetOutermost(),
			StringTable,
			*PackageFilename,
			SaveArgs))
		{
			return false;
		}
	}

	UStringTable* UITestTable = LoadObject<UStringTable>(nullptr, TEXT("/Game/Vaeneth/Data/Localization/ST_UI.ST_UI"));
	if (!UITestTable)
	{
		return false;
	}

	FString SourceString;
	return UITestTable->GetStringTable()->GetSourceString(TEXT("UI.Health.Label"), SourceString)
		&& SourceString == TEXT("Vie")
		&& UITestTable->GetStringTable()->GetSourceString(TEXT("UI.Ether.Label"), SourceString)
		&& SourceString == TEXT("Ether")
		&& UITestTable->GetStringTable()->GetSourceString(TEXT("UI.Prompt.Interact"), SourceString)
		&& SourceString == TEXT("Interagir");
#else
	return false;
#endif
}
