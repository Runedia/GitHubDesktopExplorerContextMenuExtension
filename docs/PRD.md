# PRD: GitHub Desktop Explorer Context Menu Extension

## 1. 프로젝트 개요

Windows 11 파일 탐색기에서 Git 저장소의 **폴더 내부 빈 공간을 우클릭**했을 때,

> GitHub Desktop으로 열기

명령을 표시하고, 사용자가 선택하면 현재 폴더를 GitHub Desktop에서 열어주는 Windows Explorer Shell Extension을 개발한다.

최종 사용 경험은 다음과 같다.

```text
D:\Projects\MyRepo\
├── .git\
├── src\
├── README.md
└── ...

                    ↓ 빈 공간 우클릭

┌──────────────────────────────┐
│ GitHub Desktop으로 열기       │
│ ...                          │
└──────────────────────────────┘

                    ↓

github "D:\Projects\MyRepo"
```

GitHub Desktop은 Windows에서 `github <path>` 형태로 특정 저장소를 열 수 있는 공식 CLI를 제공한다.

---

# 2. 목표

### 필수 목표

1. Windows 11 File Explorer와 통합한다.
2. **폴더 내부 빈 공간(Directory Background)** 우클릭 메뉴에 명령을 추가한다.
3. 현재 폴더가 Git 저장소인지 검사한다.
4. 현재 폴더에 `.git`이 존재하는 경우에만 메뉴를 표시한다.
5. `.git`이 없는 폴더에서는 메뉴를 표시하지 않는다.
6. 메뉴 실행 시 현재 폴더를 GitHub Desktop으로 연다.
7. GitHub Desktop의 공식 `github <path>` CLI를 사용한다.
8. 기존에 임시로 등록했던 Registry 기반 메뉴를 제거하는 uninstall 절차를 제공한다.
9. Windows 11의 새로운 우클릭 메뉴에 정상적으로 표시되도록 한다.

### 비목표

다음 기능은 구현하지 않는다.

- Git 자체 기능 제공
- commit / push / pull
- GitHub 계정 관리
- GitHub Desktop 자체 기능 대체
- Explorer의 파일/폴더 우클릭 메뉴 추가
- 저장소 내부 하위 폴더에서 부모 저장소 자동 탐색
- 네트워크를 통한 Git 저장소 검색
- GitHub API 사용

---

# 3. 사용자 시나리오

## 3.1 Git 저장소에서 사용

사용자가 다음 폴더를 Explorer에서 연다.

```text
D:\Projects\RUNEMD\
├── .git\
├── src\
├── README.md
└── Cargo.toml
```

빈 공간에서 우클릭한다.

메뉴에 다음 항목이 표시된다.

```text
GitHub Desktop으로 열기
```

선택하면 다음과 동일한 동작을 수행한다.

```cmd
github "D:\Projects\RUNEMD"
```

GitHub Desktop은 해당 저장소를 연다.

---

## 3.2 일반 폴더

다음 폴더에는:

```text
D:\Downloads\
├── file.zip
└── image.png
```

`.git`이 없으므로:

```text
GitHub Desktop으로 열기
```

메뉴 자체가 표시되지 않는다.

---

## 3.3 Git 저장소가 아닌 하위 폴더

다음 구조를 가정한다.

```text
D:\Projects\RUNEMD\
├── .git\
└── src\
    └── renderer\
```

사용자가:

```text
D:\Projects\RUNEMD\src\
```

에서 빈 공간을 우클릭해도 메뉴를 표시하지 않는다.

본 프로젝트의 기본 범위는 **현재 Explorer 폴더 자체가 Git 저장소 루트인 경우**이다.

부모 디렉터리의 `.git`을 검색하는 기능은 구현하지 않는다.

---

# 4. Windows Shell Integration

## 4.1 대상

Windows 11 File Explorer의:

```text
Directory\Background
```

컨텍스트 메뉴를 대상으로 한다.

즉:

```text
폴더 자체 우클릭
```

이 아니라:

```text
폴더 내부의 빈 공간 우클릭
```

