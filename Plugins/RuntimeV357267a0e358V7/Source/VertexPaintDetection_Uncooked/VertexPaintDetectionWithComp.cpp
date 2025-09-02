// Copyright 2022-2025 Alexander Floden, Alias Alex River. All Rights Reserved. 


#include "VertexPaintDetectionWithComp.h"


#define LOCTEXT_NAMESPACE "RuntimeVertexPaintAndDetection"

//-------------------------------------------------------

// Expand Node - Should be overriden by children with their specific pin checks

void UVertexPaintDetectionWithComp::ExpandNode(class FKismetCompilerContext& CompilerContext, UEdGraph* SourceGraph) {

	Super::ExpandNode(CompilerContext, SourceGraph);


	if (FindPinChecked(TEXT("runtimeVertexPaintAndDetectionComponent"))->LinkedTo.Num() == 0) {

		CompilerContext.MessageLog.Error(*LOCTEXT("vertexPaintComponent Missing Input!", "Paint / Detect Jobs must have a valid Runtime Vertex Paint And Detection Component assigned to vertexPaintComponent.").ToString());

		BreakAllNodeLinks();
		return;
	}
}

#undef LOCTEXT_NAMESPACE