#pragma once

#include <windows.h>
#include <shobjidl_core.h>

#include <atomic>
#include <string>

// {FF24ACA0-1690-4E21-B970-D62DAEDD97B3}
extern "C" const GUID CLSID_GitHubDesktopCommand;

void DllAddRef();
void DllRelease();
long DllObjectCount();
HINSTANCE DllModule();

class GitHubDesktopCommand final : public IExplorerCommand, public IObjectWithSite
{
public:
    GitHubDesktopCommand();

    // IUnknown
    IFACEMETHODIMP QueryInterface(REFIID riid, void** object) override;
    IFACEMETHODIMP_(ULONG) AddRef() override;
    IFACEMETHODIMP_(ULONG) Release() override;

    // IExplorerCommand
    IFACEMETHODIMP GetTitle(IShellItemArray* items, PWSTR* name) override;
    IFACEMETHODIMP GetIcon(IShellItemArray* items, PWSTR* icon) override;
    IFACEMETHODIMP GetToolTip(IShellItemArray* items, PWSTR* infoTip) override;
    IFACEMETHODIMP GetCanonicalName(GUID* guidCommandName) override;
    IFACEMETHODIMP GetState(IShellItemArray* items, BOOL okToBeSlow, EXPCMDSTATE* cmdState) override;
    IFACEMETHODIMP Invoke(IShellItemArray* items, IBindCtx* bindContext) override;
    IFACEMETHODIMP GetFlags(EXPCMDFLAGS* flags) override;
    IFACEMETHODIMP EnumSubCommands(IEnumExplorerCommand** enumCommands) override;

    // IObjectWithSite
    IFACEMETHODIMP SetSite(IUnknown* site) override;
    IFACEMETHODIMP GetSite(REFIID riid, void** site) override;

private:
    ~GitHubDesktopCommand();

    std::wstring ResolveCurrentFolder(IShellItemArray* items) const;

    std::atomic<ULONG> refCount_{1};
    IUnknown* site_ = nullptr;
};
