// Copyright 2022-2025 Alexander Floden, Alias Alex River. All Rights Reserved. 

#pragma once

#include "CoreMinimal.h"
#include <Stats/Stats.h>
#include <Stats/Stats2.h>
#include <ProfilingDebugging/CpuProfilerTrace.h>

// Measures time spent in function called in
DECLARE_STATS_GROUP(TEXT("RuntimeVertexPaintDetection"), STATGROUP_RuntimeVertexPaintDetection, STATCAT_Advanced);

#if (UE_BUILD_DEVELOPMENT || UE_BUILD_DEBUG || WITH_EDITOR)
#define RVPDP_SCOPE_CYCLE_COUNTER(Stat) \
        DECLARE_SCOPE_CYCLE_COUNTER(TEXT(#Stat), Stat, STATGROUP_RuntimeVertexPaintDetection)

// This helps in capturing detailed performance data for the specified code block we run the macro in
#define RVPDP_CPUPROFILER_STR(NameStr) \
        TRACE_CPUPROFILER_EVENT_SCOPE_STR(NameStr)
#else
// Define empty macros for shipping builds to avoid compilation errors
#define RVPDP_SCOPE_CYCLE_COUNTER(Stat)
#define RVPDP_CPUPROFILER_STR(NameStr)
#endif
