#pragma once

#include <string>
#include <string_view>

namespace ghd
{
// 폴더 직하위에 ".git" 항목(디렉터리 또는 파일)이 있는지 검사한다.
// 부모 디렉터리는 탐색하지 않으며, 프로세스 생성이나 네트워크 접근을 하지 않는다.
bool HasGitEntry(std::wstring_view folder);
}
