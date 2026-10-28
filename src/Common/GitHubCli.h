#pragma once

#include <optional>
#include <string>
#include <string_view>

namespace ghd
{
enum class LaunchResult
{
    Started,
    NotARepository,
    CliNotFound,
    UnsafePath,
    LaunchFailed,
};

// PATH에서 GitHub Desktop CLI(github.exe, github.cmd, github.bat 순)를 찾는다.
std::optional<std::wstring> FindGitHubCli();

// CLI 경로로부터 GitHubDesktop.exe(아이콘 리소스 소스)의 경로를 추정한다.
std::optional<std::wstring> FindGitHubDesktopExe(std::wstring_view cliPath);

// CommandLineToArgvW 규칙에 맞게 인자 하나를 인용한다.
std::wstring QuoteArgument(std::wstring_view argument);

// cmd.exe 해석을 거치는 배치 파일에 안전하게 전달할 수 없는 문자가 포함되어 있는지 검사한다.
bool IsSafeForBatch(std::wstring_view argument);

// 실행할 명령줄을 구성한다. .exe는 직접 실행하고 .cmd/.bat는 cmd.exe를 통해 실행한다.
// 반환값이 비어 있으면 구성 불가(안전하지 않은 인자)를 뜻한다.
struct CommandLine
{
    std::wstring application;
    std::wstring arguments;
};
std::optional<CommandLine> BuildCommandLine(std::wstring_view cliPath, std::wstring_view folder);

// .git을 다시 확인한 뒤 `github "<folder>"`와 동일하게 실행한다.
LaunchResult OpenInGitHubDesktop(std::wstring_view folder);
}
