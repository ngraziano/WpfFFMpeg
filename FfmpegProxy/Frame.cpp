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
}


void Frame::UnrefBuffer()
{
	av_frame_unref(avFrame);
}