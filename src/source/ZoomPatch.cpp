/*
 * Widescreen patch for Die Siedler: Aufbruch der Kulturen by zocker_160
 *
 * This source code is licensed under GPL-v2
 *
 */

#include <Windows.h>
#include <sstream>
#include <iostream>
#include "ZoomPatch.h"

/* memory values */

/* RoC / AdK (34688) */
memoryPTR MaxZoomPTRSR = {
    0x0048CD54,
    2,
    { 0x50, 0x2E4 }
};
memoryPTR CurrZoomPTRSR = {
    0x0048CD54,
    2,
    { 0x50, 0x2E0 }
};
memoryPTR WorldObjectPTRSR = {
    0x0048CD54,
    1,
    { 0x50 }
};

memoryPTR MaxZoomPTR = {
    0x00485754,
    2,
    { 0x50, 0x2E4 }
};
memoryPTR CurrZoomPTR = {
    0x00485754,
    2,
    { 0x50, 0x2E0 }
};
memoryPTR WorldObjectPTR = {
    0x00485754,
    1,
    { 0x50 }
};

/* AdK Banner */
DWORD ADKBannerURL1SR = 0x104513;
DWORD ADKBannerURL2SR = 0x104592;
DWORD ADKBannerURL3SR = 0x104602;

DWORD ADKBannerURL1 = 0x1046C3;
DWORD ADKBannerURL2 = 0x104742;
DWORD ADKBannerURL3 = 0x1047B2;

DWORD ADKSecuromString      = 0x3F9;
DWORD ADKGameVersionAddrSR  = 0x495800;
DWORD ADKGameVersionAddr    = 0x48E1A0;

/*###################################*/

// reading and writing stuff / helper functions and other crap

/* update memory protection and read with memcpy */
void protectedRead(void* dest, void* src, int n) {
    DWORD oldProtect = 0;
    VirtualProtect(dest, n, PAGE_EXECUTE_READWRITE, &oldProtect);
    memcpy(dest, src, n);
    VirtualProtect(dest, n, oldProtect, &oldProtect);
}
/* read from address into read buffer of length len */
bool readBytes(void* read_addr, void* read_buffer, int len) {
    // compile with "/EHa" to make this work
    // see https://stackoverflow.com/questions/16612444/catch-a-memory-access-violation-in-c
    try {
        protectedRead(read_buffer, read_addr, len);
        return true;
    }
    catch (...) {
        return false;
    }
}
/* write patch of length len to destination address */
void writeBytes(void* dest_addr, void* patch, int len) {
    protectedRead(dest_addr, patch, len);
}

/* fiddle around with the pointers */
HMODULE getBaseAddress() {
    return GetModuleHandle(NULL);
}
DWORD* calcAddress(DWORD appl_addr) {
    return (DWORD*)((DWORD)getBaseAddress() + appl_addr);
}
DWORD* tracePointer(memoryPTR* patch) {
    DWORD* location = calcAddress(patch->base_address);

    for (int i = 0; i < patch->total_offsets; i++) {
        location = (DWORD*)(*location + patch->offsets[i]);
    }
    return location;
}

float calcAspectRatio(int horizontal, int vertical) {
    if (horizontal != 0 && vertical != 0) {
        return (float)horizontal / (float)vertical;
    }
    else {
        return -1.0f;
    }
}

/* other helper functions and stuff */
bool IsKeyPressed(int vKey) {
    /* some bitmask trickery because why not */
    return GetAsyncKeyState(vKey) & 0x8000;
}

void GetDesktopResolution(int& horizontal, int& vertical)
{
    RECT desktop;
    // Get a handle to the desktop window
    const HWND hDesktop = GetDesktopWindow();
    // Get the size of screen to the variable desktop
    GetWindowRect(hDesktop, &desktop);
    // The top left corner will have coordinates (0,0)
    // and the bottom right corner will have coordinates
    // (horizontal, vertical)
    horizontal = desktop.right;
    vertical = desktop.bottom;
}
void GetDesktopResolution2(int& hor, int& vert) {
    hor = GetSystemMetrics(SM_CXSCREEN);
    vert = GetSystemMetrics(SM_CYSCREEN);
}

void showMessage(float val) {
    std::cout << "DEBUG: " << val << "\n";
    return;
    std::stringstream ss;
    ss << "Debug: " << val;
    MessageBoxA(NULL, (LPCSTR)ss.str().c_str(), "ZoomPatch by zocker_160", MB_OK);
}
void showMessage(int val) {
    std::cout << "DEBUG: " << val << "\n";
    return;
    std::stringstream ss;
    ss << "Debug: " << val;
    MessageBoxA(NULL, (LPCSTR)ss.str().c_str(), "ZoomPatch by zocker_160", MB_OK);
}
void showMessage(LPCSTR val) {
    std::cout << "DEBUG: " << val << "\n";
    //MessageBoxA(NULL, val, "ZoomPatch by zocker_160", MB_OK);
}
void startupMessage() {
    std::cout << "ZoomPatch by zocker_160 - Version: v" << version_maj << "." << version_min << "\n";
    std::cout << "Debug mode enabled!\n";
    std::cout << "Waiting for application startup...\n";
}

