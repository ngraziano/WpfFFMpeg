// stdafx.h : fichier Include pour les fichiers Include système standard,
// ou les fichiers Include spécifiques aux projets qui sont utilisés fréquemment,
// et sont rarement modifiés

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