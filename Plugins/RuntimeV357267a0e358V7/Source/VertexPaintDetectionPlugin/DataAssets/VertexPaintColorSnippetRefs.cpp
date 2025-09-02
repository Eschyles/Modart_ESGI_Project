// Copyright 2022-2025 Alexander Floden, Alias Alex River. All Rights Reserved. 


#include "VertexPaintColorSnippetRefs.h"

// Engine
#include "VertexPaintFunctionLibrary.h"
#include "Engine/StaticMesh.h"
#include "Engine/SkeletalMesh.h"
#include "Kismet/KismetMathLibrary.h"
#include "Components/PrimitiveComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SkeletalMeshComponent.h"

// Plugin
#include "VertexPaintColorSnippetDataAsset.h"


//-------------------------------------------------------

// Get Object From Snippet ID

TSoftObjectPtr<UObject> UVertexPaintColorSnippetRefs::GetObjectFromSnippetID(const FString& SnippetID) {

    if (SnippetID.Len() <= 0) return TSoftObjectPtr<UObject>();


    if (StaticMeshesColorSnippets.Num() > 0) {

        for (auto& object : StaticMeshesColorSnippets) {

            TArray<FString> colorSnippets;

            object.Value.ColorSnippetsStorageInfo.GetKeys(colorSnippets);

            if (colorSnippets.Contains(SnippetID))
                return object.Key;
        }

    }

    if (SkeletalMeshesColorSnippets.Num() > 0) {

        for (auto& object : SkeletalMeshesColorSnippets) {

            TArray<FString> colorSnippets;

            object.Value.ColorSnippetsStorageInfo.GetKeys(colorSnippets);

            if (colorSnippets.Contains(SnippetID))
                return object.Key;
        }
    }


    return TSoftObjectPtr<UObject>();
}


//-------------------------------------------------------

// Get All Color Snippets And Data Asset For Object

TMap<FString, FRVPDPStoredColorSnippetInfo> UVertexPaintColorSnippetRefs::GetAllColorSnippetsAndDataAssetForObject(const UObject* Object) {

    if (!IsValid(Object)) return TMap<FString, FRVPDPStoredColorSnippetInfo>();


    if (StaticMeshesColorSnippets.Contains(Object)) {

        return StaticMeshesColorSnippets.FindRef(Object).ColorSnippetsStorageInfo;
    }

    else if (SkeletalMeshesColorSnippets.Contains(Object)) {

        return SkeletalMeshesColorSnippets.FindRef(Object).ColorSnippetsStorageInfo;
    }

    return TMap<FString, FRVPDPStoredColorSnippetInfo>();
}


//-------------------------------------------------------

// Get All Color Snippets In Specified Data Asset

TMap<FString, FRVPDPStoredColorSnippetInfo> UVertexPaintColorSnippetRefs::GetAllColorSnippetsInSpecifiedDataAsset(const UVertexPaintColorSnippetDataAsset* ColorSnippetDataAsset, bool IncludeChildGroupSnippets) {

    if (!ColorSnippetDataAsset) return TMap<FString, FRVPDPStoredColorSnippetInfo>();


    TMap<FString, FRVPDPStoredColorSnippetInfo> colorSnippetsStoredOnSpecifiedDataAsset;

    TArray<FRVPDPColorSnippetReferenceDataInfo> colorSnippetReferenceInfo;
    StaticMeshesColorSnippets.GenerateValueArray(colorSnippetReferenceInfo);

    TArray<FRVPDPColorSnippetReferenceDataInfo> skelMeshReferenceInfo;
    SkeletalMeshesColorSnippets.GenerateValueArray(skelMeshReferenceInfo);

    colorSnippetReferenceInfo.Append(skelMeshReferenceInfo);


    for (FRVPDPColorSnippetReferenceDataInfo snippetReferenceInfo : colorSnippetReferenceInfo) {

        for (auto& dataAssetStoredOn : snippetReferenceInfo.ColorSnippetsStorageInfo) {

            if (dataAssetStoredOn.Value.ColorSnippetDataAssetStoredOn.LoadSynchronous() == ColorSnippetDataAsset) {

                if (IncludeChildGroupSnippets) {

                    colorSnippetsStoredOnSpecifiedDataAsset.Add(dataAssetStoredOn);
                }
                
                else if (!dataAssetStoredOn.Value.IsPartOfAGroupSnippet) {

                    colorSnippetsStoredOnSpecifiedDataAsset.Add(dataAssetStoredOn);
                }
            }
        }
    }


    return colorSnippetsStoredOnSpecifiedDataAsset;
}


//-------------------------------------------------------

// Get All Group Snippets In Specified Data Asset

