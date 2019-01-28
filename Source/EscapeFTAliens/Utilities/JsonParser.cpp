#include"JsonParser.h"

#include "Utilities/EFTAMap.h"
#include "Containers/UnrealString.h"

#include "Json.h"
#include "JsonSerializer.h"


EFTAMap* EFTAJsonParser::ParseMapJson(const FString& jsonName)
{
	FString contentFolder = FPaths::GameContentDir();
	FString JsonFilePath = contentFolder + "\\..\\Map\\" + jsonName;

	FString JsonString; //Json converted to FString

	FFileHelper::LoadFileToString(JsonString, *JsonFilePath);

	TSharedPtr<FJsonObject> JsonObject = MakeShareable(new FJsonObject());
	TSharedRef<TJsonReader<>> JsonReader = TJsonReaderFactory<>::Create(JsonString);

	EFTAMap* map;

	if (FJsonSerializer::Deserialize(JsonReader, JsonObject) && JsonObject.IsValid())
	{
		//The person "object" that is retrieved from the given json file
		TArray <TSharedPtr<FJsonValue>> sectorArray = JsonObject->GetArrayField("sectorList");

		FString mapName = JsonObject->GetStringField("name");

		TSharedPtr<FJsonObject> mapSizeJson = JsonObject->GetObjectField("size");

		FVector2D size;
		size.X = mapSizeJson->GetNumberField("x");
		size.Y = mapSizeJson->GetNumberField("y");

		map = new EFTAMap(mapName, size);

		for (const auto& sector : sectorArray)
		{
			TSharedPtr<FJsonObject> sectorObj = sector->AsObject();

			int32 x = FCString::Atoi(*sectorObj->GetStringField("x"));
			int32 y = FCString::Atoi(*sectorObj->GetStringField("y"));
			FString strType = sectorObj->GetStringField("state");

			EBlockType blockType = EBlockType::BT_BLOCKED;

			if (strType.Equals(TEXT("dangerous")))
			{
				blockType = EBlockType::BT_DANGER;
			}
			else if (strType.Equals(TEXT("safe")))
			{
				blockType = EBlockType::BT_SECURE;
			}
			else if (strType.Equals(TEXT("hatch")))
			{
				blockType = EBlockType::BT_ESCAPE;
			}
			else if (strType.Equals(TEXT("human")))
			{
				blockType = EBlockType::BT_HUMAN;
			}
			else if (strType.Equals(TEXT("alien")))
			{
				blockType = EBlockType::BT_ALIEN;
			}

			map->addSector(FSector(x, y, blockType));
		}

		return map;
	}
	else
	{
		GLog->Log("couldn't deserialize");

		return nullptr;
	}

	return nullptr;
}