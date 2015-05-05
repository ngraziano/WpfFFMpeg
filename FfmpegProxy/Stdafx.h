// stdafx.h�: fichier Include pour les fichiers Include syst�me standard,
// ou les fichiers Include sp�cifiques aux projets qui sont utilis�s fr�quemment,
// et sont rarement modifi�s

#pragma once
#include <msclr\marshal.h>
#include <inttypes.h>

namespace FFMpeg
{
	extern "C"
	{
		#pragma comment( lib, "avformat" )
		#include <libavformat\avformat.h>
		#pragma comment( lib, "avcodec" )
		#include <libavcodec\avcodec.h>
		#pragma comment( lib, "avutil" )
		#include <libavutil\avutil.h>
		#pragma comment( lib, "swscale" )
		#include <libswscale\swscale.h>
	}
}