TMap<FString, FRVPDPGroupColorSnippetInfo> UVertexPaintColorSnippetRefs::GetAllGroupSnippetsInSpecifiedDataAsset(const UVertexPaintColorSnippetDataAsset* ColorSnippetDataAsset) {

    if (!ColorSnippetDataAsset) return TMap<FString, FRVPDPGroupColorSnippetInfo>();


    TMap<FString, FRVPDPGroupColorSnippetInfo> groupSnippetsInDataAsset;

    for (auto& groupSnippets : GroupSnippetsAndAssociatedMeshes) {

        if (groupSnippets.Value.ColorSnippetDataAssetStoredOn.Get() == ColorSnippetDataAsset)
            groupSnippetsInDataAsset.Add(groupSnippets.Key, groupSnippets.Value);
    }


    return groupSnippetsInDataAsset;
}


//-------------------------------------------------------

// Get Child Snippets Info Associated Group Snippet ID

TMap<FString, FRVPDPStoredColorSnippetInfo> UVertexPaintColorSnippetRefs::GetChildSnippetInfosAssociatedWithGroupSnippetID(const UObject* WorldContextObject, const FString& GroupSnippetID) {

    if (!WorldContextObject) return TMap<FString, FRVPDPStoredColorSnippetInfo>();
    if (GroupSnippetID.IsEmpty()) return TMap<FString, FRVPDPStoredColorSnippetInfo>();
    if (!GroupSnippetsAndAssociatedMeshes.Contains(GroupSnippetID)) return TMap<FString, FRVPDPStoredColorSnippetInfo>();


   //  UE_LOG(LogTemp, Warning, TEXT("UVertexPaintFunctionLibrary::GetAllColorSnippetsUnderGroupSnippet(WorldContextObject, GroupSnippetID): %i  -  group snippet: %s"), UVertexPaintFunctionLibrary::GetAllColorSnippetsUnderGroupSnippetAsString(WorldContextObject, GroupSnippetID).Num(), *GroupSnippetID);

    TMap<FString, FRVPDPStoredColorSnippetInfo> associatedChildGroupSnippetInfos;

    for (const FString& childColorSnippets : UVertexPaintFunctionLibrary::GetAllColorSnippetsUnderGroupSnippetAsString(WorldContextObject, GroupSnippetID)) {


        TSoftObjectPtr<UObject> objectAssociatedWithSnippet = UVertexPaintFunctionLibrary::GetColorSnippetReferenceDataAsset(WorldContextObject)->GetObjectFromSnippetID(childColorSnippets);


        TMap<TSoftObjectPtr<UObject>, FRVPDPColorSnippetReferenceDataInfo> colorSnippetTMapToUse;

        if (Cast<UStaticMesh>(objectAssociatedWithSnippet.Get()))
            colorSnippetTMapToUse = UVertexPaintFunctionLibrary::GetColorSnippetReferenceDataAsset(WorldContextObject)->StaticMeshesColorSnippets;

        else if (Cast<USkeletalMesh>(objectAssociatedWithSnippet.Get()))
            colorSnippetTMapToUse = UVertexPaintFunctionLibrary::GetColorSnippetReferenceDataAsset(WorldContextObject)->SkeletalMeshesColorSnippets;


        FRVPDPColorSnippetReferenceDataInfo colorSnippetRefDataInfo = colorSnippetTMapToUse.FindRef(objectAssociatedWithSnippet);

        if (colorSnippetRefDataInfo.ColorSnippetsStorageInfo.Num() > 0) {

            FRVPDPStoredColorSnippetInfo storedColorSnippetInfo = colorSnippetRefDataInfo.ColorSnippetsStorageInfo.FindRef(childColorSnippets);

            if (storedColorSnippetInfo.IsPartOfAGroupSnippet) {

                associatedChildGroupSnippetInfos.Add(childColorSnippets, storedColorSnippetInfo);
            }
        }
    }

    return associatedChildGroupSnippetInfos;
}


//-------------------------------------------------------

// Get If Components Match Group Child Snippets

