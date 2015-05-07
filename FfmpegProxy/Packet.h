#pragma once


namespace FfmpegProxy
{

	ref class Packet
	{
	private:
		bool isDisposed;
	internal:
		FFMpeg::AVPacket* avPacket;
		operator FFMpeg::AVPacket*() { return avPacket; };

	public:
		Packet();
		~Packet();
		!Packet();
		
		static Packet^ GetNullPacket(int streamNumber);

		property int StreamIndex { int get() {return avPacket->stream_index;} } 
	};
}
