// Copyright 2022-2025 Alexander Floden, Alias Alex River. All Rights Reserved. 

#pragma once

#include "CoreMinimal.h"
#include "ThirdParty/rapidjson/document.h"


int32 DeserializeInt(const rapidjson::Value& value);
uint8 DeserializeUInt8(const rapidjson::Value& value);
int64 DeserializeInt64(const rapidjson::Value& value);
FString DeserializeFString(const rapidjson::Value& value);
FColor DeserializeFColor(const rapidjson::Value& value);
bool DeserializeBool(const rapidjson::Value& value);
FName DeserializeFName(const rapidjson::Value& value);
FText DeserializeFText(const rapidjson::Value& value);
float DeserializeFloat(const rapidjson::Value& value);
FVector DeserializeFVector(const rapidjson::Value& value);
FRotator DeserializeFRotator(const rapidjson::Value& value);
FTransform DeserializeFTransform(const rapidjson::Value& value);


TArray<FColor> DeserializeTArrayFColor(const rapidjson::Value& InValue);
TArray<int32> DeserializeTArrayInt(const rapidjson::Value& InValue);
TArray<uint8> DeserializeTArrayUInt8(const rapidjson::Value& InValue);
TArray<int64> DeserializeTArrayInt64(const rapidjson::Value& InValue);
TArray<FString> DeserializeTArrayFString(const rapidjson::Value& InValue);
TArray<bool> DeserializeTArrayBool(const rapidjson::Value& InValue);
TArray<FName> DeserializeTArrayFName(const rapidjson::Value& InValue);
TArray<FText> DeserializeTArrayFText(const rapidjson::Value& InValue);
TArray<float> DeserializeTArrayFloat(const rapidjson::Value& InValue);
TArray<FVector> DeserializeTArrayFVector(const rapidjson::Value& InValue);
TArray<FRotator> DeserializeTArrayFRotator(const rapidjson::Value& InValue);
TArray<FTransform> DeserializeTArrayFTransform(const rapidjson::Value& InValue);