bool UVertexPaintColorSnippetRefs::CheckAndGetTheComponentsThatMatchGroupChildSnippets(const UObject* WorldContextObject, const FString& GroupSnippetID, TArray<UPrimitiveComponent*> MeshComponents, TMap<FString, UPrimitiveComponent*>& ChildSnippetsAndMatchingMeshes) {

    ChildSnippetsAndMatchingMeshes.Empty();

    if (!WorldContextObject) return false;
    if (MeshComponents.Num() == 0) return false;
    if (GroupSnippetID.IsEmpty()) return false;


    FVector meshesCenterPoint = FVector(ForceInitToZero);
    FVector totalForwardVectors = FVector(ForceInitToZero);

    // NOTE That the way we calculate this has to match the way when we store the relative Location, where we set a transform and then inverse transform the Location etc. 
    for (UPrimitiveComponent* groupSnippetMesh : MeshComponents) {

        if (IsValid(groupSnippetMesh)) {

            meshesCenterPoint += groupSnippetMesh->GetComponentLocation();
            totalForwardVectors += groupSnippetMesh->GetForwardVector();
        }
    }

    meshesCenterPoint /= MeshComponents.Num();
    totalForwardVectors /= MeshComponents.Num();

    FTransform groupWorldTransformToCheckRelativeSpace;
    groupWorldTransformToCheckRelativeSpace.SetLocation(meshesCenterPoint);
    groupWorldTransformToCheckRelativeSpace.SetRotation(totalForwardVectors.ToOrientationRotator().Quaternion());

    TMap<FString, FRVPDPStoredColorSnippetInfo> childSnippetsAssociatedWithGroup = GetChildSnippetInfosAssociatedWithGroupSnippetID(WorldContextObject, GroupSnippetID);


    for (auto& childSnippetInfo : childSnippetsAssociatedWithGroup) {

        for (UPrimitiveComponent* groupSnippetMesh : MeshComponents) {

            if (!groupSnippetMesh) continue;


            if (UStaticMeshComponent* staticMeshComponent = Cast<UStaticMeshComponent>(groupSnippetMesh)) {

                if (childSnippetInfo.Value.ObjectColorSnippetBelongsTo != staticMeshComponent->GetStaticMesh())
                    continue;
            }

            else if (auto skeletalMeshComponent = Cast<USkeletalMeshComponent>(groupSnippetMesh)) {

#if ENGINE_MAJOR_VERSION == 4

                if (childSnippetInfo.Value.ObjectColorSnippetBelongsTo != skeletalMeshComponent->SkeletalMesh)
                    continue;

#elif ENGINE_MAJOR_VERSION == 5

#if ENGINE_MINOR_VERSION == 0

                if (childSnippetInfo.Value.ObjectColorSnippetBelongsTo != skeletalMeshComponent->SkeletalMesh)
                    continue;

#else

                if (childSnippetInfo.Value.ObjectColorSnippetBelongsTo != skeletalMeshComponent->GetSkeletalMeshAsset())
                    continue;
#endif
#endif
            }

           // UE_LOG(LogTemp, Warning, TEXT("groupWorldTransformToCheckRelativeSpace: %s  -  groupSnippetMesh->GetComponentLocation(): %s"), *groupWorldTransformToCheckRelativeSpace.ToString(), *groupSnippetMesh->GetComponentLocation().ToString());

            FVector currentRelativeLocationToGroupTransform = UKismetMathLibrary::InverseTransformLocation(groupWorldTransformToCheckRelativeSpace, groupSnippetMesh->GetComponentLocation());

            // If the same Relative Location to the Groups Center Point as it was when stored, used to associate a tag to a certain mesh and that the Group Snippet should look as intended for this mesh. This is so in case one of the groups meshes is rotated wrong, or in a completely different Location then we won't be able to associate tag and the group snippet won't get painted since it won't look as intended. 
            if (UKismetMathLibrary::EqualEqual_VectorVector(currentRelativeLocationToGroupTransform, childSnippetInfo.Value.RelativeLocationToGroupCenterPoint, .01f)) {

                const FVector directionFromCenterToMesh = UKismetMathLibrary::GetDirectionUnitVector(meshesCenterPoint, groupSnippetMesh->GetComponentLocation());
                const float dotProductFromCenterToMeshForwardVector = FVector::DotProduct(directionFromCenterToMesh, groupSnippetMesh->GetForwardVector());

                // If essentially the same dot product to the center point as when snippet was created, i.e. same relative rotation. 
                if (UKismetMathLibrary::NearlyEqual_FloatFloat(dotProductFromCenterToMeshForwardVector, childSnippetInfo.Value.DotProductToGroupCenterPoint, 0.001f)) {

                    ChildSnippetsAndMatchingMeshes.Add(childSnippetInfo.Key, groupSnippetMesh);
                    break;
                }

                else {

                    // UE_LOG(LogTemp, Warning, TEXT("Dot Product: Fail: storedDotProductToGroupCenterPoint: %f"), childSnippetInfo.Value.DotProductToGroupCenterPoint);
                }
            }
            
            else {

                // UE_LOG(LogTemp, Warning, TEXT("Location Fail: currentRelativeLocationToGroupTransform: %s  -  storedRelativeLocationToGroupCenterPoint: %s"), *currentRelativeLocationToGroupTransform.ToString(), *childSnippetInfo.Value.RelativeLocationToGroupCenterPoint.ToString());
            }
        }
    }

    // If we got the right amount of matching meshes with the correct Location etc. 
    if (childSnippetsAssociatedWithGroup.Num() > 0 && ChildSnippetsAndMatchingMeshes.Num() > 0)
        return (childSnippetsAssociatedWithGroup.Num() == ChildSnippetsAndMatchingMeshes.Num());
    else
        return false;
}


