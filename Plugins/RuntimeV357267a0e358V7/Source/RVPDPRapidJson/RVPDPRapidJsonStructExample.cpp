// Copyright 2022-2025 Alexander Floden, Alias Alex River. All Rights Reserved. 

#include "RVPDPRapidJsonStructExample.h"


// Called when the game starts or when spawned
void ARVPDPRapidJsonStructExample::BeginPlay() {

	Super::BeginPlay();
	

	// Serializing

	MyCustomStruct.Name = "TestString";
	MyCustomStruct.Age = 10;

	if (URVPDPRapidJsonFunctionLibrary::SerializeCustomStruct(MyCustomStruct, MyCustomStructSerialized)) {

		UE_LOG(LogTemp, Warning, TEXT("Successfull Serialization of MyCustomStruct: %s"), *MyCustomStructSerialized);
	}

	else {

		UE_LOG(LogTemp, Warning, TEXT("Serialization of MyCustomStruct Failed"));
	}




	// De-Serializing

	FRVPDPRapidJsonMyCustomStructSerializationExample deserializedMyCustomStruct;

	if (URVPDPRapidJsonFunctionLibrary::DeserializeCustomStruct(MyCustomStructSerialized, deserializedMyCustomStruct)) {

		UE_LOG(LogTemp, Warning, TEXT("Deserialization of MyCustomStructSerialized: %s - age: %i "), *deserializedMyCustomStruct.Name, deserializedMyCustomStruct.Age);
	}
	else {

		UE_LOG(LogTemp, Warning, TEXT("De-Serialization of MyCustomStructSerialized Failed"));
	}
}