이다.

Windows 공식 문서에서도 `IExplorerCommand` 기반 File Explorer Context Menu에서 `Directory\Background`를 폴더 배경 대상으로 사용할 수 있다.

---

# 5. 구현 방식

## 5.1 Shell Extension API

Windows 11의 공식적인 Explorer Context Menu 확장 방식인:

```text
IExplorerCommand
```

를 사용한다.

구현은 native COM DLL을 기본으로 한다.

권장 기술:

```text
C++20
Win32
COM
IExplorerCommand
```

가능하면 Windows SDK가 제공하는 공식 Shell API만 사용한다.

---

# 6. Context Menu 등록

Windows 11의 새로운 Context Menu에 통합한다.

권장 구조:

```text
Native DLL
    ↓
COM Class
    ↓
IExplorerCommand
    ↓
windows.fileExplorerContextMenus
    ↓
Directory\Background
```

Windows 11의 현대적인 Explorer Context Menu 등록에는 패키지 manifest를 통한 `windows.fileExplorerContextMenus` 및 COM class 등록을 사용한다.

패키지 manifest(`package/AppxManifest.xml`)로 Windows 11의 새 Context Menu에 통합한다. 현재 등록 방식은 서명하지 않은 loose-file 패키지(`Add-AppxPackage -Register`)이며 Windows 개발자 모드가 필요하다. 서명된 Sparse Package(MSIX) 배포는 후속 과제이다. (근거: `decisions/v0.1.0.md`)

Legacy:

```text
HKCR\Directory\Background\shell
```

기반의 단순 Registry verb를 주 구현 방식으로 사용하지 않는다.

---

# 7. Git Repository 판별

## 7.1 기본 조건

현재 Explorer 폴더의 실제 파일 시스템 경로를 가져온다.

예:

```text
D:\Projects\RUNEMD
```

다음 경로를 검사한다.

```text
D:\Projects\RUNEMD\.git
```

존재하면 Git 저장소로 판단한다.

---

## 7.2 `.git`의 형태

다음 두 형태를 모두 고려한다.

### 일반 repository

```text
MyRepo\
└── .git\
```

### Git worktree / 기타 형태

```text
MyRepo\
└── .git
```

`.git`이 디렉터리가 아닌 일반 파일인 경우도 Git 저장소로 인정한다.

따라서 단순히:

```text
Directory.Exists(".git")
```

만 사용하지 않는다.

파일 또는 디렉터리 존재 여부를 검사한다.

---

# 8. Context Menu Visibility

`IExplorerCommand::GetState()`를 이용하여 메뉴 표시 여부를 결정한다.

### Git repository

```text
.git 존재
        ↓
ECS_ENABLED
```

### 일반 폴더

```text
.git 없음
        ↓
ECS_HIDDEN
```

따라서 일반 폴더에서는 메뉴 항목 자체가 보이지 않아야 한다.

단순히 메뉴를 표시한 뒤 실행 단계에서 실패시키는 방식은 사용하지 않는다.

---

# 9. Performance Requirements

Explorer의 Context Menu 생성은 UI 경로에서 수행될 수 있으므로 매우 빠르게 동작해야 한다.

따라서 메뉴 표시 여부를 판단하기 위해:

- Git 명령 실행 금지
- 네트워크 접근 금지
- GitHub API 접근 금지
- GitHub Desktop 실행 금지
- 프로세스 생성 금지
- 대규모 디렉터리 검색 금지

단순한:

```text
현재 경로 + "\.git"
```

존재 여부 검사만 수행한다.

Windows의 `IExplorerCommand` 관련 API는 Explorer UI 경로에서 호출될 수 있으므로 무거운 작업을 수행하지 않아야 한다.

---

# 10. Command Invocation

사용자가 메뉴를 선택하면 `IExplorerCommand::Invoke()`에서 현재 폴더 경로를 가져온다.

그 후 GitHub Desktop CLI를 실행한다.

```cmd
github "<repository-path>"
```

예:

```cmd
github "D:\Projects\RUNEMD"
```