//-------------------------------------------------------

// Remove Snippet Object

void UVertexPaintColorSnippetRefs::RemoveSnippetObject(const UObject* Object) {

    if (StaticMeshesColorSnippets.Contains(Object)) {

        StaticMeshesColorSnippets.Remove(Object);
    }

    if (SkeletalMeshesColorSnippets.Contains(Object)) {

        SkeletalMeshesColorSnippets.Remove(Object);
    }
}


//-------------------------------------------------------

// Remove Color Snippet

void UVertexPaintColorSnippetRefs::RemoveColorSnippet(const FString& SnippetID) {

    if (SnippetID.Len() <= 0) return;


    TSoftObjectPtr<UObject> mesh = GetObjectFromSnippetID(SnippetID);

    if (Cast<USkeletalMesh>(mesh.Get())) {

        if (SkeletalMeshesColorSnippets.Contains(mesh.Get())) {

            // If there are more then only removes the specifc snippet reference
            if (SkeletalMeshesColorSnippets.FindRef(mesh.Get()).ColorSnippetsStorageInfo.Num() > 1) {

                FRVPDPColorSnippetReferenceDataInfo snippetWithDataAssets = SkeletalMeshesColorSnippets.FindRef(mesh.Get());

                snippetWithDataAssets.ColorSnippetsStorageInfo.Remove(SnippetID);

                SkeletalMeshesColorSnippets.Add(mesh.Get(), snippetWithDataAssets);
            }
            else {

                // If this is the last one then removes the entire TMap for this object. 
                SkeletalMeshesColorSnippets.Remove(mesh.Get());
            }
        }
    }

    else if (Cast<UStaticMesh>(mesh.Get())) {

        if (StaticMeshesColorSnippets.Contains(mesh.Get())) {

            // If there are more then only removes the specifc snippet reference
            if (StaticMeshesColorSnippets.FindRef(mesh.Get()).ColorSnippetsStorageInfo.Num() > 1) {

                FRVPDPColorSnippetReferenceDataInfo snippetWithDataAssets = StaticMeshesColorSnippets.FindRef(mesh.Get());

                snippetWithDataAssets.ColorSnippetsStorageInfo.Remove(SnippetID);

                StaticMeshesColorSnippets.Add(mesh.Get(), snippetWithDataAssets);
            }
            else {

                // If this is the last one then removes the entire TMap for this object. 
                StaticMeshesColorSnippets.Remove(mesh.Get());
            }
        }
    }
}


//-------------------------------------------------------

// Contains Color Snippet

bool UVertexPaintColorSnippetRefs::ContainsColorSnippet(FString SnippetID, bool OptionalHasToBeStoredInDataAsset, UVertexPaintColorSnippetDataAsset* DataAsset) {

    if (SnippetID.Len() <= 0) return false;


    TSoftObjectPtr<UObject> mesh = GetObjectFromSnippetID(SnippetID);

    if (Cast<USkeletalMesh>(mesh.Get())) {

        if (SkeletalMeshesColorSnippets.Contains(mesh.Get())) {

            if (OptionalHasToBeStoredInDataAsset)
                return (SkeletalMeshesColorSnippets.FindRef(mesh.Get()).ColorSnippetsStorageInfo.FindRef(SnippetID).ColorSnippetDataAssetStoredOn.Get() == DataAsset);

            else
                return true;
        }
    }

    else if (Cast<UStaticMesh>(mesh.Get())) {

        if (StaticMeshesColorSnippets.Contains(mesh.Get())) {

            if (OptionalHasToBeStoredInDataAsset)
                return (StaticMeshesColorSnippets.FindRef(mesh.Get()).ColorSnippetsStorageInfo.FindRef(SnippetID).ColorSnippetDataAssetStoredOn.Get() == DataAsset);
            else
                return true;
        }
    }

    else if (GroupSnippetsAndAssociatedMeshes.Contains(SnippetID)) {

        if (OptionalHasToBeStoredInDataAsset)
            return (GroupSnippetsAndAssociatedMeshes.FindRef(SnippetID).ColorSnippetDataAssetStoredOn.Get() == DataAsset);
        else
            return true;
    }

    return false;
}
