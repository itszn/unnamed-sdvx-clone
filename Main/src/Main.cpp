#include "stdafx.h"
#include "Application.hpp"

#ifdef _WIN32
// Windows entry point
int32 __stdcall WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd)
{
	OutputDebugStringW(L"[Main] Creating new Application\n");

	new Application();

	OutputDebugStringW(L"[Main] New Application Created\n");

	String commandLine = Utility::ConvertToUTF8(GetCommandLineW());
	g_application->SetCommandLine(*commandLine);

	int32 ret = g_application->Run();

	OutputDebugStringW(L"[Main] Calling ~Application\n");
	delete g_application;

	OutputDebugStringW(L"[Main] WinMain ending\n");
	return ret;
}
#else
// Linux entry point
int main(int argc, char** argv)
{
	new Application();
	g_application->SetCommandLine(argc, argv);
	int32 ret = g_application->Run();
	delete g_application;
	return ret;
}
#endif
