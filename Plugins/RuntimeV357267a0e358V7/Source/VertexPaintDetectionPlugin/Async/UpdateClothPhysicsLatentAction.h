// Copyright 2022-2025 Alexander Floden, Alias Alex River. All Rights Reserved. 

#include "VertexPaintBaseLatentAction.h"


struct FRVPDPUpdateClothPhysicsAsyncTask;

class FUpdateClothPhysicsLatentAction : public FVertexPaintBaseLatentAction {

public:


    FUpdateClothPhysicsLatentAction(const FLatentActionInfo& LatentInfo, FRVPDPUpdateClothPhysicsAsyncTask* InTask)
        : FVertexPaintBaseLatentAction(LatentInfo), UpdateClothPhysicsAsyncTask(InTask) {

        // Initialization for this specific latent action child
    }

    virtual void UpdateOperation(FLatentResponse& Response) override {

        FVertexPaintBaseLatentAction::UpdateOperation(Response);

        // Update logic for this specific Latent Action child
    }


#if WITH_EDITOR
    virtual FString GetDescription() const override
    {
        return TEXT("Update Cloth Physics Based on Existing Colors Latent Action");
    }
#endif

private:
    
    FRVPDPUpdateClothPhysicsAsyncTask* UpdateClothPhysicsAsyncTask;
};