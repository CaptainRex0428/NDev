// Fill out your copyright notice in the Description page of Project Settings.


#include "NDevMessage.h"

DEFINE_LOG_CATEGORY(NDev);

void MsgLog(const FString& Message, bool isWarning)
{
	
}

// Screen Debug

void ScreenMsg(const FString& Message, const FColor& Color)
{
	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(-1, 8.f, Color, Message);
	}
}

void ScreenMsgLog(const FString& Message, const FColor& Color)
{
	ScreenMsg(Message, Color);
	MsgLog(Message);
}

// Dialog Debug

EAppReturnType::Type DlgMsgLog(EAppMsgType::Type MsgType, const FString& Message, bool isWarning, bool isLog)
{
	if (isWarning)
	{
		FString title = TEXT("WARNING");
		if (isLog)
		{
			MsgLog(Message, isWarning);
		}
		return FMessageDialog::Open(MsgType, FText::FromString(Message), FText::FromString(title));
	}

	FString title = TEXT("TIP");
	if (isLog)
	{
		MsgLog(Message, isWarning);
	}
	return FMessageDialog::Open(MsgType, FText::FromString(Message), FText::FromString(title));
}

EAppReturnType::Type DlgMsg(EAppMsgType::Type MsgType, const FString& Message, bool isWarning)
{
	return DlgMsgLog(MsgType, Message, isWarning, false);
}

// Notification Debug

void NtfyMsg(const FString& Message)
{
	FNotificationInfo NotifyInfo(FText::FromString(Message));
	NotifyInfo.bUseLargeFont = true;
	NotifyInfo.FadeOutDuration = 7.f;

	FSlateNotificationManager::Get().AddNotification(NotifyInfo);
}

void NtfyMsgLog(const FString& Message, bool isWarning)
{
	NtfyMsg(Message);
	MsgLog(Message, isWarning);
}

void MsgLog(const FString& Message, ELogVerbosity::Type VerboseType)
{
	switch (VerboseType)
	{
	case ELogVerbosity::Error:
	{
		UE_LOG(NDev, Error, TEXT("%s"), *Message);
		break;
	}
	case ELogVerbosity::Warning:
	{
		UE_LOG(NDev, Warning, TEXT("%s"), *Message);
		break;
	}
	case ELogVerbosity::Display:
	{
		UE_LOG(NDev, Display, TEXT("%s"), *Message);
		break;
	}
	case ELogVerbosity::Fatal:
	{
		UE_LOG(NDev, Fatal, TEXT("%s"), *Message);
		break;
	}
	case ELogVerbosity::Verbose:
	{
		UE_LOG(NDev, Verbose, TEXT("%s"), *Message);
		break;
	}
	default:
		UE_LOG(NDev, Log, TEXT("%s"), *Message);
		break;
	}
}
