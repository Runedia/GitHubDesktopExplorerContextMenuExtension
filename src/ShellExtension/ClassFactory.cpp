#include "ClassFactory.h"

#include "GitHubDesktopCommand.h"

#include <new>

ClassFactory::ClassFactory()
{
    DllAddRef();
}

ClassFactory::~ClassFactory()
{
    DllRelease();
}

IFACEMETHODIMP ClassFactory::QueryInterface(REFIID riid, void** object)
{
    if (!object)
    {
        return E_POINTER;
    }
    if (riid == IID_IUnknown || riid == IID_IClassFactory)
    {
        *object = static_cast<IClassFactory*>(this);
        AddRef();
        return S_OK;
    }
    *object = nullptr;
    return E_NOINTERFACE;
}

IFACEMETHODIMP_(ULONG) ClassFactory::AddRef()
{
    return ++refCount_;
}

IFACEMETHODIMP_(ULONG) ClassFactory::Release()
{
    ULONG count = --refCount_;
    if (count == 0)
    {
        delete this;
    }
    return count;
}

IFACEMETHODIMP ClassFactory::CreateInstance(IUnknown* outer, REFIID riid, void** object)
{
    if (!object)
    {
        return E_POINTER;
    }
    *object = nullptr;
    if (outer)
    {
        return CLASS_E_NOAGGREGATION;
    }

    auto* command = new (std::nothrow) GitHubDesktopCommand();
    if (!command)
    {
        return E_OUTOFMEMORY;
    }
    HRESULT hr = command->QueryInterface(riid, object);
    command->Release();
    return hr;
}

IFACEMETHODIMP ClassFactory::LockServer(BOOL lock)
{
    if (lock)
    {
        DllAddRef();
    }
    else
    {
        DllRelease();
    }
    return S_OK;
}
