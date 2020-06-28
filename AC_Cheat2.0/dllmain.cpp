// dllmain.cpp : Defines the entry point for the DLL application.
#include "pch.h"

// dllmain.cpp : Defines the entry point for the DLL application.

#include <thread>
#include <math.h>
#include <vector>
#include <iostream>
#include <GLFW\glfw3.h>
#include "Player.h"
#define good 0501AE8
HMODULE base = GetModuleHandle(L"ac_client.exe");
auto EntityOffset = DWORD(base) + 0x10F4F8;
auto OnlinePlayers = DWORD(base) + 0x10F500;
//auto viewMatrixOffset = DWORD(base) + 0x10A400;
auto viewMatrixOffset = (DWORD)0x0501AE8;
//auto viewMatrixOffset = (DWORD)0x0501AA8;

void* pLocalPlayer = (void*)0x0509b74;

DWORD playerStruct;
DWORD oHookAddress; 
BYTE* HookAddress;
HMODULE ModuleBase;
HDC hdcAC;
HBRUSH color;


inline void initGLDraw()
{
    glPushMatrix();
    GLfloat tmp_viewport[4];
    glGetFloatv(GL_VIEWPORT, tmp_viewport);

    glViewport(0, 0, tmp_viewport[2], tmp_viewport[3]);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0, tmp_viewport[2], tmp_viewport[3], 0, -1, 1);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
}

void DrawOutline(float x, float y, float width, float height, float lineWidth, GLfloat r, GLfloat g, GLfloat b )
{
    
    initGLDraw();
    glLineWidth(lineWidth);
    glBegin(GL_LINE_STRIP);
    glColor3f(r, g, b);
    glVertex2f(x, y);
    glVertex2f(x + width, y);
    glVertex2f(x + width, y + height);
    glVertex2f(x, y + height);
    glVertex2f(x, y);
    glPopMatrix();
    glEnd();
}




typedef BOOL(__stdcall* hkdwglSwapBuffers)(HDC hDc);

hkdwglSwapBuffers i_wglSwapBuffers;

bool IsEntityValid(Player* i)
{
    if (i) {
        if (i->PlayerState == 0x4E4AC0) return true;
    }
    return false;
}




int WorldToScreen(vec3 pos, vec3* screen, float matrix[16], int windowWidth, int windowHeight) // 3D to 2D
{
    //Matrix-vector Product, multiplying world(eye) coordinates by projection matrix = clipCoords
    vec4 clipCoords;
    clipCoords.x = pos.x * matrix[0] + pos.y * matrix[4] + pos.z * matrix[8] + matrix[12];
    clipCoords.y = pos.x * matrix[1] + pos.y * matrix[5] + pos.z * matrix[9] + matrix[13];
    clipCoords.z = pos.x * matrix[2] + pos.y * matrix[6] + pos.z * matrix[10] + matrix[14];
    clipCoords.w = pos.x * matrix[3] + pos.y * matrix[7] + pos.z * matrix[11] + matrix[15];

    if (clipCoords.w < 0.1f)
        return 0;

    //perspective division, dividing by clip.W = Normalized Device Coordinates
    vec3 NDC;
    NDC.x = clipCoords.x / clipCoords.w;
    NDC.y = clipCoords.y / clipCoords.w;
    NDC.z = clipCoords.z / clipCoords.w;

    //Transform to window coordinates
    screen->x = (windowWidth / 2 * NDC.x) + (NDC.x + windowWidth / 2);
    screen->y = -(windowHeight / 2 * NDC.y) + (NDC.y + windowHeight / 2);
    return 1;
}

int getOnlinePlayers()
{
    auto Online = (int*)((DWORD)OnlinePlayers);

    return (int)(*Online) - 1; //not online so Players - myself
}

