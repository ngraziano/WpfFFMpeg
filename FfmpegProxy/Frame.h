#pragma once

namespace FfmpegProxy
{

public
	ref class Frame
	{
	private:
		bool isDisposed;
		FFMpeg::SwsContext* img_convert_ctx;
		internal : FFMpeg::AVFrame* avFrame;

	public:
		Frame();
		~Frame();
		!Frame();

		property int Height
		{
			int get() { return avFrame->height; }
		}

		property int Width
		{
			int get() { return avFrame->width; }
		}

		void CopyToBuffer(System::IntPtr buffer, int linesize);
	};
}