#include "VertexPaintDetectionLog.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Runtime/Launch/Resources/Version.h"


DEFINE_LOG_CATEGORY(RuntimeVertexColorPlugin);


void VertexPaintDetectionLog::PrintVertexPaintDetectionLog(const FRVPDPDebugSettings& DebugSettings, const FColor& ScreenTextColor, const FString& Text) {


    if (DebugSettings.PrintLogsToScreen && IsInGameThread()) {

        // 5.4 gets packaging error if not specifically const uobject
#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 4

        if (DebugSettings.WorldTaskWasCreatedIn.IsValid()) {

            if (const UObject* taskWorld = reinterpret_cast<const UObject*>(DebugSettings.WorldTaskWasCreatedIn.Get()))
                UKismetSystemLibrary::PrintString(taskWorld, Text, true, false, ScreenTextColor, DebugSettings.PrintLogsToScreen_Duration);
        }

#else

        if (DebugSettings.WorldTaskWasCreatedIn.IsValid()) {

            UKismetSystemLibrary::PrintString(DebugSettings.WorldTaskWasCreatedIn.Get(), Text, true, false, ScreenTextColor, DebugSettings.PrintLogsToScreen_Duration);
        }

#endif
    }

    if (DebugSettings.PrintLogsToOutputLog)
        UE_LOG(RuntimeVertexColorPlugin, Log, TEXT("%s"), *Text);
}