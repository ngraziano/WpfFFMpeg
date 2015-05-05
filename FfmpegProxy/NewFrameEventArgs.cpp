#include "stdafx.h"
#include "NewFrameEventArgs.h"

using namespace FfmpegProxy;

NewFrameEventArgs::NewFrameEventArgs(Frame^ newframe)
{
	NewFrame =newframe;
}
