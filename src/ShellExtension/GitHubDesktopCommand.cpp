#include "GitHubDesktopCommand.h"

#include "GitHubCli.h"
#include "GitRepository.h"
#include "Resource.h"

#include <shlobj.h>
#include <shlwapi.h>

namespace
{
std::wstring LoadResourceString(UINT id)
{
    const wchar_t* text = nullptr;
    int length = LoadStringW(DllModule(), id, reinterpret_cast<LPWSTR>(&text), 0);
    return length > 0 ? std::wstring(text, static_cast<size_t>(length)) : std::wstring();
}

HRESULT DuplicateString(const std::wstring& value, PWSTR* result)
{
    *result = nullptr;
    return SHStrDupW(value.c_str(), result);
}

std::wstring PathFromItem(IShellItem* item)
{
    PWSTR path = nullptr;
    std::wstring result;
    if (SUCCEEDED(item->GetDisplayName(SIGDN_FILESYSPATH, &path)) && path)
    {
        result = path;
    }
    CoTaskMemFree(path);
    return result;
}

// 사이트 체인(IShellBrowser -> IFolderView -> IPersistFolder2)으로 현재 탐색기 폴더를 얻는다.
std::wstring PathFromSite(IUnknown* site)
{
    if (!site)
    {
        return {};
    }

    IServiceProvider* services = nullptr;
    if (FAILED(site->QueryInterface(IID_PPV_ARGS(&services))))
    {
        return {};
    }

    std::wstring result;
    IShellBrowser* browser = nullptr;
    if (SUCCEEDED(services->QueryService(SID_STopLevelBrowser, IID_PPV_ARGS(&browser))))
    {
        IShellView* view = nullptr;
        if (SUCCEEDED(browser->QueryActiveShellView(&view)))
        {
            IFolderView* folderView = nullptr;
            if (SUCCEEDED(view->QueryInterface(IID_PPV_ARGS(&folderView))))
            {
                IPersistFolder2* persist = nullptr;
                if (SUCCEEDED(folderView->GetFolder(IID_PPV_ARGS(&persist))))
                {
                    PIDLIST_ABSOLUTE pidl = nullptr;
                    if (SUCCEEDED(persist->GetCurFolder(&pidl)))
                    {
                        PWSTR path = nullptr;
                        if (SUCCEEDED(SHGetNameFromIDList(pidl, SIGDN_FILESYSPATH, &path)) && path)
                        {
                            result = path;
                        }
                        CoTaskMemFree(path);
                        CoTaskMemFree(pidl);
                    }
                    persist->Release();
                }
                folderView->Release();
            }
            view->Release();
        }
        browser->Release();
    }
    services->Release();
    return result;
}
}

GitHubDesktopCommand::GitHubDesktopCommand()
{
    DllAddRef();
}

GitHubDesktopCommand::~GitHubDesktopCommand()
{
    if (site_)
    {
        site_->Release();
    }
    DllRelease();
}

IFACEMETHODIMP GitHubDesktopCommand::QueryInterface(REFIID riid, void** object)
{
    if (!object)
    {
        return E_POINTER;
    }
    *object = nullptr;
    if (riid == IID_IUnknown || riid == IID_IExplorerCommand)
    {
        *object = static_cast<IExplorerCommand*>(this);
    }
    else if (riid == IID_IObjectWithSite)
    {
        *object = static_cast<IObjectWithSite*>(this);
    }
    else
    {
        return E_NOINTERFACE;
    }
    AddRef();
    return S_OK;
}

IFACEMETHODIMP_(ULONG) GitHubDesktopCommand::AddRef()
{
    return ++refCount_;
}

IFACEMETHODIMP_(ULONG) GitHubDesktopCommand::Release()
{
    ULONG count = --refCount_;
    if (count == 0)
    {
        delete this;
    }
    return count;
}

IFACEMETHODIMP GitHubDesktopCommand::GetTitle(IShellItemArray*, PWSTR* name)
{
    return DuplicateString(LoadResourceString(IDS_TITLE), name);
}

