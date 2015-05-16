#include "stdafx.h"

#include "FfmpegProxy.h"
#include "FFMPEGInit.h"
#include "Frame.h"
#include "Packet.h"
#include "AvDictionaryMarshal.h"
#include <msclr\auto_handle.h>

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
	img_convert_ctx = nullptr;
	packetQueue = gcnew BlockingCollection<Packet^>(100);
	frameQueue = gcnew BlockingCollection<Frame^>(100);
	optionsDictionary = gcnew Dictionary<String^,String^>(); 
}

FFMPEGProxy::!FFMPEGProxy()
{
	if (videoCodecContext)
	{
		avcodec_close(videoCodecContext);
		videoCodecContext = nullptr;
	}


	sws_freeContext(img_convert_ctx);
	img_convert_ctx = nullptr;

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
	delete frameQueue;

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
		
		AVDictionary** optionPtr = marshalContext.marshal_as<AVDictionary**>(Options);

		int result = avformat_open_input(contextPtr, uriChar, nullptr, optionPtr);
		
		auto optionResul = marshal_as<IDictionary<String^,String^>^>(*optionPtr);

		for each (String^ var in optionResul->Keys)
		{
			Console::WriteLine(var);
		}
		
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
				// Time base in micro second
				timeBaseStreamVideo = 1000000 * av_q2d(avContext->streams[i]->time_base) ;

			}
		}

		if (videoCodecContext)
		{
			AVCodec* codec = avcodec_find_decoder(videoCodecContext->codec_id);
			videoCodecContext->refcounted_frames = 1;
			result = avcodec_open2(videoCodecContext, codec, nullptr);
			if (result)
			{
				Console::WriteLine("AVCODEC OPEN : {0},{1}", result, FFMPEGInit::GetErrorString(result));
				return;
			}

			msclr::auto_handle<Tasks::Task> packetTask (gcnew Tasks::Task(gcnew Action(this,&FFMPEGProxy::PacketLoop),stopToken, Tasks::TaskCreationOptions::LongRunning));
			msclr::auto_handle<Tasks::Task> decodeTask (gcnew Tasks::Task(gcnew Action(this,&FFMPEGProxy::FrameDecodeLoop),stopToken, Tasks::TaskCreationOptions::LongRunning));
			packetTask->Start();
			decodeTask->Start();
			FrameReaderLoop();
			decodeTask->Wait();
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

/// <summary>
/// Frames reader loop.
/// </summary>
void FFMPEGProxy::FrameReaderLoop()
{
	auto stopToken = stopSource->Token;
	// time of the last loop
	int64_t time = av_gettime_relative();
	// old presentation time
	int64_t oldPTS = 0;
	// last time to wait calculated
	int64_t timeToWait = 0;
	// correction delay
	int64_t correctDelay = 0;

	try
	{
		while (!frameQueue->IsCompleted && !stopToken.IsCancellationRequested)
		{
			int64_t newtime = av_gettime_relative();
			int64_t error = newtime - time - timeToWait; 
			// adjust correction delay by 1% of the error (100 frame to get the full error corrected : filter spike in error)
			correctDelay -= error / 100;
			time = newtime;


			Frame ^ oneFrame = frameQueue->Take(stopToken);
			
			// calculate the time to wait
			int64_t timeToWaitCorrected = 0;
			if(oldPTS != 0)
			{
				int64_t newtimeToWait;
				newtimeToWait = (oneFrame->BestEffortTimeStamp - oldPTS) * timeBaseStreamVideo; 
				// if frame is repeated
				newtimeToWait += timeBaseStreamVideo /2 * oneFrame->ReapeatPict;

				// prevent invalid time to wait value (must not increase of factor 10 and must be positive)
				if(newtimeToWait > 0 && (newtimeToWait < 10 * timeToWait || timeToWait == 0) )
					timeToWait = newtimeToWait;

				timeToWaitCorrected = timeToWait + correctDelay;
			}

			if (!stopToken.IsCancellationRequested)
			{
				//	Console::WriteLine(" Delay {0} Error : {2} , Correction {1} Queue {3}",timeToWait, correctDelay,error,frameQueue->Count);

				if(timeToWaitCorrected > 0)
				{
					av_usleep(timeToWaitCorrected);
				}
				else
				{
					correctDelay = - timeToWait;
				}

				NewFrame(this, gcnew NewFrameEventArgs(oneFrame));
			}

			oldPTS = oneFrame->BestEffortTimeStamp;


			delete oneFrame;
		}
	}
	catch(OperationCanceledException^)
	{
		LOG->Warn("Frame Reader Loop Cancel operation.");
	}
}

void FFMPEGProxy::FrameDecodeLoop()
{
	auto stopToken = stopSource->Token;

	try
	{
		while (!packetQueue->IsCompleted && !stopToken.IsCancellationRequested)
		{
			Frame ^ oneFrame = gcnew Frame();

			Packet ^ packet = packetQueue->Take(stopToken);
			int gotPicture = 0; // Flag.
			int bytesUse = avcodec_decode_video2(videoCodecContext, oneFrame, &gotPicture, packet);

			if (gotPicture)
			{
				frameQueue->Add(oneFrame,stopToken);
			}
			delete packet;
		}

		// empty buffer, need to be retest....
		int gotPicture = 0; // Flag.
		do
		{
			Frame ^ oneFrame = gcnew Frame();
			gotPicture = 0;
			int bytesUse = avcodec_decode_video2(videoCodecContext, oneFrame, &gotPicture, Packet::GetNullPacket(videoStreamIndex));

			if (gotPicture)
			{
				frameQueue->Add(oneFrame,stopToken);
			}
		}
		while(gotPicture);
		frameQueue->CompleteAdding();
	}
	catch(OperationCanceledException^)
	{
		LOG->Warn("Frame Decode Loop Cancel operation.");
	}
}

void FFMPEGProxy::CopyToBuffer(Frame^ frame,System::IntPtr buffer, int linesize)
{
	if(!frame)
		throw gcnew ArgumentNullException("frame");

	if(buffer == System::IntPtr::Zero)
		throw gcnew ArgumentNullException("buffer");

	if(linesize<=0)
		throw gcnew ArgumentOutOfRangeException("linesize",linesize,"linesize must be positive"); 

	img_convert_ctx = sws_getCachedContext(img_convert_ctx,
		frame->Width, frame->Height, videoCodecContext->pix_fmt,
		frame->Width, frame->Height, PIX_FMT_RGB32,
		SWS_BICUBIC,
		nullptr, nullptr, nullptr);

	uint8_t* data[AV_NUM_DATA_POINTERS];
	int linesizeArray[AV_NUM_DATA_POINTERS];

	for (int i = 0; i < AV_NUM_DATA_POINTERS; i++)
	{
		data[i] = 0;
		linesizeArray[i] = 0;
	}

	data[0] = (uint8_t*)buffer.ToPointer();
	linesizeArray[0] = linesize;

	sws_scale(img_convert_ctx, frame->avFrame->data, frame->avFrame->linesize, 0, frame->Height,
		data,
		linesizeArray);
}

double FFMPEGProxy::GuessAspectRatio(Frame ^frame)
{
	if(!frame)
		throw gcnew ArgumentNullException("frame");

	return  av_q2d(av_guess_sample_aspect_ratio(avContext,avContext->streams[videoStreamIndex],frame));
}
