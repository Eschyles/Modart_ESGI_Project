// Copyright 2022-2025 Alexander Floden, Alias Alex River. All Rights Reserved. 

#pragma once

#include "CoreMinimal.h"
#include "ThirdParty/rapidjson/document.h"


void SerializeInt(rapidjson::Value& value, int32 intValue, rapidjson::Value::AllocatorType& allocator);
void SerializeUInt8(rapidjson::Value& value, uint8 uint8Value, rapidjson::Value::AllocatorType& allocator);
void SerializeInt64(rapidjson::Value& value, int64 int64Value, rapidjson::Value::AllocatorType& allocator);
void SerializeFString(rapidjson::Value& value, const FString& stringValue, rapidjson::Value::AllocatorType& allocator);
void SerializeFColor(rapidjson::Value& value, const FColor& colorValue, rapidjson::Value::AllocatorType& allocator);
void SerializeBool(rapidjson::Value& value, bool boolValue, rapidjson::Value::AllocatorType& allocator);
void SerializeFName(rapidjson::Value& value, const FName& nameValue, rapidjson::Value::AllocatorType& allocator);
void SerializeFText(rapidjson::Value& value, const FText& textValue, rapidjson::Value::AllocatorType& allocator);
void SerializeFloat(rapidjson::Value& root, const float& FloatValue, rapidjson::Value::AllocatorType& allocator);
void SerializeFVector(rapidjson::Value& root, const FVector& VectorValue, rapidjson::Value::AllocatorType& allocator);
void SerializeFRotator(rapidjson::Value& root, const FRotator& RotatorValue, rapidjson::Value::AllocatorType& allocator);
void SerializeFTransform(rapidjson::Value& root, const FTransform& TransformValue, rapidjson::Value::AllocatorType& allocator);



void SerializeTArrayFColor(rapidjson::Value& OutValue, const TArray<FColor>& InColors, rapidjson::Value::AllocatorType& allocator);
void SerializeTArrayInt(rapidjson::Value& OutValue, const TArray<int32>& InInts, rapidjson::Value::AllocatorType& allocator);
void SerializeTArrayUInt8(rapidjson::Value& OutValue, const TArray<uint8>& InUInt8s, rapidjson::Value::AllocatorType& allocator);
void SerializeTArrayInt64(rapidjson::Value& OutValue, const TArray<int64>& InInt64s, rapidjson::Value::AllocatorType& allocator);
void SerializeTArrayFString(rapidjson::Value& OutValue, const TArray<FString>& InStrings, rapidjson::Value::AllocatorType& allocator);
void SerializeTArrayBool(rapidjson::Value& OutValue, const TArray<bool>& InBools, rapidjson::Value::AllocatorType& allocator);
void SerializeTArrayFName(rapidjson::Value& OutValue, const TArray<FName>& InNames, rapidjson::Value::AllocatorType& allocator);
void SerializeTArrayFText(rapidjson::Value& OutValue, const TArray<FText>& InTexts, rapidjson::Value::AllocatorType& allocator);
void SerializeTArrayFloat(rapidjson::Value& OutValue, const TArray<float>& InStrings, rapidjson::Value::AllocatorType& allocator);
void SerializeTArrayFVector(rapidjson::Value& OutValue, const TArray<FVector>& InBools, rapidjson::Value::AllocatorType& allocator);
void SerializeTArrayFRotator(rapidjson::Value& OutValue, const TArray<FRotator>& InNames, rapidjson::Value::AllocatorType& allocator);
void SerializeTArrayFTransform(rapidjson::Value& OutValue, const TArray<FTransform>& InTexts, rapidjson::Value::AllocatorType& allocator);
