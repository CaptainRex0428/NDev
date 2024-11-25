// Copy right Turing DigiArt rvo0428@163.com

#pragma once

#include "CoreMinimal.h"

#include "Widgets/Notifications/SNotificationList.h"
#include "FrameWork/Notifications/NotificationManager.h"
#include "Logging/LogMacros.h"

DECLARE_LOG_CATEGORY_EXTERN(NDev, Verbose, All)

void MsgLog(const FString& Message, ELogVerbosity::Type VerboseType = ELogVerbosity::Log);

// Screen Debug

static void ScreenMsg(const FString& Message, const FColor& Color = FColor::Cyan);

static void ScreenMsgLog(const FString& Message, const FColor& Color = FColor::Cyan);

// Dialog Debug

static EAppReturnType::Type DlgMsgLog(EAppMsgType::Type MsgType, const FString& Message, bool isWarning = true, bool isLog = true);

static EAppReturnType::Type DlgMsg(EAppMsgType::Type MsgType, const FString& Message, bool isWarning = true);
