#include "stdafx.h"

#include "FfmpegProxy.h"
#include "FFMPEGInit.h"
#include "Frame.h"
#include "Packet.h"
#include "AvDictionaryMarshal.h"

using namespace FfmpegProxy;
using namespace FFMpeg;
using namespace msclr::interop;
using namespace System::Threading;
using namespace System::Collections::Concurrent;

FFMPEGProxy::FFMPEGProxy()
{
	isDisposed = false;

	// Init library if not already done
	FFMPEGInit::InitFFMPEG();

	// Manage stop
	stopSource = gcnew CancellationTokenSource();
	loopEnded = gcnew ManualResetEventSlim(true);
	interruptDelegate = gcnew InterruptAVDelegate(this,&FFMPEGProxy::InterruptCallback);

	// context
	avContext = avformat_alloc_context();
	packetQueue = gcnew BlockingCollection<Packet^>(100);
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

	delete packetQueue;

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
			LOG->WarnFormat("AVFORMAT OPEN INPUT : {0},{1}", result, FFMPEGInit::GetErrorString(result));
			return;
		}

		result = avformat_find_stream_info(avContext, nullptr);
		if (result)
		{
			LOG->WarnFormat("AVFORMAT FIND STREAM INFO : {0},{1}", result, FFMPEGInit::GetErrorString(result));
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

			auto packetTask = gcnew Tasks::Task(gcnew Action(this,&FFMPEGProxy::PacketLoop),stopToken, Tasks::TaskCreationOptions::LongRunning);
			//auto decodeTask = gcnew Tasks::Task(gcnew Action(this,&FFMPEGProxy::PacketLoop),stopToken, Tasks::TaskCreationOptions::LongRunning);
			packetTask->Start();
			FrameReaderLoop();
			packetTask->Wait();
		}
	}
	finally
	{
		loopEnded->Set();
	}
}

void FFMPEGProxy::PacketLoop()
{
	auto stopToken = stopSource->Token;
	bool eof = false;

	while (!eof && !stopToken.IsCancellationRequested)
	{
		Packet ^ packet = gcnew Packet();
		int result = av_read_frame(avContext, packet);
		if (result)
		{
			if(result == AVERROR_EOF)
			{
				eof = true;
				// ajoute un packet null en fin de chaque stream.
				packetQueue->Add(Packet::GetNullPacket(videoStreamIndex),stopToken);
				packetQueue->CompleteAdding();
			}
		}
		else
		{
			if (packet->StreamIndex == videoStreamIndex)
			{
				packetQueue->Add(packet,stopToken);
			}
			else
			{
				// detruit les packet non géré.
				delete packet;
			}
		}
	}

}

void FFMPEGProxy::FrameReaderLoop()
{
	Frame ^ oneFrame = gcnew Frame();
	auto stopToken = stopSource->Token;

	try
	{
		while (!packetQueue->IsCompleted && !stopToken.IsCancellationRequested)
		{
			Packet ^ packet = packetQueue->Take(stopToken);
			int gotPicture = 0; // Flag.
			auto test=videoCodecContext->refcounted_frames;
			int bytesUse = avcodec_decode_video2(videoCodecContext, oneFrame, &gotPicture, packet);

			if (gotPicture && !stopToken.IsCancellationRequested)
			{
				NewFrame(this, gcnew NewFrameEventArgs(oneFrame));
			}
			delete packet;
		}
	}
	catch(OperationCanceledException^)
	{
		LOG->Warn("Cancel operation.");
	}
	delete oneFrame;

}