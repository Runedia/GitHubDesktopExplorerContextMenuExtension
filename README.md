# GitHub Desktop Explorer Context Menu Extension

Windows 11 파일 탐색기에서 **Git 저장소 폴더의 빈 공간을 우클릭**하면 `GitHub Desktop으로 열기` 항목을 표시하고, 선택하면 해당 폴더를 GitHub Desktop에서 여는 Shell Extension이다.

- Windows 11의 기본(새) 우클릭 메뉴에 직접 표시된다. "기타 옵션 표시" 안에 숨지 않는다.
- 현재 폴더 바로 아래에 `.git`(디렉터리 또는 파일)이 있을 때만 표시된다. 부모 폴더는 탐색하지 않는다.
- 상태 조회 중에는 프로세스 생성, 네트워크 접근, Git 명령 실행을 하지 않는다. 파일 속성 조회 한 번만 수행한다.
- 관리자 권한이 필요 없다.

## 요구 사항

| 용도 | 요구 사항 |
|---|---|
| 실행 | Windows 11 x64, GitHub Desktop(명령줄 도구가 `PATH`에 있어야 함) |
| 설치 | Windows 개발자 모드 켜짐, PowerShell 7 |
| 빌드 | Visual Studio Build Tools(MSVC, C++20), Windows SDK, CMake, Ninja, PowerShell 7 |

## 빌드

```powershell
.\build.ps1
```

`build.ps1`은 MSVC 환경을 불러와 CMake/Ninja로 빌드하고, CTest(`CommonTests`)를 실행한 뒤 `dist\package`를 구성한다. 옵션은 다음과 같다.

| 옵션 | 설명 |
|---|---|
| `-Configuration Debug` | Debug 빌드 (기본값 `Release`) |
| `-SkipTests` | 테스트 생략 |

## 설치

```powershell
.\installer\Install.ps1
```

수행 내용은 다음과 같다.

1. 기존 Registry 기반 메뉴 `HKCU\Software\Classes\Directory\Background\shell\GitHubDesktop`를 제거한다.
2. 기존 테스트 스크립트 `%USERPROFILE%\Open-GitHubDesktop.ps1`이 있으면 제거한다.
3. `dist\package`를 `%USERPROFILE%\.GitHubDesktopExplorer\package`로 복사하고, 그 위치를 `Add-AppxPackage -Register`로 등록한다.
4. GitHub Desktop CLI가 `PATH`에 있는지 확인하고 없으면 경고한다.

메뉴가 보이지 않으면 Explorer를 재시작한다. 열려 있는 탐색기 창이 닫히므로 자동으로 하지 않는다.

```powershell
Stop-Process -Name explorer -Force; Start-Process explorer.exe
```

재빌드 후 다시 반영하려면 `Install.ps1`을 다시 실행한다. 기존 등록을 제거하고 새로 등록한다.

## 제거

```powershell
.\installer\Uninstall.ps1
```

패키지 등록(COM, Context Menu 포함), 설치 폴더, 레거시 항목을 제거한다. 사용자의 Git 저장소와 GitHub Desktop 설치에는 영향을 주지 않는다. DLL이 Explorer에 로드되어 있어 설치 폴더를 지우지 못하면 경고를 출력하므로, Explorer 재시작 후 직접 삭제한다.

## 동작 방식

```text
폴더 빈 공간 우클릭
  → IExplorerCommand::GetState()   .git 존재 여부만 검사 (ECS_ENABLED / ECS_HIDDEN)
  → 메뉴 선택
  → IExplorerCommand::Invoke()     .git을 다시 확인한 뒤 GitHub Desktop CLI 실행
```

### GitHub Desktop CLI 탐색

`PATH`에서 `github.exe`, `github.cmd`, `github.bat` 순으로 찾는다. 공식 설치본은 `github.exe`가 아니라 `%LOCALAPPDATA%\GitHubDesktop\bin\github.bat`를 제공한다.

- `.exe`는 `CreateProcess`로 직접 실행한다.
- `.cmd`/`.bat`는 `cmd.exe /d /s /c`를 거쳐 실행한다.

## 알려진 제한

- CLI가 `.bat`/`.cmd`이면 경로에 `%`, `^`, `"`, 줄바꿈이 포함된 폴더는 열 수 없다. `cmd.exe`의 재해석을 막을 수 없어 의도적으로 거부하며, 오류 메시지를 표시한다.
- 서명하지 않은 loose-file 패키지로 등록한다. 서명된 Sparse Package(MSIX) 배포는 구현하지 않았다.
- 이 프로젝트의 `Install.ps1`은 `%LOCALAPPDATA%` 아래에 복사한 패키지 등록이 실패하여(원인 미확인) 사용자 프로필 아래 `.GitHubDesktopExplorer`를 쓴다.
- 부모 폴더의 `.git`은 찾지 않는다. 저장소 하위 폴더에서는 메뉴가 표시되지 않는다.
- 현재 폴더 우클릭(폴더 항목)과 파일 우클릭 메뉴는 대상이 아니다.

## 프로젝트 구조

```text
src/
  Common/           GitRepository, GitHubCli (DLL과 테스트가 공유)
  ShellExtension/   COM DLL (IExplorerCommand, ClassFactory, DllMain, .rc, .def)
  Stub/             매니페스트 Application용 자리표시자 실행 파일
package/            AppxManifest.xml, Assets
installer/          Install.ps1, Uninstall.ps1, Cleanup-Legacy.ps1
tests/              CommonTests.cpp (CTest)
docs/               PRD.md, decisions/
build.ps1           빌드, 테스트, dist\package 구성
```

## 문서

- [docs/PRD.md](docs/PRD.md): 현재 유효한 요구 사항과 구현 상태
- [docs/decisions/](docs/decisions/README.md): 요구 사항이 바뀐 이유와 설계 결정 기록
