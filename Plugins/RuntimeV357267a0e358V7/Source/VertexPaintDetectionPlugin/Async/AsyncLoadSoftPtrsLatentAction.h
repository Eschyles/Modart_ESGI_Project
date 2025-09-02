// Copyright 2022-2025 Alexander Floden, Alias Alex River. All Rights Reserved. 

#include "VertexPaintBaseLatentAction.h"



class FAsyncLoadSoftPtrsLatentAction : public FVertexPaintBaseLatentAction {

public:

    
    FAsyncLoadSoftPtrsLatentAction(const FLatentActionInfo& LatentInfo)
        : FVertexPaintBaseLatentAction(LatentInfo) {

        // Initialization for this specific latent action child
    }

    virtual void UpdateOperation(FLatentResponse& Response) override {

        FVertexPaintBaseLatentAction::UpdateOperation(Response);

        // Update logic for this specific Latent Action child
    }
    

#if WITH_EDITOR
    virtual FString GetDescription() const override
    {
        return TEXT("Async Load Soft Object Ptrs Latent Action");
    }
#endif
};