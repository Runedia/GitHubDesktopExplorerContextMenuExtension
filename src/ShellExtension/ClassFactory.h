#pragma once

#include <windows.h>
#include <unknwn.h>

#include <atomic>

class ClassFactory final : public IClassFactory
{
public:
    ClassFactory();

    IFACEMETHODIMP QueryInterface(REFIID riid, void** object) override;
    IFACEMETHODIMP_(ULONG) AddRef() override;
    IFACEMETHODIMP_(ULONG) Release() override;

    IFACEMETHODIMP CreateInstance(IUnknown* outer, REFIID riid, void** object) override;
    IFACEMETHODIMP LockServer(BOOL lock) override;

private:
    ~ClassFactory();

    std::atomic<ULONG> refCount_{1};
};
