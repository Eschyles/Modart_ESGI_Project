// Copyright 2022-2025 Alexander Floden, Alias Alex River. All Rights Reserved. 

#include "RVPDPRapidJsonSerializer.h"
#include "ThirdParty/rapidjson/writer.h"
#include "ThirdParty/rapidjson/stringbuffer.h"


void SerializeInt(rapidjson::Value& value, int32 intValue, rapidjson::Value::AllocatorType& allocator) {
    value.SetInt(intValue);
}

void SerializeUInt8(rapidjson::Value& value, uint8 uint8Value, rapidjson::Value::AllocatorType& allocator) {
    value.SetUint(uint8Value);
}

void SerializeInt64(rapidjson::Value& value, int64 int64Value, rapidjson::Value::AllocatorType& allocator) {
    value.SetInt64(int64Value);
}

void SerializeFString(rapidjson::Value& value, const FString& stringValue, rapidjson::Value::AllocatorType& allocator) {
    value.SetString(TCHAR_TO_UTF8(*stringValue), allocator);
}

void SerializeFColor(rapidjson::Value& value, const FColor& colorValue, rapidjson::Value::AllocatorType& allocator) {
    rapidjson::Value colorObject(rapidjson::kObjectType);
    colorObject.AddMember("R", colorValue.R, allocator);
    colorObject.AddMember("G", colorValue.G, allocator);
    colorObject.AddMember("B", colorValue.B, allocator);
    colorObject.AddMember("A", colorValue.A, allocator);
    value.Swap(colorObject);
}

void SerializeBool(rapidjson::Value& value, bool boolValue, rapidjson::Value::AllocatorType& allocator) {
    value.SetBool(boolValue);
}

void SerializeFName(rapidjson::Value& value, const FName& nameValue, rapidjson::Value::AllocatorType& allocator) {
    value.SetString(TCHAR_TO_UTF8(*nameValue.ToString()), allocator);
}

void SerializeFText(rapidjson::Value& value, const FText& textValue, rapidjson::Value::AllocatorType& allocator) {
    value.SetString(TCHAR_TO_UTF8(*textValue.ToString()), allocator);
}

void SerializeFloat(rapidjson::Value& root, const float& FloatValue, rapidjson::Value::AllocatorType& allocator)
{
    root.SetDouble(FloatValue);
}

void SerializeFVector(rapidjson::Value& root, const FVector& VectorValue, rapidjson::Value::AllocatorType& allocator)
{
    root.SetObject();
    root.AddMember("X", VectorValue.X, allocator);
    root.AddMember("Y", VectorValue.Y, allocator);
    root.AddMember("Z", VectorValue.Z, allocator);
}

void SerializeFRotator(rapidjson::Value& root, const FRotator& RotatorValue, rapidjson::Value::AllocatorType& allocator)
{
    root.SetObject();
    root.AddMember("Pitch", RotatorValue.Pitch, allocator);
    root.AddMember("Yaw", RotatorValue.Yaw, allocator);
    root.AddMember("Roll", RotatorValue.Roll, allocator);
}

void SerializeFTransform(rapidjson::Value& root, const FTransform& TransformValue, rapidjson::Value::AllocatorType& allocator)
{
    root.SetObject();
    rapidjson::Value location(rapidjson::kObjectType);
    SerializeFVector(location, TransformValue.GetLocation(), allocator);
    root.AddMember("Location", location, allocator);

    rapidjson::Value rotation(rapidjson::kObjectType);
    SerializeFRotator(rotation, TransformValue.Rotator(), allocator);
    root.AddMember("Rotation", rotation, allocator);

    rapidjson::Value scale(rapidjson::kObjectType);
    SerializeFVector(scale, TransformValue.GetScale3D(), allocator);
    root.AddMember("Scale", scale, allocator);
}



void SerializeTArrayFColor(rapidjson::Value& OutValue, const TArray<FColor>& InColors, rapidjson::Value::AllocatorType& allocator)
{
    OutValue.SetArray();
    for (const FColor& color : InColors)
    {
        rapidjson::Value colorValue;
        SerializeFColor(colorValue, color, allocator);
        OutValue.PushBack(colorValue, allocator);
    }
}