bool isSecuromVersion() {
    char secStr[] = "securom";
    char secStrExe[8];

    memcpy(secStrExe, (char*)calcAddress(ADKSecuromString), 7);
    secStrExe[7] = 0;

    showMessage("SecuromStrExe:");
    showMessage(secStrExe);

    return strcmp(secStrExe, secStr) == 0;
}

bool checkSettlersII(char* versionString) {
    char versionSettlers[9] = "Version:"; // All Settlers II remakes have this
    char tVersion[9];

    if (!readBytes(versionString, tVersion, 8))
        return false;
    //memcpy(tVersion, versionString, 8);
    tVersion[8] = 0;

    showMessage(tVersion);

    if (strcmp(tVersion, versionSettlers) != 0)
        return false;
    else
        return true;
}

bool checkSettlersVersion(char* versionString) {
    char versionADK[15] = "Version: 34688"; // Rise of Cultures / Aufbruch der Kulturen
    char gameVersion[15];

    if (!readBytes(versionString, gameVersion, 14))
        return false;
    //memcpy(gameVersion, versionString, 14);
    gameVersion[14] = 0;

    showMessage(gameVersion);

    if (strcmp(gameVersion, versionADK) != 0)
        return false;
    else
        return true;
}

bool calcNewZoomValue(int& hor, int& vert, float& zoom_value, bool wideview) {
    GetDesktopResolution2(hor, vert);
    float aspr = calcAspectRatio(hor, vert);
    if (aspr > 0.0f && aspr <= 20.0f) {
        /* maxZoomValue will be set depending on the Aspect Ratio of the screen */
        if (wideview) {
            if (aspr < 1.5f) {
                zoom_value = 5.0f;
                return true;
            }
            else if (aspr < 1.9f) {
                zoom_value = 6.0f;
                return true;
            }
            else if (aspr < 2.2f) {
                zoom_value = 7.0f;
                return true;
            }
            else if (aspr < 2.5f) {
                zoom_value = 8.0f;
                return true;
            }
            else if (aspr >= 2.5f) {
                zoom_value = 9.0f;
                return true;
            }
            else {
                return false;
            }
        }
        else {
            if (aspr < 1.5f) {
                zoom_value = 4.0f;
                return true;
            }
            else if (aspr < 1.9f) {
                zoom_value = 5.0f;
                return true;
            }
            else if (aspr < 2.2f) {
                zoom_value = 6.0f;
                return true;
            }
            else if (aspr < 2.5f) {
                zoom_value = 7.0f;
                return true;
            }
            else if (aspr < 3.2f) {
                zoom_value = 8.0f;
                return true;
            }
            else if (aspr >= 3.2f) {
                zoom_value = 9.0f;
                return true;
            }
            else {
                return false;
            }
        }
        
        return true;
    }
    else {
        return false;
    }
}

void AdKBannerPatch(threadData* tData, bool bSecurom) {
    char** sURL1;
    char** sURL2;
    char** sURL3;

    if (bSecurom) {
        sURL1 = (char**)((DWORD)getBaseAddress() + ADKBannerURL1SR);
        sURL2 = (char**)((DWORD)getBaseAddress() + ADKBannerURL2SR);
        sURL3 = (char**)((DWORD)getBaseAddress() + ADKBannerURL3SR);
    } else {
        sURL1 = (char**)((DWORD)getBaseAddress() + ADKBannerURL1SR + 0x1B0);
        sURL2 = (char**)((DWORD)getBaseAddress() + ADKBannerURL2SR + 0x1B0);
        sURL3 = (char**)((DWORD)getBaseAddress() + ADKBannerURL3SR + 0x1B0);
    }

    showMessage(*sURL1);
    showMessage(*sURL2);
    showMessage(*sURL3);

    char* sURL1_new = tData->BannerURL_1;
    char* sURL2_new = tData->BannerURL_2;
    char* sURL3_new = tData->BannerURL_3;

    showMessage("Patching Banner URLs...");

    writeBytes(sURL1, &sURL1_new, 4);
    writeBytes(sURL2, &sURL2_new, 4);
    writeBytes(sURL3, &sURL3_new, 4);

    showMessage(*sURL1);
    showMessage(*sURL2);
    showMessage(*sURL3);
}