GitHub Desktop 공식 문서에서 Windows 환경의 특정 저장소 실행 방식으로 다음 형태를 지원한다.

```cmd
github PATH\TO\REPO
```

---

# 11. GitHub CLI 탐색

GitHub Desktop CLI의 위치를 하드코딩하지 않는다.

Windows 환경 변수 `PATH`에서 다음 순서로 찾는다.

```text
github.exe
github.cmd
github.bat
```

공식 GitHub Desktop 설치본(3.6.x)은 `github.exe`가 아니라 `%LOCALAPPDATA%\GitHubDesktop\bin\github.bat`를 제공한다. (근거: `decisions/v0.1.0.md`)

실행 방식:

```text
github.exe        → CreateProcess로 직접 실행
github.cmd / .bat → %SystemRoot%\System32\cmd.exe /d /s /c ""<cli>" "<folder>""
        ↓
GitHub Desktop
```

배치 파일 경로는 cmd.exe가 재해석할 수 있으므로, 대상 경로에 `%`, `^`, `"`, 줄바꿈이 있으면 실행하지 않고 오류를 표시한다. `.exe` 경로에는 이 제한이 없다.

GitHub Desktop의 설치 위치:

```text
C:\Users\<user>\AppData\Local\GitHubDesktop\
```

를 직접 가정하지 않는다.

---

# 12. 오류 처리

## 12.1 GitHub CLI를 찾을 수 없음

GitHub Desktop CLI를 찾지 못하거나 실행에 실패하면 사용자에게 명확한 오류를 표시한다.

예:

```text
GitHub Desktop 명령줄 도구를 찾을 수 없습니다.

GitHub Desktop에서 명령줄 도구가 설치되어 있는지 확인하세요.
```

단, Explorer Context Menu의 상태 조회 단계에서는 오류 UI를 표시하지 않는다.

---

## 12.2 저장소가 삭제된 경우

`GetState()`와 `Invoke()` 사이에 폴더가 삭제되는 상황을 고려한다.

Invoke 시 다시 `.git` 존재 여부를 확인한다.

존재하지 않으면 실행하지 않는다.

---

## 12.3 경로에 공백이 포함된 경우

다음과 같은 경로를 정상적으로 지원해야 한다.

```text
D:\Projects\My Projects\RUNEMD
```

실행 명령은 반드시 올바르게 quoting한다.

```text
github "D:\Projects\My Projects\RUNEMD"
```

---

# 13. Icon

Context Menu에는 GitHub Desktop 아이콘을 사용한다.

우선순위:

1. GitHub Desktop 공식 아이콘 resource 사용 (탐색한 CLI 경로의 상위 폴더에 있는 `GitHubDesktop.exe`의 첫 번째 아이콘)
2. 프로젝트에서 제공하는 별도 `.ico`
3. 아이콘을 찾을 수 없을 경우 기본 Shell icon 사용

아이콘을 별도로 복제하는 경우 GitHub Desktop의 상표 및 라이선스 조건을 확인한다.

---

# 14. UI Text

기본 표시 문자열:

```text
GitHub Desktop으로 열기
```

내부 canonical command name:

```text
OpenWithGitHubDesktop
```

Tooltip:

```text
GitHub Desktop에서 이 저장소 열기
```

향후 다국어 지원을 고려하여 UI 문자열은 코드에 하드코딩하지 않는다.

---

# 15. Installer

설치 프로그램은 다음 작업을 수행해야 한다.

1. Shell Extension 설치
2. COM Class 등록
3. Explorer Context Menu 등록
4. 패키지 등록 (`Add-AppxPackage -Register`, 개발자 모드 필요)
5. GitHub Desktop CLI 존재 여부 확인
6. 설치 완료 후 Explorer 재시작 또는 Shell refresh 안내

설치 과정에서 GitHub Desktop 자체를 설치하지 않는다.

---

# 16. Uninstaller

제거 프로그램은 다음 항목을 모두 제거해야 한다.

```text
Shell Extension
COM registration
MSIX / Sparse Package registration
Context Menu registration
설치 파일
```

