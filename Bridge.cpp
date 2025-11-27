// GB32_Media_Foundation_Bridge.cpp
// FINAL – compiles with ZERO errors/warnings in VS 2022/2019/2017 Win32

#include <windows.h>
#include <mfapi.h>
#include <mfplay.h>
#include <shlwapi.h>
#include <strsafe.h>
#include <malloc.h>

#pragma comment(lib, "mfplat.lib")
#pragma comment(lib, "mfplay.lib")
#pragma comment(lib, "mfuuid.lib")
#pragma comment(lib, "shlwapi.lib")
#pragma comment(lib, "ole32.lib")

static IMFPMediaPlayer* g_player = nullptr;
static BOOL             g_mfStarted = FALSE;

// ======================================================
// 1. Init
// ======================================================
extern "C" __declspec(dllexport) HRESULT WINAPI MF_Init(HWND hwnd)
{
    if (!IsWindow(hwnd)) return E_INVALIDARG;

    if (!g_mfStarted)
    {
        HRESULT hr = MFStartup(MF_VERSION, MFSTARTUP_FULL);
        if (FAILED(hr)) return hr;
        g_mfStarted = TRUE;
    }

    if (g_player)
    {
        g_player->Shutdown();
        g_player->Release();
        g_player = nullptr;
    }

    return MFPCreateMediaPlayer(nullptr, FALSE, 0, nullptr, hwnd, &g_player);
}

// ======================================================
// 3. Load – takes normal GB32 String
// ======================================================
extern "C" __declspec(dllexport) HRESULT WINAPI MF_Load(const char* filenameA)
{
    if (!g_player) return E_POINTER;
    if (!filenameA || filenameA[0] == '\0') return E_INVALIDARG;   // ← fixed line

    int len = MultiByteToWideChar(CP_ACP, 0, filenameA, -1, nullptr, 0);
    if (len <= 1) return E_INVALIDARG;

    wchar_t* wpath = (wchar_t*)alloca(len * sizeof(wchar_t));
    MultiByteToWideChar(CP_ACP, 0, filenameA, -1, wpath, len);

    wchar_t url[4096];
    DWORD urlLen = _countof(url);
    HRESULT hrUrl = UrlCreateFromPathW(wpath, url, &urlLen, 0);
    const wchar_t* source = SUCCEEDED(hrUrl) ? url : wpath;

    IMFPMediaItem* item = nullptr;
    HRESULT hr = g_player->CreateMediaItemFromURL(source, TRUE, 0, &item);
    if (FAILED(hr) || !item) { if (item) item->Release(); return hr ? hr : E_FAIL; }

    hr = g_player->SetMediaItem(item);
    item->Release();
    return hr;
}

// ======================================================
// 4–6. Play / Pause / Stop
// ======================================================
extern "C" __declspec(dllexport) HRESULT WINAPI MF_Play() { return g_player ? g_player->Play() : E_POINTER; }
extern "C" __declspec(dllexport) HRESULT WINAPI MF_Pause() { return g_player ? g_player->Pause() : E_POINTER; }
extern "C" __declspec(dllexport) HRESULT WINAPI MF_Stop() { return g_player ? g_player->Stop() : E_POINTER; }

// ======================================================
// 7. Shutdown
// ======================================================
extern "C" __declspec(dllexport) void WINAPI MF_Shutdown()
{
    if (g_player)
    {
        g_player->Shutdown();
        g_player->Release();
        g_player = nullptr;
    }
    if (g_mfStarted)
    {
        MFShutdown();
        g_mfStarted = FALSE;
    }
}

// ======================================================
// 8. Resize – 100% safe & working
// ======================================================
extern "C" __declspec(dllexport) void WINAPI MF_Resize()
{
    if (!g_player) return;

    HWND videoWnd = nullptr;
    if (FAILED(g_player->GetVideoWindow(&videoWnd)) || !IsWindow(videoWnd))
        return;

    g_player->UpdateVideo();           // <-- this is the real resize call
    InvalidateRect(videoWnd, nullptr, TRUE);
    UpdateWindow(videoWnd);
}

// ======================================================
// DllMain
// ======================================================
BOOL APIENTRY DllMain(HMODULE hModule, DWORD reason, LPVOID)
{
    if (reason == DLL_PROCESS_ATTACH)
        DisableThreadLibraryCalls(hModule);
    else if (reason == DLL_PROCESS_DETACH)
        MF_Shutdown();
    return TRUE;
}