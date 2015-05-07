#include "stdafx.h"
#include "Frame.h"

using namespace FFMpeg;
using namespace FfmpegProxy;

Frame::Frame(void)
{
	isDisposed = false;
	avFrame = av_frame_alloc();
}

/* Free managed and unmanaged object */
Frame::~Frame()
{
	if (isDisposed)
		return;

	this->!Frame();
}

/* Free unmanaged object */
Frame::!Frame()
{
	pin_ptr<AVFrame*> framePtr = &avFrame;
	av_frame_free(framePtr);

	sws_freeContext(img_convert_ctx);
	img_convert_ctx = nullptr;
}

void Frame::CopyToBuffer(System::IntPtr buffer, int linesize)
{
	img_convert_ctx = sws_getCachedContext(img_convert_ctx,
										   Width, Height, AV_PIX_FMT_YUV420P,
										   Width, Height, PIX_FMT_RGB32,
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

	sws_scale(img_convert_ctx, avFrame->data, avFrame->linesize, 0, avFrame->height,
			  data,
			  linesizeArray);
}