사용자가 직접 만든 Git 저장소나 GitHub Desktop 설치에는 영향을 주지 않는다.

---

# 17. 기존 Registry Context Menu 제거

개발 과정에서 이전에 등록했던 다음 Registry 기반 메뉴를 제거해야 한다.

## 기존 메뉴 위치

```text
HKCU\Software\Classes\Directory\Background\shell\GitHubDesktop
```

이 키가 존재할 경우 제거한다.

제거 예시:

```reg
Windows Registry Editor Version 5.00

[-HKEY_CURRENT_USER\Software\Classes\Directory\Background\shell\GitHubDesktop]
```

PowerShell로 제거할 경우:

```powershell
Remove-Item `
  "HKCU:\Software\Classes\Directory\Background\shell\GitHubDesktop" `
  -Recurse `
  -Force `
  -ErrorAction SilentlyContinue
```

---

# 18. 기존 PowerShell Script 제거

이전 테스트에서 다음 파일을 만들었다면 제거한다.

```text
%USERPROFILE%\Open-GitHubDesktop.ps1
```

제거:

```powershell
Remove-Item `
  "$env:USERPROFILE\Open-GitHubDesktop.ps1" `
  -Force `
  -ErrorAction SilentlyContinue
```

단, 설치 프로그램이 해당 파일을 생성하지 않았다면 임의의 다른 사용자 파일은 삭제하지 않는다.

---

# 19. 기존 Registry 방식과의 충돌 방지

설치 과정에서 다음 Registry 경로가 남아 있는지 확인한다.

```text
HKCU\Software\Classes\Directory\Background\shell\GitHubDesktop
```

존재하면 설치 전에 제거한다.

새 Shell Extension과 기존 Registry verb를 동시에 등록하지 않는다.

목표는 Explorer에서:

```text
GitHub Desktop으로 열기
```

항목이 정확히 하나만 존재하는 것이다.

---

# 20. Explorer Reload

Shell Extension을 설치 또는 제거한 후 Explorer가 기존 COM registration을 계속 가지고 있을 수 있다.

따라서 installer/uninstaller는 필요한 경우 Explorer 재시작을 수행하거나 사용자에게 다음을 안내한다.

```powershell
Stop-Process -Name explorer -Force
Start-Process explorer.exe
```

강제 Explorer 재시작은 사용자의 열려 있는 Explorer 상태에 영향을 줄 수 있으므로 기본적으로는:

```text
설치 완료
→ Explorer 재시작 필요 여부 안내
```

방식을 우선한다.

---

# 21. 보안 요구사항

Shell Extension은 Explorer 프로세스의 일부로 동작할 수 있으므로 최소 권한 원칙을 적용한다.

금지:

- 관리자 권한 요구
- 네트워크 연결
- GitHub API 호출
- 사용자 데이터 수집
- telemetry
- 자동 업데이트 서버 통신
- shell command 문자열을 통한 임의 명령 실행 (단, GitHub Desktop CLI가 `.bat`인 경우에 한해 고정된 형식의 `cmd.exe /d /s /c` 호출만 허용하며, 위험 문자가 포함된 경로는 거부한다. 11장 참조)

실행 대상은 PATH에서 탐색한 GitHub Desktop CLI:

```text
github.exe | github.cmd | github.bat
```

이며, 사용자가 선택한 현재 폴더 경로만 인자로 전달한다.

---

# 22. 테스트 요구사항

## 22.1 Git repository

```text
D:\TestRepo\
└── .git\
```

기대 결과:

```text
메뉴 표시
```

---

## 22.2 일반 directory

```text
D:\TestFolder\
```

기대 결과:

```text
메뉴 숨김
```

---

## 22.3 `.git` 파일

```text
D:\TestRepo\.git
```

이 `.git`이 directory가 아닌 파일이어도:

```text
메뉴 표시
```

---

## 22.4 경로에 공백

```text
D:\My Projects\Test Repo\
```

기대 결과:

```text
GitHub Desktop으로 정상 실행
```

---

## 22.5 한글 경로

```text
D:\프로젝트\테스트 저장소\
```

기대 결과:

```text
GitHub Desktop으로 정상 실행
```

---

## 22.6 긴 경로

Windows에서 지원되는 범위 내에서 긴 repository path를 테스트한다.

---

## 22.7 GitHub Desktop 미설치

Shell Extension 자체는 설치할 수 있으나 GitHub Desktop CLI(`github.exe`/`.cmd`/`.bat`)가 없는 경우 실행 시 명확한 오류를 제공한다.

가능하면 installer 단계에서 감지하고 경고한다.

---

## 22.8 GitHub Desktop CLI 미설치

GitHub Desktop이 존재하지만 CLI가 PATH에 없는 경우를 테스트한다.

---

## 22.9 Explorer 새 Context Menu

Windows 11의 일반 우클릭 메뉴에서 바로 표시되는지 확인한다.

다음 위치에만 표시되는 것은 실패로 간주한다.

```text
기타 옵션 표시
```

가능하면 Windows 11의 기본 Context Menu에 직접 표시한다.

---

# 23. Compatibility

최소 대상:

```text
Windows 11 x64
```

우선 지원:

```text
Windows 11 24H2+
```

Windows 10 지원은 요구사항에 포함하지 않는다.

---

# 24. Project Structure

권장 구조:

```text
GitHubDesktopExplorerContextMenuExtension/
├── src/
│   ├── ShellExtension/     COM DLL (IExplorerCommand, ClassFactory, DllMain, .rc, .def)
│   ├── Common/             GitRepository, GitHubCli (DLL과 테스트가 공유)
│   └── Stub/               매니페스트 Application용 자리표시자 실행 파일
├── package/
│   ├── AppxManifest.xml
│   └── Assets/
├── installer/              Install.ps1, Uninstall.ps1, Cleanup-Legacy.ps1
├── tests/                  CommonTests.cpp (CTest)
├── docs/
│   ├── PRD.md
│   └── decisions/          버전별 결정 기록
├── CMakeLists.txt
├── build.ps1               빌드 + 테스트 + dist\package 구성
└── README.md
```

---

# 25. Acceptance Criteria

프로젝트 완료 조건:

### AC-01

Git 저장소 루트의 빈 공간을 우클릭했을 때:

```text
GitHub Desktop으로 열기
```

가 표시된다.

### AC-02

일반 폴더의 빈 공간을 우클릭했을 때 해당 메뉴가 표시되지 않는다.

### AC-03

`.git`이 directory인 repository를 인식한다.

### AC-04

`.git`이 file인 repository도 인식한다.

### AC-05

현재 폴더의 `.git`만 검사한다.

### AC-06

GitHub Desktop CLI를 통해 현재 repository를 연다.

```cmd
github "<current-directory>"
```

### AC-07

공백이 포함된 경로를 정상적으로 처리한다.

### AC-08

한글 경로를 정상적으로 처리한다.

### AC-09

Windows 11 기본 Context Menu에 표시된다.

### AC-10

기존 Registry 기반:

```text
HKCU\Software\Classes\Directory\Background\shell\GitHubDesktop
```

항목이 설치 후 존재하지 않는다.

### AC-11

기존:

```text
%USERPROFILE%\Open-GitHubDesktop.ps1
```

파일을 프로젝트가 생성하지 않으며, uninstall/cleanup 과정에서 기존 테스트 파일을 제거할 수 있다.

### AC-12

Shell Extension이 Explorer의 UI를 눈에 띄게 지연시키지 않는다.

### AC-13

Shell Extension은 네트워크 접근을 하지 않는다.

### AC-14

GitHub Desktop이 설치되어 있지 않거나 CLI를 찾을 수 없는 경우 충돌하지 않고 명확한 오류를 제공한다.

---

# 26. 구현 우선순위

### Phase 1 — Core

- `IExplorerCommand`
- COM registration
- `Directory\Background`
- `.git` detection
- `ECS_HIDDEN`
- GitHub Desktop CLI invocation

### Phase 2 — Windows 11 Integration

- 패키지 manifest (loose-file 등록; 서명된 Sparse Package는 후속)
- Windows 11 기본 Context Menu
- 아이콘
- Explorer reload

### Phase 3 — Installer

- 설치
- 제거
- 기존 Registry cleanup
- 기존 PowerShell script cleanup

### Phase 4 — Testing

- 경로 테스트
- Unicode 테스트
- `.git` directory/file 테스트
- GitHub Desktop 미설치 테스트
- Explorer 재시작 테스트

---

# 27. 중요한 구현 원칙

1. **Registry hack을 핵심 구현으로 사용하지 않는다.**
2. Windows 11의 공식 `IExplorerCommand` 방식을 우선한다.
3. `Directory\Background`만 대상으로 한다.
4. `.git` 존재 여부는 빠른 로컬 filesystem 검사만으로 판단한다.
5. 부모 디렉터리를 탐색하지 않는다.
6. `Invoke()`에서만 GitHub Desktop을 실행한다.
7. GitHub Desktop의 공식 `github <path>` CLI를 사용한다.
8. Explorer UI thread에서 무거운 작업을 수행하지 않는다.
9. 기존 Registry 기반 테스트 메뉴를 설치 시 제거한다.
10. Uninstall 시 프로젝트가 생성한 모든 Shell Extension registration을 제거한다.
11. 사용자의 Git repository와 GitHub Desktop 설치 자체는 삭제하거나 수정하지 않는다.
12. 관리자 권한을 요구하지 않는 방향을 우선한다.

---

# 28. 완료 상태

최종 사용자는 다음 동작만 하면 된다.

```text
Git 저장소를 Explorer에서 연다.
        ↓
