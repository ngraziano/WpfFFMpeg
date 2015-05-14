#pragma once

namespace FfmpegProxy
{
	using namespace System;
public
	ref class Frame
	{
	private:
		bool isDisposed;
	internal:
		FFMpeg::AVFrame* avFrame;
		operator FFMpeg::AVFrame*() { return avFrame; };

	public:
		Frame();
		~Frame();
		!Frame();
		
		void UnrefBuffer();
		
		property int Height
		{
			int get() { return avFrame->height; }
		}

		property int Width
		{
			int get() { return avFrame->width; }
		}

		property Int64 BestEffortTimeStamp
		{
			Int64 get() { return FFMpeg::av_frame_get_best_effort_timestamp(avFrame); }
		}
	};
}