IFACEMETHODIMP GitHubDesktopCommand::GetIcon(IShellItemArray*, PWSTR* icon)
{
    *icon = nullptr;
    // 아이콘 조회는 파일 존재 확인 외의 작업을 하지 않는다. 찾지 못하면 기본 Shell 아이콘을 쓴다.
    std::optional<std::wstring> cli = ghd::FindGitHubCli();
    if (cli)
    {
        std::optional<std::wstring> exe = ghd::FindGitHubDesktopExe(*cli);
        if (exe)
        {
            return DuplicateString(*exe + L",0", icon);
        }
    }
    return E_NOTIMPL;
}

IFACEMETHODIMP GitHubDesktopCommand::GetToolTip(IShellItemArray*, PWSTR* infoTip)
{
    return DuplicateString(LoadResourceString(IDS_TOOLTIP), infoTip);
}

IFACEMETHODIMP GitHubDesktopCommand::GetCanonicalName(GUID* guidCommandName)
{
    *guidCommandName = CLSID_GitHubDesktopCommand;
    return S_OK;
}

std::wstring GitHubDesktopCommand::ResolveCurrentFolder(IShellItemArray* items) const
{
    std::wstring path = PathFromSite(site_);
    if (!path.empty())
    {
        return path;
    }

    // 배경 메뉴에서는 보통 항목이 전달되지 않는다. 전달되었다면 단일 폴더 항목만 인정한다.
    DWORD count = 0;
    if (items && SUCCEEDED(items->GetCount(&count)) && count == 1)
    {
        IShellItem* item = nullptr;
        if (SUCCEEDED(items->GetItemAt(0, &item)))
        {
            path = PathFromItem(item);
            item->Release();
        }
    }
    return path;
}

IFACEMETHODIMP GitHubDesktopCommand::GetState(IShellItemArray* items, BOOL, EXPCMDSTATE* cmdState)
{
    // 상태 조회 경로에서는 파일 시스템 속성 한 번만 확인한다. 오류 UI도 표시하지 않는다.
    std::wstring folder = ResolveCurrentFolder(items);
    *cmdState = (!folder.empty() && ghd::HasGitEntry(folder)) ? ECS_ENABLED : ECS_HIDDEN;
    return S_OK;
}

IFACEMETHODIMP GitHubDesktopCommand::Invoke(IShellItemArray* items, IBindCtx*)
{
    std::wstring folder = ResolveCurrentFolder(items);
    if (folder.empty())
    {
        return S_OK;
    }

    UINT messageId = 0;
    switch (ghd::OpenInGitHubDesktop(folder))
    {
    case ghd::LaunchResult::Started: return S_OK;
    case ghd::LaunchResult::NotARepository: messageId = IDS_ERROR_NOT_REPO; break;
    case ghd::LaunchResult::CliNotFound: messageId = IDS_ERROR_CLI; break;
    case ghd::LaunchResult::UnsafePath: messageId = IDS_ERROR_UNSAFE; break;
    case ghd::LaunchResult::LaunchFailed: messageId = IDS_ERROR_LAUNCH; break;
    }

    HWND owner = nullptr;
    if (site_)
    {
        IOleWindow* oleWindow = nullptr;
        if (SUCCEEDED(site_->QueryInterface(IID_PPV_ARGS(&oleWindow))))
        {
            oleWindow->GetWindow(&owner);
            oleWindow->Release();
        }
    }
    MessageBoxW(owner, LoadResourceString(messageId).c_str(), LoadResourceString(IDS_ERROR_CAPTION).c_str(),
        MB_OK | MB_ICONWARNING);
    return S_OK;
}

IFACEMETHODIMP GitHubDesktopCommand::GetFlags(EXPCMDFLAGS* flags)
{
    *flags = ECF_DEFAULT;
    return S_OK;
}

IFACEMETHODIMP GitHubDesktopCommand::EnumSubCommands(IEnumExplorerCommand** enumCommands)
{
    *enumCommands = nullptr;
    return E_NOTIMPL;
}

IFACEMETHODIMP GitHubDesktopCommand::SetSite(IUnknown* site)
{
    if (site_)
    {
        site_->Release();
    }
    site_ = site;
    if (site_)
    {
        site_->AddRef();
    }
    return S_OK;
}

IFACEMETHODIMP GitHubDesktopCommand::GetSite(REFIID riid, void** site)
{
    if (!site_)
    {
        *site = nullptr;
        return E_FAIL;
    }
    return site_->QueryInterface(riid, site);
}
