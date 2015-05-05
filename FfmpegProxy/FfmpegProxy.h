// FfmpegProxy.h
#pragma once

#include "NewFrameEventArgs.h"

using namespace System;

namespace FfmpegProxy
{
public
ref class FFMPEGProxy
{
	private:
	bool isDisposed;
	FFMpeg::AVFormatContext* avContext;
	int videoStreamIndex;
	FFMpeg::AVCodecContext* videoCodecContext;
	
	System::Threading::CancellationTokenSource^ stopSource;
	System::Threading::ManualResetEventSlim^ loopEnded;

	public:
	FFMPEGProxy();
	!FFMPEGProxy();
	~FFMPEGProxy();
	void Open(String ^ uri);
	event EventHandler<NewFrameEventArgs ^> ^ NewFrame;
};
}
