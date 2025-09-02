// Copyright 2022-2025 Alexander Floden, Alias Alex River. All Rights Reserved. 

#include "RVPDPRapidJsonDeserializer.h"


int32 DeserializeInt(const rapidjson::Value& value) {
    return value.GetInt();
}

uint8 DeserializeUInt8(const rapidjson::Value& value) {
    return value.GetUint();
}

int64 DeserializeInt64(const rapidjson::Value& value) {
    return value.GetInt64();
}

FString DeserializeFString(const rapidjson::Value& value) {
    return FString(value.GetString());
}

FColor DeserializeFColor(const rapidjson::Value& value) {
    return FColor(value["R"].GetUint(), value["G"].GetUint(), value["B"].GetUint(), value["A"].GetUint());
}

bool DeserializeBool(const rapidjson::Value& value) {
    return value.GetBool();
}

FName DeserializeFName(const rapidjson::Value& value) {
    return FName(*FString(value.GetString()));
}

FText DeserializeFText(const rapidjson::Value& value) {
    return FText::FromString(FString(value.GetString()));
}

float DeserializeFloat(const rapidjson::Value& value)
{
    return value.GetFloat();
}

FVector DeserializeFVector(const rapidjson::Value& value)
{
    return FVector(value["X"].GetFloat(), value["Y"].GetFloat(), value["Z"].GetFloat());
}

FRotator DeserializeFRotator(const rapidjson::Value& value)
{
    return FRotator(value["Pitch"].GetFloat(), value["Yaw"].GetFloat(), value["Roll"].GetFloat());
}

FTransform DeserializeFTransform(const rapidjson::Value& value)
{
    FVector location = DeserializeFVector(value["Location"]);
    FRotator rotation = DeserializeFRotator(value["Rotation"]);
    FVector scale = DeserializeFVector(value["Scale"]);

    return FTransform(rotation, location, scale);
}



TArray<FColor> DeserializeTArrayFColor(const rapidjson::Value& InValue)
{
    TArray<FColor> outColors;
    if (InValue.IsArray())
    {
        for (const rapidjson::Value& colorValue : InValue.GetArray())
        {
            FColor color = DeserializeFColor(colorValue);
            outColors.Add(color);
        }
    }
    return outColors;
}

TArray<int32> DeserializeTArrayInt(const rapidjson::Value& InValue)
{
    TArray<int32> outInts;
    if (InValue.IsArray())
    {
        for (const rapidjson::Value& intValue : InValue.GetArray())
        {
            int32 intVal = DeserializeInt(intValue);
            outInts.Add(intVal);
        }
    }
    return outInts;
}

TArray<uint8> DeserializeTArrayUInt8(const rapidjson::Value& InValue)
{
    TArray<uint8> outUInt8s;
    if (InValue.IsArray())
    {
        for (const rapidjson::Value& uint8Value : InValue.GetArray())
        {
            uint8 uint8Val = DeserializeUInt8(uint8Value);
            outUInt8s.Add(uint8Val);
        }
    }
    return outUInt8s;
}

TArray<int64> DeserializeTArrayInt64(const rapidjson::Value& InValue)
{
    TArray<int64> outInt64s;
    if (InValue.IsArray())
    {
        for (const rapidjson::Value& int64Value : InValue.GetArray())
        {
            int64 int64Val = DeserializeInt64(int64Value);
            outInt64s.Add(int64Val);
        }
    }
    return outInt64s;
}

TArray<FString> DeserializeTArrayFString(const rapidjson::Value& InValue)
{
    TArray<FString> outStrings;
    if (InValue.IsArray())
    {
        for (const rapidjson::Value& stringValue : InValue.GetArray())
        {
            FString stringVal = DeserializeFString(stringValue);
            outStrings.Add(stringVal);
        }
    }
    return outStrings;
}

TArray<bool> DeserializeTArrayBool(const rapidjson::Value& InValue)
{
    TArray<bool> outBools;
    if (InValue.IsArray())
    {
        for (const rapidjson::Value& boolValue : InValue.GetArray())
        {
            bool boolVal = DeserializeBool(boolValue);
            outBools.Add(boolVal);
        }
    }
    return outBools;
}

TArray<FName> DeserializeTArrayFName(const rapidjson::Value& InValue)
{
    TArray<FName> outNames;
    if (InValue.IsArray())
    {
        for (const rapidjson::Value& nameValue : InValue.GetArray())
        {
            FName nameVal = DeserializeFName(nameValue);
            outNames.Add(nameVal);
        }
    }
    return outNames;
}

TArray<FText> DeserializeTArrayFText(const rapidjson::Value& InValue)
{
    TArray<FText> outTexts;
    if (InValue.IsArray())
    {
        for (const rapidjson::Value& textValue : InValue.GetArray())
        {
            FText textVal = DeserializeFText(textValue);
            outTexts.Add(textVal);
        }
    }
    return outTexts;
}

TArray<float> DeserializeTArrayFloat(const rapidjson::Value& InValue)
{
    TArray<float> outFloats;

    if (InValue.IsArray()) {

        for (const rapidjson::Value& floatValue : InValue.GetArray())
            outFloats.Add(DeserializeFloat(floatValue));
    }

    return outFloats;
}

TArray<FVector> DeserializeTArrayFVector(const rapidjson::Value& InValue)
{
    TArray<FVector> outVectors;

    if (InValue.IsArray()) {

        for (const rapidjson::Value& vectorValue : InValue.GetArray())
            outVectors.Add(DeserializeFVector(vectorValue));
    }

    return outVectors;
}

TArray<FRotator> DeserializeTArrayFRotator(const rapidjson::Value& InValue)
{
    TArray<FRotator> outRotators;

    if (InValue.IsArray()) {

        for (const rapidjson::Value& rotatorValue : InValue.GetArray())
            outRotators.Add(DeserializeFRotator(rotatorValue));
    }

    return outRotators;
}

TArray<FTransform> DeserializeTArrayFTransform(const rapidjson::Value& InValue)
{
    TArray<FTransform> outTransform;

    if (InValue.IsArray()) {

        for (const rapidjson::Value& transformValue : InValue.GetArray())
            outTransform.Add(DeserializeFTransform(transformValue));
    }

    return outTransform;
}
