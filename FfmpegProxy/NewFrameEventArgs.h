#pragma once

#include "Frame.h"

namespace FfmpegProxy {

	public ref class NewFrameEventArgs : System::EventArgs
	{

	public:
		NewFrameEventArgs(Frame^ newframe);
		property Frame^ NewFrame;

	};
}
