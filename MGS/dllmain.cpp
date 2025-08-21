#include "minhook/MinHook.h"
#include "Utils.h"
#include "Hooking.h"

#pragma comment(lib, "minhook/minhook.lib")

using namespace std;
using namespace SDK;

DWORD InitThread(LPVOID)
{
    AllocConsole();
    FILE* fptr;
    freopen_s(&fptr, "CONOUT$", "w+", stdout);

    MH_Initialize();
    Log("Initializing MGS");

    Gamemode::InitHooks();
    Tick::InitHooks();
    PC::InitHooks();
    Misc::InitHooks();
    Abilities::InitHooks();
    Inventory::InitHooks();
    Phoebe::InitHooks();

    MH_EnableHook(MH_ALL_HOOKS);

    *(bool*)(InSDKUtils::GetImageBase() + 0x804B659) = false; //GIsClient
    *(bool*)(InSDKUtils::GetImageBase() + 0x804B65A) = true; //GIsServer

    Sleep(1000);

    UKismetSystemLibrary::ExecuteConsoleCommand(UWorld::GetWorld(), L"open Apollo_Terrain", nullptr);
    UWorld::GetWorld()->OwningGameInstance->LocalPlayers.Remove(0); 
   
    return 0;
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD Reason, LPVOID lpReserved)
{
    switch (Reason)
    {
    case DLL_PROCESS_ATTACH:
        CreateThread(0, 0, InitThread, 0, 0, 0);
        break;
    default:
        break;
    }
    return TRUE;
}