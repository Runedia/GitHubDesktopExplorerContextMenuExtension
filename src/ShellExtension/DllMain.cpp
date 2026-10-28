#include "ClassFactory.h"
#include "GitHubDesktopCommand.h"

#include <atomic>
#include <new>

// {FF24ACA0-1690-4E21-B970-D62DAEDD97B3}
extern "C" const GUID CLSID_GitHubDesktopCommand =
    {0xff24aca0, 0x1690, 0x4e21, {0xb9, 0x70, 0xd6, 0x2d, 0xae, 0xdd, 0x97, 0xb3}};

namespace
{
std::atomic<long> g_objectCount{0};
HINSTANCE g_module = nullptr;
}

void DllAddRef() { ++g_objectCount; }
void DllRelease() { --g_objectCount; }
long DllObjectCount() { return g_objectCount.load(); }
HINSTANCE DllModule() { return g_module; }

BOOL APIENTRY DllMain(HMODULE module, DWORD reason, LPVOID)
{
    if (reason == DLL_PROCESS_ATTACH)
    {
        g_module = module;
        DisableThreadLibraryCalls(module);
    }
    return TRUE;
}

STDAPI DllGetClassObject(REFCLSID clsid, REFIID riid, void** object)
{
    if (!object)
    {
        return E_POINTER;
    }
    *object = nullptr;
    if (clsid != CLSID_GitHubDesktopCommand)
    {
        return CLASS_E_CLASSNOTAVAILABLE;
    }

    auto* factory = new (std::nothrow) ClassFactory();
    if (!factory)
    {
        return E_OUTOFMEMORY;
    }
    HRESULT hr = factory->QueryInterface(riid, object);
    factory->Release();
    return hr;
}

STDAPI DllCanUnloadNow()
{
    return DllObjectCount() == 0 ? S_OK : S_FALSE;
}