빈 공간 우클릭
        ↓
GitHub Desktop으로 열기
        ↓
클릭
        ↓
GitHub Desktop에서 현재 저장소 열림
```

일반 폴더에서는 해당 메뉴가 존재하지 않는다.

---

# 29. 구현 상태 (v0.1.0, 2026-10-29)

검증 근거는 `build.ps1` 실행 결과(빌드 성공, CTest `CommonTests` 통과), 설치 후 명령 출력, 그리고 사용자 보고이다.

설치 후 확인된 사실: `Get-AppxPackage Runedia.GitHubDesktopExplorer`가 Status `Ok`, InstallLocation `%USERPROFILE%\.GitHubDesktopExplorer\package`를 반환했고, 레거시 키와 `Open-GitHubDesktop.ps1`은 존재하지 않는다. 사용자는 `Install.ps1`로 설치한 뒤 Explorer 메뉴 표시와 실행이 정상 동작하며 특이사항이 없다고 보고했다.

| 항목 | 상태 |
|---|---|
| AC-03, AC-04, AC-05 (`.git` 디렉터리/파일, 현재 폴더만 검사) | 단위 테스트로 검증 |
| AC-07, AC-08 (공백, 한글 경로) | 경로 판별과 명령줄 구성을 단위 테스트로 검증. 실제 GitHub Desktop 실행은 미검증 |
| 긴 경로 (22.6) | 판별만 단위 테스트로 검증 |
| AC-01, AC-02, AC-09 (Explorer 메뉴 표시/숨김) | 사용자 보고로 정상 동작. 화면 증거나 자동 테스트는 없음 |
| AC-06 (CLI 실행) | 사용자 보고로 정상 동작. 경로 유형(공백/한글/긴 경로)별 실제 실행은 개별 확인하지 않음 |
| AC-10, AC-11 (레거시 정리) | 설치 후 레거시 키와 스크립트가 없음을 확인 |
| AC-12 (UI 지연) | 미측정. 상태 조회는 `GetFileAttributesW` 한 번과 사이트 체인 조회만 수행 |
| AC-13, AC-14 | 코드상 네트워크 호출 없음. CLI 미발견 시 오류 메시지 구현, 동작 미검증 |
| 서명된 Sparse Package 배포 | 미구현 (후속 과제) |
| README.md | 작성 완료 |
