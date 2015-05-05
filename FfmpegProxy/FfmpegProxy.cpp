#include "stdafx.h"

#include "FfmpegProxy.h"
#include "FFMPEGInit.h"
#include "Frame.h"

using namespace FfmpegProxy;
using namespace FFMpeg;
using namespace msclr::interop;
using namespace System::Threading;

FFMPEGProxy::FFMPEGProxy()
{
	isDisposed = false;
	FFMPEGInit::InitFFMPEG();

	stopSource = gcnew CancellationTokenSource();
	loopEnded = gcnew ManualResetEventSlim(true);
	avContext = avformat_alloc_context();
	interruptDelegate = gcnew InterruptAVDelegate(this,&FFMPEGProxy::InterruptCallback);
	avContext->interrupt_callback.callback = reinterpret_cast<int (*)(void*)>(System::Runtime::InteropServices::Marshal::GetFunctionPointerForDelegate(interruptDelegate).ToPointer());
		
}

FFMPEGProxy::!FFMPEGProxy()
{
	if (videoCodecContext)
	{
		avcodec_close(videoCodecContext);
		videoCodecContext = nullptr;
	}
	pin_ptr<AVFormatContext*> contextPtr = &avContext;
	avformat_close_input(contextPtr);
}

FFMPEGProxy::~FFMPEGProxy()
{
	if (isDisposed)
		return;

	stopSource->Cancel();
	delete stopSource;
	loopEnded->Wait();
	delete loopEnded;

	this->!FFMPEGProxy();
}

int FFMPEGProxy::InterruptCallback(void *)
{
	// return 1 to stop the current ffmpeg call.
	if (stopSource->IsCancellationRequested)
		return 1;

	// Continue
	return 0;
}

void FFMPEGProxy::Open(String ^ uri)
{
	if (String::IsNullOrWhiteSpace(uri))
	{
		throw gcnew ArgumentNullException("uri");
	}

	marshal_context marshalContext;
	auto stopToken = stopSource->Token;
	loopEnded->Reset();
	try
	{
		const char* uriChar = marshalContext.marshal_as<const char*>(uri);
		pin_ptr<AVFormatContext*> contextPtr = &avContext;

		int result = avformat_open_input(contextPtr, uriChar, nullptr, nullptr);

		if (result)
		{
			Console::WriteLine("AVFORMAT OPEN INPUT : {0},{1}", result, FFMPEGInit::GetErrorString(result));
			return;
		}

		result = avformat_find_stream_info(avContext, nullptr);
		if (result)
		{
			Console::WriteLine("AVFORMAT FIND STREAM INFO : {0},{1}", result, FFMPEGInit::GetErrorString(result));
			return;
		}

		for (unsigned int i = 0; i < avContext->nb_streams; i++)
		{
			if (avContext->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO)
			{
				// Keep the video index.
				videoStreamIndex = i;

				// Get the Codec context associated with this stream.
				videoCodecContext = avContext->streams[i]->codec;
			}
		}

		if (videoCodecContext)
		{
			AVCodec* codec = avcodec_find_decoder(videoCodecContext->codec_id);
			result = avcodec_open2(videoCodecContext, codec, nullptr);
			if (result)
			{
				Console::WriteLine("AVCODEC OPEN : {0},{1}", result, FFMPEGInit::GetErrorString(result));
				return;
			}

			Frame ^ oneFrame = gcnew Frame();

			AVPacket packet;
			av_init_packet(&packet);
			bool eof = false;
			while (!eof && !stopToken.IsCancellationRequested)
			{
				result = av_read_frame(avContext, &packet);
				if (result)
				{
					eof = true;
				}
				else
				{
					if (packet.stream_index == videoStreamIndex)
					{
						int gotPicture = 0; // Flag.
						int bytesUse = avcodec_decode_video2(videoCodecContext, oneFrame->avFrame, &gotPicture, &packet);

						if (gotPicture && !stopToken.IsCancellationRequested)
						{
							NewFrame(this, gcnew NewFrameEventArgs(oneFrame));
						}
					}
					av_free_packet(&packet);
				}
			}

			delete oneFrame;
		}
	}
	finally
	{
		loopEnded->Set();
	}
}