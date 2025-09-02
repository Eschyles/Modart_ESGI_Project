// Copyright 2022-2025 Alexander Floden, Alias Alex River. All Rights Reserved. 


#include "VertexPaintDetectionOnMeshNode.h"


#define LOCTEXT_NAMESPACE "RuntimeVertexPaintAndDetection"

//-------------------------------------------------------

// Expand Node - Should be overriden by children with their specific pin checks

void UVertexPaintDetectionOnMeshNode::ExpandNode(class FKismetCompilerContext& CompilerContext, UEdGraph* SourceGraph) {

	Super::ExpandNode(CompilerContext, SourceGraph);

	if (FindPinChecked(TEXT("meshComponent"))->LinkedTo.Num() == 0) {

		CompilerContext.MessageLog.Error(*LOCTEXT("meshComponent Missing Input!", "Must have a valid Static, Skeletal, Spline, Dynamic or  Geometry Collection Component assigned to meshComponent.").ToString());

		BreakAllNodeLinks();
		return;
	}
}

#undef LOCTEXT_NAMESPACE