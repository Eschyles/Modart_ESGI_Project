// Copyright 2022-2025 Alexander Floden, Alias Alex River. All Rights Reserved. 

#pragma once

#include "CoreMinimal.h"
#include "RVPDPRapidJsonFunctionLibrary.h"
#include "Async/AsyncWork.h"


struct FRVPDPSerializationAsyncTask {


	DECLARE_DELEGATE_OneParam(FOnTaskComplete, const FAsyncSerializationInfo&);
	FOnTaskComplete OnTaskComplete;


	TStatId GetStatId() const {

		RETURN_QUICK_DECLARE_CYCLE_STAT(FRVPDPSerializationAsyncTask, STATGROUP_ThreadPoolAsyncTasks);
	}

	bool CanAbandon() {

		return true;
	}

	void Abandon() {

		// Cleanup or finalize any remaining logic if the task is abandoned
	}

	FAsyncSerializationInfo serializationInfoToFill;

	FRVPDPSerializationAsyncTask(FAsyncSerializationInfo serializationInfo) {

		serializationInfoToFill = serializationInfo;
	}

	void DoWork() {


		switch (serializationInfoToFill.ArrayType) {

			case ESerializationArrayTypes::Null:
				break;

			case ESerializationArrayTypes::Int32Array:

				// If array have been set previous to this, we should serialize, instead of de-serialize. 
				if (serializationInfoToFill.Int32Array.Num() > 0)
					serializationInfoToFill.SerializedString = URVPDPRapidJsonFunctionLibrary::SerializeTArrayInt_Wrapper(serializationInfoToFill.Int32Array);
				else if (serializationInfoToFill.SerializedString.Len() > 0)
					serializationInfoToFill.Int32Array = URVPDPRapidJsonFunctionLibrary::DeserializeTArrayInt_Wrapper(serializationInfoToFill.SerializedString);

				break;

			case ESerializationArrayTypes::Uint8Array:

				if (serializationInfoToFill.Uint8Array.Num() > 0)
					serializationInfoToFill.SerializedString = URVPDPRapidJsonFunctionLibrary::SerializeTArrayUInt8_Wrapper(serializationInfoToFill.Uint8Array);
				else if (serializationInfoToFill.SerializedString.Len() > 0)
					serializationInfoToFill.Uint8Array = URVPDPRapidJsonFunctionLibrary::DeserializeTArrayUInt8_Wrapper(serializationInfoToFill.SerializedString);

				break;

			case ESerializationArrayTypes::Int64Array:

				if (serializationInfoToFill.Int64Array.Num() > 0)
					serializationInfoToFill.SerializedString = URVPDPRapidJsonFunctionLibrary::SerializeTArrayInt64_Wrapper(serializationInfoToFill.Int64Array);
				else if (serializationInfoToFill.SerializedString.Len() > 0)
					serializationInfoToFill.Int64Array = URVPDPRapidJsonFunctionLibrary::DeserializeTArrayInt64_Wrapper(serializationInfoToFill.SerializedString);

				break;

			case ESerializationArrayTypes::FStringArray:

				if (serializationInfoToFill.StringArray.Num() > 0)
					serializationInfoToFill.SerializedString = URVPDPRapidJsonFunctionLibrary::SerializeTArrayFString_Wrapper(serializationInfoToFill.StringArray);
				else if (serializationInfoToFill.SerializedString.Len() > 0)
					serializationInfoToFill.StringArray = URVPDPRapidJsonFunctionLibrary::DeserializeTArrayFString_Wrapper(serializationInfoToFill.SerializedString);

				break;

			case ESerializationArrayTypes::FColorArray:

				if (serializationInfoToFill.ColorArray.Num() > 0)
					serializationInfoToFill.SerializedString = URVPDPRapidJsonFunctionLibrary::SerializeTArrayFColor_Wrapper(serializationInfoToFill.ColorArray);
				else if (serializationInfoToFill.SerializedString.Len() > 0)
					serializationInfoToFill.ColorArray = URVPDPRapidJsonFunctionLibrary::DeserializeTArrayFColor_Wrapper(serializationInfoToFill.SerializedString);

				break;

			case ESerializationArrayTypes::BoolArray:

				if (serializationInfoToFill.BoolArray.Num() > 0)
					serializationInfoToFill.SerializedString = URVPDPRapidJsonFunctionLibrary::SerializeTArrayBool_Wrapper(serializationInfoToFill.BoolArray);
				else if (serializationInfoToFill.SerializedString.Len() > 0)
					serializationInfoToFill.BoolArray = URVPDPRapidJsonFunctionLibrary::DeserializeTArrayBool_Wrapper(serializationInfoToFill.SerializedString);

				break;

			case ESerializationArrayTypes::FNameArray:

				if (serializationInfoToFill.NameArray.Num() > 0)
					serializationInfoToFill.SerializedString = URVPDPRapidJsonFunctionLibrary::SerializeTArrayFName_Wrapper(serializationInfoToFill.NameArray);
				else if (serializationInfoToFill.SerializedString.Len() > 0)
					serializationInfoToFill.NameArray = URVPDPRapidJsonFunctionLibrary::DeserializeTArrayFName_Wrapper(serializationInfoToFill.SerializedString);

				break;

			case ESerializationArrayTypes::FTextArray:

				if (serializationInfoToFill.TextArray.Num() > 0)
					serializationInfoToFill.SerializedString = URVPDPRapidJsonFunctionLibrary::SerializeTArrayFText_Wrapper(serializationInfoToFill.TextArray);
				else if (serializationInfoToFill.SerializedString.Len() > 0)
					serializationInfoToFill.TextArray = URVPDPRapidJsonFunctionLibrary::DeserializeTArrayFText_Wrapper(serializationInfoToFill.SerializedString);

				break;

			case ESerializationArrayTypes::FloatArray:

				if (serializationInfoToFill.FloatArray.Num() > 0)
					serializationInfoToFill.SerializedString = URVPDPRapidJsonFunctionLibrary::SerializeTArrayFloat_Wrapper(serializationInfoToFill.FloatArray);
				else if (serializationInfoToFill.SerializedString.Len() > 0)
					serializationInfoToFill.FloatArray = URVPDPRapidJsonFunctionLibrary::DeserializeTArrayFloat_Wrapper(serializationInfoToFill.SerializedString);

				break;

			case ESerializationArrayTypes::FVectorArray:

				if (serializationInfoToFill.VectorArray.Num() > 0)
					serializationInfoToFill.SerializedString = URVPDPRapidJsonFunctionLibrary::SerializeTArrayFVector_Wrapper(serializationInfoToFill.VectorArray);
				else if (serializationInfoToFill.SerializedString.Len() > 0)
					serializationInfoToFill.VectorArray = URVPDPRapidJsonFunctionLibrary::DeserializeTArrayFVector_Wrapper(serializationInfoToFill.SerializedString);

				break;

			case ESerializationArrayTypes::FRotatorArray:
				
				if (serializationInfoToFill.RotatorArray.Num() > 0)
					serializationInfoToFill.SerializedString = URVPDPRapidJsonFunctionLibrary::SerializeTArrayFRotator_Wrapper(serializationInfoToFill.RotatorArray);
				else if (serializationInfoToFill.SerializedString.Len() > 0)
					serializationInfoToFill.RotatorArray = URVPDPRapidJsonFunctionLibrary::DeserializeTArrayFRotator_Wrapper(serializationInfoToFill.SerializedString);

				break;

			case ESerializationArrayTypes::FTransformArray:

				if (serializationInfoToFill.TransformArray.Num() > 0)
					serializationInfoToFill.SerializedString = URVPDPRapidJsonFunctionLibrary::SerializeTArrayFTransform_Wrapper(serializationInfoToFill.TransformArray);
				else if (serializationInfoToFill.SerializedString.Len() > 0)
					serializationInfoToFill.TransformArray = URVPDPRapidJsonFunctionLibrary::DeserializeTArrayFTransform_Wrapper(serializationInfoToFill.SerializedString);

				break;

			default:
				break;
		}

		OnTaskComplete.ExecuteIfBound(serializationInfoToFill);
	}
};