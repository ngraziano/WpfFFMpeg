#include "stdafx.h"
#include "NewFrameEventArgs.h"

using namespace FfmpegProxy;

/// <summary>
/// Initializes a new instance of the <see cref="NewFrameEventArgs"/> class.
/// </summary>
/// <param name="newframe">The newframe.</param>
NewFrameEventArgs::NewFrameEventArgs(Frame ^ newframe)
{
	NewFrame = newframe;
}
