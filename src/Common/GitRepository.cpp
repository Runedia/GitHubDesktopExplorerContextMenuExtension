#include "GitRepository.h"

#include <windows.h>

namespace ghd
{
namespace
{
// MAX_PATH를 넘는 경로를 위해 확장 길이 접두사를 붙인다.
std::wstring ToExtendedPath(std::wstring_view path)
{
    constexpr std::wstring_view kExtended = LR"(\\?\)";
    if (path.starts_with(kExtended))
    {
        return std::wstring(path);
    }
    if (path.starts_with(LR"(\\)"))
    {
        return std::wstring(LR"(\\?\UNC\)") + std::wstring(path.substr(2));
    }
    return std::wstring(kExtended) + std::wstring(path);
}
}

bool HasGitEntry(std::wstring_view folder)
{
    if (folder.empty())
    {
        return false;
    }
    while (folder.size() > 3 && folder.back() == L'\\')
    {
        folder.remove_suffix(1);
    }

    std::wstring candidate(folder);
    if (candidate.back() != L'\\')
    {
        candidate.push_back(L'\\');
    }
    candidate += L".git";

    // 디렉터리/파일 모두 인정한다. 존재 여부만 확인하므로 속성 조회로 충분하다.
    return GetFileAttributesW(ToExtendedPath(candidate).c_str()) != INVALID_FILE_ATTRIBUTES;
}
}