void drawthread() {

    int targets = getOnlinePlayers();

    ViewMatrix* viewmatrix = (ViewMatrix*)(DWORD)(viewMatrixOffset);
    EntityList* list = (EntityList*)*(DWORD*)(EntityOffset);

    vec3 headscreenpos, footscreenpos;
    float flHeight, flWidth;
    for (int i = 0; i < targets; i++)
    {
        if (!list || !IsEntityValid(list->PlayerPointer[i])) continue;
      
        WorldToScreen({ list->PlayerPointer[i]->headposx, list->PlayerPointer[i]->headposy,list->PlayerPointer[i]->headposz }, &headscreenpos, viewmatrix->matrix, 800, 600);
        WorldToScreen({ list->PlayerPointer[i]->footposx, list->PlayerPointer[i]->footposy,list->PlayerPointer[i]->footposz }, &footscreenpos, viewmatrix->matrix, 800, 600);
        flHeight = abs(footscreenpos.y - headscreenpos.y);
        flWidth = flHeight / 2.4F;

        DrawOutline(footscreenpos.x, headscreenpos.y, flWidth, flHeight, 2, 255, 0, 0);
    }
}

BOOL __stdcall hookSwapbuff(_In_ HDC hDc)
{
    drawthread();
    return i_wglSwapBuffers(hDc);
}


 
bool Detour(BYTE* address, BYTE* dst, const uintptr_t size)
{
    if (size < 5) return false;
    DWORD oldprotect;
    VirtualProtect(address, size, PAGE_EXECUTE_READWRITE, &oldprotect);
    uintptr_t reladdr = dst - address - 5;
    *address = 0xE9;
    *(uintptr_t*)(address + 1) = reladdr;
    VirtualProtect(address, size, oldprotect, &oldprotect);
    return true;
}

BYTE* Trap(BYTE* address, BYTE* dst, const uintptr_t size) {
    if (size < 5) return 0;
    BYTE* holder = (BYTE*)VirtualAlloc(0, size, MEM_COMMIT | MEM_RESERVE, 0x40);

    memcpy_s(holder, size, address, size);
    uintptr_t holderreladdr = address - holder - 5;
    *(holder + size) = 0xE9;
    *(uintptr_t*)((uintptr_t)holder + size + 1) = holderreladdr;
    Detour(address, dst, size);
    return holder;
}

int dot_product(float vector_a[], float vector_b[]) {
    int product = 0;
    for (int i = 0; i < 3; i++)
        product = product + vector_a[i] * vector_b[i];
    return product;
}

HMODULE WINAPI CheatThread(HMODULE hModule)
{
    AllocConsole();
    FILE* f;    
    freopen_s(&f, "CONOUT$", "w", stdout);


    i_wglSwapBuffers = (hkdwglSwapBuffers)GetProcAddress(GetModuleHandle(L"opengl32.dll"), "wglSwapBuffers");
    i_wglSwapBuffers = (hkdwglSwapBuffers)Trap((BYTE*)i_wglSwapBuffers, (BYTE*)hookSwapbuff, 5);


    

    //Use local player pointer to set playerStruct
    HWND gameWindow = FindWindow(0, L"AssaultCube");
    hdcAC = GetDC(gameWindow);
   

    memcpy(&playerStruct, pLocalPlayer, sizeof(DWORD));


    


   // CreateThread(nullptr, 0, (LPTHREAD_START_ROUTINE)hops, inAir, 0, nullptr);


    //Hook 
    //1. Write Jmp to tramp Write offset (Do pageprotection)
    //2. Tramp save flags regs JMP to function execution
    //3. Come back to tramp restore regs/flags return to normal exec


    std::cout << "Done" << std::endl;

    
   
   
    fclose(f);
    FreeConsole();
    return 0;
}

BOOL APIENTRY DllMain(HMODULE hModule,
    DWORD  ul_reason_for_call,
    LPVOID lpReserved
    )
{
    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:
    {
        CloseHandle(CreateThread(nullptr, 0, (LPTHREAD_START_ROUTINE)CheatThread, hModule, 0, nullptr));


    }
    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
    case DLL_PROCESS_DETACH:
        break;
    }
    return TRUE;
}
/*
HANDLE Setup()
{

}

int changeHealth()
{
    HANDLE baseaddr = GetModuleHandle(NULL);

}
*/