void SerializeTArrayInt(rapidjson::Value& OutValue, const TArray<int32>& InInts, rapidjson::Value::AllocatorType& allocator)
{
    OutValue.SetArray();
    for (const int32& intVal : InInts)
    {
        rapidjson::Value intValue;
        SerializeInt(intValue, intVal, allocator);
        OutValue.PushBack(intValue, allocator);
    }
}

void SerializeTArrayUInt8(rapidjson::Value& OutValue, const TArray<uint8>& InUInt8s, rapidjson::Value::AllocatorType& allocator)
{
    OutValue.SetArray();
    for (const uint8& uint8Val : InUInt8s)
    {
        rapidjson::Value uint8Value;
        SerializeUInt8(uint8Value, uint8Val, allocator);
        OutValue.PushBack(uint8Value, allocator);
    }
}

void SerializeTArrayInt64(rapidjson::Value& OutValue, const TArray<int64>& InInt64s, rapidjson::Value::AllocatorType& allocator)
{
    OutValue.SetArray();
    for (const int64& int64Val : InInt64s)
    {
        rapidjson::Value int64Value;
        SerializeInt64(int64Value, int64Val, allocator);
        OutValue.PushBack(int64Value, allocator);
    }
}

void SerializeTArrayFString(rapidjson::Value& OutValue, const TArray<FString>& InStrings, rapidjson::Value::AllocatorType& allocator)
{
    OutValue.SetArray();
    for (const FString& stringVal : InStrings)
    {
        rapidjson::Value stringValue;
        SerializeFString(stringValue, stringVal, allocator);
        OutValue.PushBack(stringValue, allocator);
    }
}

void SerializeTArrayBool(rapidjson::Value& OutValue, const TArray<bool>& InBools, rapidjson::Value::AllocatorType& allocator)
{
    OutValue.SetArray();
    for (const bool& boolVal : InBools)
    {
        rapidjson::Value boolValue;
        SerializeBool(boolValue, boolVal, allocator);
        OutValue.PushBack(boolValue, allocator);
    }
}

void SerializeTArrayFName(rapidjson::Value& OutValue, const TArray<FName>& InNames, rapidjson::Value::AllocatorType& allocator)
{
    OutValue.SetArray();
    for (const FName& nameVal : InNames)
    {
        rapidjson::Value nameValue;
        SerializeFName(nameValue, nameVal, allocator);
        OutValue.PushBack(nameValue, allocator);
    }
}

void SerializeTArrayFText(rapidjson::Value& OutValue, const TArray<FText>& InTexts, rapidjson::Value::AllocatorType& allocator)
{
    OutValue.SetArray();
    for (const FText& textVal : InTexts)
    {
        rapidjson::Value textValue;
        SerializeFText(textValue, textVal, allocator);
        OutValue.PushBack(textValue, allocator);
    }
}

void SerializeTArrayFloat(rapidjson::Value& OutValue, const TArray<float>& inFloats, rapidjson::Value::AllocatorType& allocator)
{
    OutValue.SetArray();
    for (const float& floatTemp : inFloats)
    {
        rapidjson::Value floatValue;
        SerializeFloat(floatValue, floatTemp, allocator);
        OutValue.PushBack(floatValue, allocator);
    }
}

void SerializeTArrayFVector(rapidjson::Value& OutValue, const TArray<FVector>& inVector, rapidjson::Value::AllocatorType& allocator)
{
    OutValue.SetArray();
    for (const FVector& vectorTemp : inVector)
    {
        rapidjson::Value vectorValue;
        SerializeFVector(vectorValue, vectorTemp, allocator);
        OutValue.PushBack(vectorValue, allocator);
    }
}

void SerializeTArrayFRotator(rapidjson::Value& OutValue, const TArray<FRotator>& inRotator, rapidjson::Value::AllocatorType& allocator)
{
    OutValue.SetArray();
    for (const FRotator& rotatorTemp : inRotator)
    {
        rapidjson::Value rotatorValue;
        SerializeFRotator(rotatorValue, rotatorTemp, allocator);
        OutValue.PushBack(rotatorValue, allocator);
    }
}

void SerializeTArrayFTransform(rapidjson::Value& OutValue, const TArray<FTransform>& inTransform, rapidjson::Value::AllocatorType& allocator)
{
    OutValue.SetArray();
    for (const FTransform& transformTemp : inTransform)
    {
        rapidjson::Value transformValue;
        SerializeFTransform(transformValue, transformTemp, allocator);
        OutValue.PushBack(transformValue, allocator);
    }
}
