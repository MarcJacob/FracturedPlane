#pragma once

#include "CoreMinimal.h"
#include "FPGameConfig.generated.h"

UCLASS(Config=Game, defaultconfig, meta = (DisplayName="FP Game Config"))
class FRACTUREDPLANECLIENTCORE_API UFPGameConfig : public UDeveloperSettings
{
	GENERATED_BODY()
public:
	
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Map Management", meta = (AllowedClasses = "World"))
	FSoftObjectPath MainMenuMapPath;

	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Map Management", meta = (AllowedClasses = "World"))
	FSoftObjectPath PreGameMapPath;
	
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Map Management", meta = (AllowedClasses = "World"))
	FSoftObjectPath WorldViewMapPath;
};