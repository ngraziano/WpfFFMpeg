#include "stdafx.h"
#include "Frame.h"

using namespace FFMpeg;
using namespace FfmpegProxy;

/// <summary>
/// Initializes a new instance of the <see cref="Frame" /> class.
/// </summary>
Frame::Frame(void)
{
	isDisposed = false;
	avFrame = av_frame_alloc();
}

/// <summary>
/// Finalizes an instance of the <see cref="Frame"/> class.
/// </summary>
Frame::~Frame()
{
	if (isDisposed)
		return;

	this->!Frame();
}

/// <summary>
/// Free unmanaged object in the frame.
/// </summary>
/// <returns></returns>
Frame::!Frame()
{
	pin_ptr<AVFrame*> framePtr = &avFrame;
	av_frame_free(framePtr);
}


/// <summary>
/// Decrease reference count of the data buffer.
/// </summary>
void Frame::UnrefBuffer()
{
	av_frame_unref(avFrame);
}