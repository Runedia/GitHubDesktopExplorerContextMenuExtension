#include "GitHubCli.h"

#include "GitRepository.h"

#include <windows.h>

#include <algorithm>
#include <array>
#include <cwctype>
#include <vector>

namespace ghd
{
namespace
{
bool EndsWithIgnoreCase(std::wstring_view text, std::wstring_view suffix)
{
    if (text.size() < suffix.size())
    {
        return false;
    }
    return std::equal(suffix.begin(), suffix.end(), text.end() - suffix.size(),
        [](wchar_t a, wchar_t b) { return std::towlower(a) == std::towlower(b); });
}

std::wstring SystemCmdPath()
{
    std::wstring buffer(MAX_PATH, L'\0');
    UINT length = GetSystemDirectoryW(buffer.data(), static_cast<UINT>(buffer.size()));
    if (length == 0 || length >= buffer.size())
    {
        return L"cmd.exe";
    }
    buffer.resize(length);
    return buffer + L"\\cmd.exe";
}
}

std::optional<std::wstring> FindGitHubCli()
{
    constexpr std::array<const wchar_t*, 3> kExtensions = {L".exe", L".cmd", L".bat"};
    for (const wchar_t* extension : kExtensions)
    {
        std::vector<wchar_t> buffer(MAX_PATH);
        DWORD length = SearchPathW(nullptr, L"github", extension, static_cast<DWORD>(buffer.size()), buffer.data(), nullptr);
        if (length > buffer.size())
        {
            buffer.resize(length);
            length = SearchPathW(nullptr, L"github", extension, static_cast<DWORD>(buffer.size()), buffer.data(), nullptr);
        }
        if (length > 0 && length < buffer.size())
        {
            return std::wstring(buffer.data(), length);
        }
    }
    return std::nullopt;
}

std::optional<std::wstring> FindGitHubDesktopExe(std::wstring_view cliPath)
{
    // %LOCALAPPDATA%\GitHubDesktop\bin\github.bat -> %LOCALAPPDATA%\GitHubDesktop\GitHubDesktop.exe
    size_t slash = cliPath.find_last_of(L'\\');
    if (slash == std::wstring_view::npos)
    {
        return std::nullopt;
    }
    std::wstring_view binDir = cliPath.substr(0, slash);
    size_t parentSlash = binDir.find_last_of(L'\\');
    if (parentSlash == std::wstring_view::npos)
    {
        return std::nullopt;
    }
    std::wstring candidate(binDir.substr(0, parentSlash));
    candidate += L"\\GitHubDesktop.exe";
    if (GetFileAttributesW(candidate.c_str()) == INVALID_FILE_ATTRIBUTES)
    {
        return std::nullopt;
    }
    return candidate;
}

std::wstring QuoteArgument(std::wstring_view argument)
{
    std::wstring quoted = L"\"";
    size_t backslashes = 0;
    for (wchar_t ch : argument)
    {
        if (ch == L'\\')
        {
            ++backslashes;
            continue;
        }
        if (ch == L'"')
        {
            quoted.append(backslashes * 2 + 1, L'\\');
        }
        else
        {
            quoted.append(backslashes, L'\\');
        }
        backslashes = 0;
        quoted.push_back(ch);
    }
    quoted.append(backslashes * 2, L'\\');
    quoted.push_back(L'"');
    return quoted;
}

bool IsSafeForBatch(std::wstring_view argument)
{
    // % 는 환경 변수 확장, ^ 는 call 재해석, " 는 인용 탈출에 쓰인다.
    return argument.find_first_of(L"%^\"\r\n") == std::wstring_view::npos;
}

std::optional<CommandLine> BuildCommandLine(std::wstring_view cliPath, std::wstring_view folder)
{
    // 루트 경로("D:\")를 제외하고 끝의 역슬래시를 제거한다.
    while (folder.size() > 3 && folder.back() == L'\\')
    {
        folder.remove_suffix(1);
    }

    const bool isBatch = EndsWithIgnoreCase(cliPath, L".cmd") || EndsWithIgnoreCase(cliPath, L".bat");
    if (!isBatch)
    {
        return CommandLine{std::wstring(cliPath), QuoteArgument(cliPath) + L" " + QuoteArgument(folder)};
    }

    if (!IsSafeForBatch(folder) || !IsSafeForBatch(cliPath))
    {
        return std::nullopt;
    }

    // cmd.exe /d /s /c ""<cli>" "<folder>"" : /s 는 바깥 따옴표 한 쌍만 제거한다.
    std::wstring cmd = SystemCmdPath();
    std::wstring arguments = QuoteArgument(cmd) + L" /d /s /c \"\"" + std::wstring(cliPath) + L"\" \"" + std::wstring(folder) + L"\"\"";
    return CommandLine{cmd, arguments};
}

LaunchResult OpenInGitHubDesktop(std::wstring_view folder)
{
    // GetState()와 Invoke() 사이에 폴더가 사라졌을 수 있으므로 다시 확인한다.
    if (!HasGitEntry(folder))
    {
        return LaunchResult::NotARepository;
    }

    std::optional<std::wstring> cli = FindGitHubCli();
    if (!cli)
    {
        return LaunchResult::CliNotFound;
    }

    std::optional<CommandLine> command = BuildCommandLine(*cli, folder);
    if (!command)
    {
        return LaunchResult::UnsafePath;
    }

    // 작업 디렉터리는 상속한다. 대상 저장소를 cwd로 지정하면 장기 실행 프로세스가 폴더를 잠글 수 있다.
    std::wstring mutableArguments = command->arguments;

    STARTUPINFOW startup{};
    startup.cb = sizeof(startup);
    PROCESS_INFORMATION process{};
    if (!CreateProcessW(command->application.c_str(), mutableArguments.data(), nullptr, nullptr, FALSE,
            CREATE_NO_WINDOW | CREATE_UNICODE_ENVIRONMENT, nullptr, nullptr, &startup, &process))
    {
        return LaunchResult::LaunchFailed;
    }
    CloseHandle(process.hThread);
    CloseHandle(process.hProcess);
    return LaunchResult::Started;
}
}
