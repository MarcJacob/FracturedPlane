// Win32_Main.cpp
// Main Entry point of program when running on Windows.

#include "ServerFramework/ServerPlatform.h"
#include "iostream"

#define WIN32_LEAN_AND_MEAN
#include "Windows.h"

#define USE_CONSOLE

#ifndef USE_CONSOLE
// String stream that will get flushed to System Debug Output regularly.
std::stringstream DebugStream;

void OutputDebugStreamToSystemDebug()
{
	OutputDebugString(DebugStream.str().c_str());
}

#endif

static bool bServerShutdown = false;
static SYSTEM_INFO SystemInfo;

void EndProgram()
{
	// Make sure to flush all debug before exiting process.

#ifndef USE_CONSOLE

	OutputDebugStreamToSystemDebug();

#endif

	// Free up all system resources
	// ...
}

extern bool Win32Net_Init();
extern void Win32Net_RegisterPlatformFunctions(ServerPlatform& Platform);

// Initialize Win32 Platform & return ServerPlatform data structure. Returns whether initialization was successful.
bool Win32_InitPlatform(ServerPlatform& OutPlatform)
{
	std::cout << "Initializing Win32 Platform...\n";

	// Read static System Info
	GetSystemInfo(&SystemInfo);
	
	// #TODO(Marc): Should be read from config file.
	
	// Prepare Memory footprint
	// Provide the server with minimum memory it needs.
	// #TODO(Marc): Should be able to pass Platform Capabilities to the Server code so it can return both whether Server
	// can run at all, and if it can, how well and how much memory it should take up.
	// In turn the resources the server requests should depend on what we want it to do (game world size, max player
	// count...)
	size_t RequestedServerMemory = GetRequiredServerMemory();

	// Allocate memory at minimum application address.
	OutPlatform.Memory = static_cast<byte*>(VirtualAlloc(nullptr, RequestedServerMemory, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE));	
	OutPlatform.MemorySize = RequestedServerMemory;

	if (nullptr == OutPlatform.Memory)
	{
		std::cerr << "Failed to allocate memory when initializing Win32 Platform. Error Code:" << GetLastError() << "\n";
		return false;
	}
	
	// Prepare Data Storage

	// Prepare Threading Services

	// Prepare Network Services & Data
	if (!Win32Net_Init())
	{
		std::cerr << "Failed to initialize Win32 Networking.\n";
		return false;
	}
	Win32Net_RegisterPlatformFunctions(OutPlatform);

	return true;
}

int WINAPI WinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPSTR lpCmdLine, _In_ int nShowCmd)
{

#ifdef USE_CONSOLE
	// Allocate Output Console and link its out stream to standard and error out.
	{
		AllocConsole();

		FILE* ConsoleOut;
		freopen_s(&ConsoleOut, "CONOUT$", "w", stdout);
		freopen_s(&ConsoleOut, "CONOUT$", "w", stderr);
	}
#else
	// Pipe std::cout and std::cerr into Debug Stream.
	{
		std::cout.rdbuf(DebugStream.rdbuf());
		std::cerr.rdbuf(DebugStream.rdbuf());
	}
#endif

	
	// Platform Initialization
	ServerPlatform Platform;
	if (!Win32_InitPlatform(Platform))
	{
		std::cerr << "Win32 Platform Initialization failed ! Ending program...\n";
		EndProgram();
		return 1;
	}

	// Initialize Time Tracking Data
	LARGE_INTEGER LastUpdateBeginCounter, CurrentUpdateBeginCounter;
	QueryPerformanceCounter(&LastUpdateBeginCounter);
	QueryPerformanceCounter(&CurrentUpdateBeginCounter);

	double dCounterFrequency = 0.f;
	LARGE_INTEGER iCounterFreq;
	{
		QueryPerformanceFrequency(&iCounterFreq);
		dCounterFrequency = static_cast<float>(iCounterFreq.QuadPart);
	}

	// Server Initialization - Call linked InitializeServer function and retrieve a GameServerPtr pointer (void*)
	// that can be passed to further Server Flow Control calls.
	std::cout << "Initializing Server...\n";
	GameServerPtr Server;
	if (!InitializeServer(Platform, Server))
	{
		std::cout << "Failed to initialize server. Shutting program down.\n";
		EndProgram();
		return 1;
	}

	// Platform & Server Main loop
	double UpdateDeltaTime = 0.f;
	while (!bServerShutdown)
	{
		// Catch window events
		MSG WindowMessage;
		while (PeekMessage(&WindowMessage, NULL, 0, 0, PM_REMOVE))
		{
			TranslateMessage(&WindowMessage);
			DispatchMessage(&WindowMessage);
		}
		
		QueryPerformanceCounter(&CurrentUpdateBeginCounter);
		int Delta = CurrentUpdateBeginCounter.QuadPart - LastUpdateBeginCounter.QuadPart;
		UpdateDeltaTime = static_cast<double>(Delta) / dCounterFrequency;
		
		// Enforce a time between updates of at least 1ms. We don't need such a larger level of precision for time updates on the main thread.
		if (Delta < iCounterFreq.QuadPart / 1000) 
		{
			// Sleeping and ensuring updates don't happen too often should help avoid pointlessly consuming power for a level of precision we do not require.
			Sleep(1);
			UpdateDeltaTime += 0.001;
		}
		UpdateServer(Server, UpdateDeltaTime);
		LastUpdateBeginCounter = CurrentUpdateBeginCounter;
	}

	// Cleanup & Shutdown
	ShutdownServer(Server, ShutdownReason::UNKNOWN);
	EndProgram();

#ifdef USE_CONSOLE
	system("pause");
#endif

	return 0;
}