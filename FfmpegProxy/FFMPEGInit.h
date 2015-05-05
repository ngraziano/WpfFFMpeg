#pragma once

namespace FfmpegProxy
{
	using namespace System;
	using namespace System::Runtime::InteropServices;

	[UnmanagedFunctionPointerAttribute(CallingConvention::Cdecl)]
	delegate void LogFFMPEGDelegate(void* ptr, int level, const char* fmt, va_list vl);

	ref class FFMPEGInit
	{
	private:
		static bool isFFMPEGInit = false;
		static log4net::ILog ^ FFMPEGlogger = log4net::LogManager::GetLogger("FFMPEG");
		static LogFFMPEGDelegate ^ logDelegate = gcnew LogFFMPEGDelegate(&FFMPEGInit::callbackLogFFMPEG);
		static void callbackLogFFMPEG(void* ptr, int level, const char* fmt, va_list vl);

	public:
		static void InitFFMPEG();
		static void UnInitFFMPEG();
		static String ^ GetErrorString(int errorNumber);
	};
}
