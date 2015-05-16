#pragma once

namespace FfmpegProxy
{
	using namespace System;
	/// <summary>
	/// Represent an FFMPEG frame. 
	/// </summary>
	public ref class Frame
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
		
		/// <summary>
		/// Gets the height.
		/// </summary>
		/// <value>
		/// The height.
		/// </value>
		property int Height
		{
			int get() { return avFrame->height; }
		}

		/// <summary>
		/// Gets the width.
		/// </summary>
		/// <value>
		/// The width.
		/// </value>
		property int Width
		{
			int get() { return avFrame->width; }
		}

		/// <summary>
		/// Gets the best effort time stamp.
		/// </summary>
		/// <value>
		/// The best effort time stamp.
		/// </value>
		property Int64 BestEffortTimeStamp
		{
			Int64 get() { return FFMpeg::av_frame_get_best_effort_timestamp(avFrame); }
		}

		property int ReapeatPict
		{
			int get() { return avFrame->repeat_pict; }
		}

	};
}