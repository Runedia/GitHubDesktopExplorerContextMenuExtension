#include "GitHubCli.h"
#include "GitRepository.h"

#include <windows.h>

#include <cstdio>
#include <filesystem>
#include <fstream>
#include <string>

namespace fs = std::filesystem;

namespace
{
int g_failures = 0;
int g_checks = 0;

void Check(bool condition, const char* expression, int line)
{
    ++g_checks;
    if (!condition)
    {
        ++g_failures;
        std::fprintf(stderr, "FAIL line %d: %s\n", line, expression);
    }
}

#define CHECK(expr) Check((expr), #expr, __LINE__)

fs::path MakeTempRoot()
{
    fs::path root = fs::temp_directory_path() / L"ghd-tests" / std::to_wstring(GetCurrentProcessId());
    fs::remove_all(root);
    fs::create_directories(root);
    return root;
}

void TestGitDetection(const fs::path& root)
{
    // .git 디렉터리
    fs::path dirRepo = root / L"dir-repo";
    fs::create_directories(dirRepo / L".git");
    CHECK(ghd::HasGitEntry(dirRepo.wstring()));
    CHECK(ghd::HasGitEntry(dirRepo.wstring() + L"\\"));

    // .git 파일 (worktree 형태)
    fs::path fileRepo = root / L"file-repo";
    fs::create_directories(fileRepo);
    std::ofstream(fileRepo / L".git") << "gitdir: ../elsewhere\n";
    CHECK(ghd::HasGitEntry(fileRepo.wstring()));

    // 일반 폴더
    fs::path plain = root / L"plain";
    fs::create_directories(plain);
    CHECK(!ghd::HasGitEntry(plain.wstring()));

    // 부모 저장소는 탐색하지 않는다.
    fs::path sub = dirRepo / L"src" / L"renderer";
    fs::create_directories(sub);
    CHECK(!ghd::HasGitEntry(sub.wstring()));

    // 공백 + 한글 경로
    fs::path korean = root / L"프로젝트" / L"테스트 저장소";
    fs::create_directories(korean / L".git");
    CHECK(ghd::HasGitEntry(korean.wstring()));

    // 존재하지 않는 경로와 빈 문자열
    CHECK(!ghd::HasGitEntry((root / L"missing").wstring()));
    CHECK(!ghd::HasGitEntry(L""));
}

void TestLongPath(const fs::path& root)
{
    std::wstring longPath = L"\\\\?\\" + root.wstring();
    while (longPath.size() < 400)
    {
        longPath += L"\\segment-with-a-fairly-long-name";
    }
    fs::create_directories(fs::path(longPath) / L".git");
    // 확장 접두사가 이미 있는 경우와 없는 경우 모두 동작해야 한다.
    CHECK(ghd::HasGitEntry(longPath));
    CHECK(ghd::HasGitEntry(longPath.substr(4)));
}

void TestQuoting()
{
    CHECK(ghd::QuoteArgument(L"D:\\Projects\\My Projects\\RUNEMD") == L"\"D:\\Projects\\My Projects\\RUNEMD\"");
    CHECK(ghd::QuoteArgument(L"D:\\") == L"\"D:\\\\\"");
    CHECK(ghd::QuoteArgument(L"a\"b") == L"\"a\\\"b\"");
    CHECK(ghd::QuoteArgument(L"D:\\프로젝트\\테스트 저장소") == L"\"D:\\프로젝트\\테스트 저장소\"");
}

void TestBatchSafety()
{
    CHECK(ghd::IsSafeForBatch(L"D:\\My Projects\\Repo"));
    CHECK(ghd::IsSafeForBatch(L"D:\\a&b\\(c)"));
    CHECK(!ghd::IsSafeForBatch(L"D:\\100%\\Repo"));
    CHECK(!ghd::IsSafeForBatch(L"D:\\a^b"));
    CHECK(!ghd::IsSafeForBatch(L"D:\\a\"b"));
}

void TestCommandLine()
{
    auto exe = ghd::BuildCommandLine(L"C:\\gh\\github.exe", L"D:\\My Projects\\Repo\\");
    CHECK(exe.has_value());
    if (exe)
    {
        CHECK(exe->application == L"C:\\gh\\github.exe");
        CHECK(exe->arguments == L"\"C:\\gh\\github.exe\" \"D:\\My Projects\\Repo\"");
    }

    auto bat = ghd::BuildCommandLine(L"C:\\gh\\bin\\github.bat", L"D:\\프로젝트\\테스트 저장소");
    CHECK(bat.has_value());
    if (bat)
    {
        CHECK(bat->application.size() > 8);
        CHECK(bat->arguments.find(L"/d /s /c \"\"C:\\gh\\bin\\github.bat\" \"D:\\프로젝트\\테스트 저장소\"\"") != std::wstring::npos);
    }

    CHECK(!ghd::BuildCommandLine(L"C:\\gh\\bin\\github.bat", L"D:\\100%\\Repo").has_value());
    CHECK(ghd::BuildCommandLine(L"C:\\gh\\github.exe", L"D:\\100%\\Repo").has_value());

    CHECK(ghd::FindGitHubDesktopExe(L"C:\\nonexistent\\bin\\github.bat") == std::nullopt);
}

void TestLaunchGuards(const fs::path& root)
{
    // .git 이 없으면 CLI 탐색 이전에 실행을 거부한다.
    fs::path plain = root / L"launch-plain";
    fs::create_directories(plain);
    CHECK(ghd::OpenInGitHubDesktop(plain.wstring()) == ghd::LaunchResult::NotARepository);
}
}

int wmain()
{
    fs::path root = MakeTempRoot();
    TestGitDetection(root);
    TestLongPath(root);
    TestQuoting();
    TestBatchSafety();
    TestCommandLine();
    TestLaunchGuards(root);

    std::error_code ignored;
    fs::remove_all(root, ignored);

    std::printf("%d checks, %d failures\n", g_checks, g_failures);
    return g_failures == 0 ? 0 : 1;
}
