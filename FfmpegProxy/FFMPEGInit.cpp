#include "stdafx.h"
#include "FFMPEGInit.h"

using namespace FfmpegProxy;
using namespace FFMpeg;
using namespace System;
using namespace msclr::interop;

void FFMPEGInit::InitFFMPEG()
{
	if (!isFFMPEGInit)
	{
		isFFMPEGInit = true;

		av_register_all();

		av_log_set_callback(reinterpret_cast<void (*)(void*, int, const char*, va_list)>(System::Runtime::InteropServices::Marshal::GetFunctionPointerForDelegate(logDelegate).ToPointer()));
		av_log_set_level(AV_LOG_DEBUG);
		//av_log_set_level(AV_LOG_VERBOSE);

		av_log(NULL, AV_LOG_VERBOSE, "Setup log callback done\n");

		avcodec_register_all();

		avformat_network_init();

		
	}
}

void FFMPEGInit::UnInitFFMPEG()
{

	if (isFFMPEGInit)
	{
		isFFMPEGInit = false;
		av_log_set_callback(nullptr);
		avformat_network_deinit();
	}
}

void FFMPEGInit::callbackLogFFMPEG(void* ptr, int level, const char* fmt, va_list vl)
{
	if ((level == AV_LOG_DEBUG && FFMPEGlogger->IsDebugEnabled)
		|| (level == AV_LOG_VERBOSE && FFMPEGlogger->IsDebugEnabled)
		|| (level == AV_LOG_INFO && FFMPEGlogger->IsInfoEnabled)
		|| (level == AV_LOG_WARNING && FFMPEGlogger->IsWarnEnabled)
		|| (level == AV_LOG_ERROR && FFMPEGlogger->IsErrorEnabled)
		|| (level == AV_LOG_FATAL && FFMPEGlogger->IsFatalEnabled))
	{
		char line[1024];
		static int print_prefix = 1;
		av_log_format_line(ptr, level, fmt, vl, line, sizeof(line), &print_prefix);
		String ^ message = marshal_as<String ^>(line);

		switch (level)
		{
			case AV_LOG_DEBUG:
			case AV_LOG_VERBOSE:
				FFMPEGlogger->Debug(message);
				break;
			case AV_LOG_INFO:
				FFMPEGlogger->Info(message);
				break;
			case AV_LOG_WARNING:
				FFMPEGlogger->Warn(message);
				break;
			case AV_LOG_ERROR:
				FFMPEGlogger->Error(message);
				break;
			case AV_LOG_FATAL:
				FFMPEGlogger->Fatal(message);
				break;
			default:
				FFMPEGlogger->WarnFormat("Unknown level {0} for message : {1}", level, message);
				break;
		}
	}
}

String ^ FFMPEGInit::GetErrorString(int errorNumber)
{
	char buffer[AV_ERROR_MAX_STRING_SIZE];
	return marshal_as<String ^>(av_make_error_string(buffer, AV_ERROR_MAX_STRING_SIZE, errorNumber));
}