int MainLoop(memoryPTR& WorldObjectPTR,
    memoryPTR& MaxZoomPTR,
    memoryPTR& CurrZoomPTR,
    threadData* tData
    ) {
    float* worldObj;
    float* maxZoom;

    int hor;
    int ver;
    float newZoomValue = 4.0f; // 4 is the default zoom value
    float* zoomStep_p = &tData->ZoomIncrement;

    memoryPTR zoomIncr = {
        0x20D00E + 0x02,
        0,
        { 0x0 }
    };
    memoryPTR zoomDecr = {
        0x20CFE4 + 0x02,
        0,
        { 0x0 }
    };

    /*
    {
        DWORD tmp[4];
        readBytes(tracePointer(&zoomIncr), tmp, 4);
        std::cout << "ZoomStep " << tData->ZoomIncrement << "\n";
        std::cout << "ZoomStep " << &tData->ZoomIncrement << "\n";
        writeBytes(tracePointer(&zoomIncr), &zoomStep_p, 4);
        writeBytes(tracePointer(&zoomDecr), &zoomStep_p, 4);
    }
    */

    /* check if WorldObject does exist */
    {
        float* tmp = (float*)calcAddress(WorldObjectPTR.base_address);
        if (*tmp < 0)
            return 0;
    }

    for (;; Sleep(1000)) {
        worldObj = (float*)(tracePointer(&WorldObjectPTR));

        if (tData->bDebugMode) {
            if (IsKeyPressed(VK_F3)) {
                int t_hor = 0;
                int t_ver = 0;
                GetDesktopResolution2(t_hor, t_ver);
                std::stringstream ss;
                ss << "DEBUG:  xRes: " << t_hor << " yRes: " << t_ver;
                ss << " ASPR: " << calcAspectRatio(t_hor, t_ver);
                ss << " MaxZoomValue: " << newZoomValue;
                std::cout << ss.str() << "\n";
                //MessageBoxA(NULL, (LPCSTR)ss.str().c_str(), "ZoomPatch by zocker_160", MB_OK);
            }
            if (IsKeyPressed(VK_F6)) {
                if (*worldObj != 0) {
                    //*(float*)(tracePointer(&CurrZoomPTR)) += 1.0f;
                    showMessage(*(float*)(tracePointer(&MaxZoomPTR)));
                }
            }
            if (IsKeyPressed(VK_F7)) {
                showMessage(*(float*)(tracePointer(&CurrZoomPTR)));
            }
            if (IsKeyPressed(VK_F8)) {
                std::cout << "World address: " << worldObj << "\n";
            }
        }

        /* Zoom hack */
        if (*worldObj != 0) {
            maxZoom = (float*)(tracePointer(&MaxZoomPTR));
            if (calcNewZoomValue(hor, ver, newZoomValue, tData->bWideView) && *maxZoom != newZoomValue) {
                if (tData->bWideView)
                    showMessage("WideViewMode enabled");
                else
                    showMessage("WideViewMode disabled");
                *maxZoom = newZoomValue;
            }
        }

    }
}

int MainEntry(threadData* tData) {
    FILE* f;
    char* sADK;
    bool bSecurom;

    if (tData->bDebugMode) {
        AllocConsole();
        freopen_s(&f, "CONOUT$", "w", stdout);
        startupMessage();
    }   

    bSecurom = isSecuromVersion();

    /* Check if securom is present */
    if (bSecurom) {
        showMessage("Securom version detected");
    } else {
        showMessage("no Securom found");
    }

    /* Banner patch has to be done as early as possible */
    if (tData->bBannerPatch)
        AdKBannerPatch(tData, bSecurom);

    /* wait a bit for the application to start up (might crash otherwise) */
    Sleep(2000);

    /* check if gameVersion is supported */
    if (bSecurom)
        sADK = (char*)((DWORD)getBaseAddress() + ADKGameVersionAddrSR);
    else
        sADK = (char*)((DWORD)getBaseAddress() + ADKGameVersionAddr);

    bool bSupported = false;

    for (int i = 0; i < 8; i++) {
        if (checkSettlersII(sADK) && checkSettlersVersion(sADK)) {
            showMessage("Found ADK version.");
            bSupported = true;

            /* Check if securom is present */
            if (bSecurom) {
                showMessage("Securom version detected");
                return MainLoop(WorldObjectPTRSR, MaxZoomPTRSR, CurrZoomPTRSR, tData);
            } else {
                showMessage("no Securom found");
                return MainLoop(WorldObjectPTR, MaxZoomPTR, CurrZoomPTR, tData);
            }
        }
        showMessage("retrying...");
        Sleep(2000);
    }
    if (!bSupported)
        showMessage("Game version not supported!");
    return 0;
}

DWORD WINAPI ZoomPatchThread(LPVOID param) {
    return MainEntry(reinterpret_cast<threadData*>(param));
}

// rename to "DllMain" if you want to use this
bool APIENTRY DllMain_alt(  HMODULE hModule,
                            DWORD  ul_reason_for_call,
                            LPVOID lpReserved
                          ) {
    switch (ul_reason_for_call) {
    case DLL_PROCESS_ATTACH:
        //SetProcessDPIAware();
        CreateThread(0, 0, ZoomPatchThread, hModule, 0, 0);
        return 0;
    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
    case DLL_PROCESS_DETACH:
        FreeLibraryAndExitThread(hModule, 0);
        return 0;
    }
    return 0;
}
