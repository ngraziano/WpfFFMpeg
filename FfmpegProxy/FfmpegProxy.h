// FfmpegProxy.h
#pragma once

#include "NewFrameEventArgs.h"
#include "Packet.h"

using namespace System;

namespace FfmpegProxy
{
	using namespace System::Collections::Concurrent;
	using namespace System::Collections::Generic;
	using namespace System::Runtime::InteropServices;
	using namespace System::Threading;


	[UnmanagedFunctionPointerAttribute(CallingConvention::Cdecl)]
	delegate int InterruptAVDelegate(void* ptr);


public
	ref class FFMPEGProxy
	{
	private:
		static log4net::ILog ^ LOG = log4net::LogManager::GetLogger(System::Reflection::MethodBase::GetCurrentMethod()->DeclaringType);

		bool isDisposed;
		FFMpeg::AVFormatContext* avContext;
		int videoStreamIndex;
		FFMpeg::AVCodecContext* videoCodecContext;
		int64_t timeBaseStreamVideo;
		FFMpeg::SwsContext* img_convert_ctx;


		initonly InterruptAVDelegate^ interruptDelegate;
		initonly CancellationTokenSource ^ stopSource;
		initonly ManualResetEventSlim ^ loopEnded;

		int InterruptCallback(void* ptr);
		void FrameReaderLoop();
		void PacketLoop();
		void FrameDecodeLoop();

		initonly BlockingCollection<Packet^>^ packetQueue;
		initonly BlockingCollection<Frame^>^ frameQueue;

		initonly Dictionary<String^,String^>^ optionsDictionary;

		

	public:
		FFMPEGProxy();
		!FFMPEGProxy();
		~FFMPEGProxy();
		void Open(String ^ uri);
		void CopyToBuffer(Frame^ frame,System::IntPtr buffer, int linesize);
		double GuessAspectRatio(Frame ^frame);

		property IDictionary<String^,String^>^ Options 
		{
			IDictionary<String^,String^>^ get() { return optionsDictionary; }
		}

		event EventHandler<NewFrameEventArgs ^> ^ NewFrame;
	};
}
