// Copyright 2022-2025 Alexander Floden, Alias Alex River. All Rights Reserved. 

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "RVPDPRapidJsonFunctionLibrary.h"
#include "RVPDPRapidJsonStructExample.generated.h"



USTRUCT(BlueprintType)
struct FRVPDPRapidJsonMyCustomStructSerializationExample : public FRVPDPRapidJsonCustomStructSerialization
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Custom Struct Serialization Example")
		FString Name = "";

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Custom Struct Serialization Example")
		int32 Age = 0;

	virtual TSharedPtr<FJsonObject> ToJson() const override {

		TSharedPtr<FJsonObject> JsonObject = Super::ToJson();
		JsonObject->SetStringField(TEXT("Name"), Name);
		JsonObject->SetNumberField(TEXT("Age"), Age);
		return JsonObject;
	}

	virtual bool FromJson(const TSharedPtr<FJsonObject>& JsonObject) override {

		if (!Super::FromJson(JsonObject))
			return false;


		if (JsonObject->HasTypedField<EJson::String>(TEXT("Name")))
			Name = JsonObject->GetStringField(TEXT("Name"));

		if (JsonObject->HasTypedField<EJson::Number>(TEXT("Age")))
			Age = JsonObject->GetIntegerField(TEXT("Age"));

		return true;
	}
};



UCLASS()
class RVPDPRAPIDJSON_API ARVPDPRapidJsonStructExample : public AActor {

	GENERATED_BODY()

protected:

	FRVPDPRapidJsonMyCustomStructSerializationExample MyCustomStruct;
	FString MyCustomStructSerialized;

	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

};
