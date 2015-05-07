#include "stdafx.h"
#include "Packet.h"


using namespace FFMpeg;
using namespace FfmpegProxy;

Packet::Packet()
{
	isDisposed = false;
	avPacket = new AVPacket();
	av_init_packet(avPacket);
}

/* Free managed and unmanaged object */
Packet::~Packet()
{
	if (isDisposed)
		return;

	this->!Packet();
}

/* Free unmanaged object */
Packet::!Packet()
{
	av_free_packet(avPacket);
	delete avPacket;
}

Packet^ Packet::GetNullPacket(int streamNumber)
{
	auto nullPacket = gcnew Packet();
	nullPacket->avPacket->data = nullptr;
	nullPacket->avPacket->size = 0;
	nullPacket->avPacket->stream_index = streamNumber;
	return nullPacket;
}