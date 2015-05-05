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
	data[0] = (uint8_t*)buffer.ToPointer();
	data[1] = nullptr;
	data[2] = nullptr;
	data[3] = nullptr;
	data[4] = nullptr;
	data[5] = nullptr;
	data[6] = nullptr;
	data[7] = nullptr;

	int linesizeArray[AV_NUM_DATA_POINTERS];
	linesizeArray[0] = linesize;
	linesizeArray[1] = 0;
	linesizeArray[2] = 0;
	linesizeArray[3] = 0;
	linesizeArray[4] = 0;
	linesizeArray[5] = 0;
	linesizeArray[6] = 0;
	linesizeArray[7] = 0;

	sws_scale(img_convert_ctx, avFrame->data, avFrame->linesize, 0, avFrame->height,
			  data,
			  linesizeArray);
}