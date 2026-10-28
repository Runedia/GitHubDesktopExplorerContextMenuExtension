#Requires -Version 7
<#
.SYNOPSIS
    dist\package 를 현재 사용자에게 등록한다 (관리자 권한 불필요, Windows 개발자 모드 필요).
.DESCRIPTION
    1. 기존 Registry 기반 메뉴(HKCU\...\Directory\Background\shell\GitHubDesktop)를 제거한다.
    2. 기존 테스트 스크립트 %USERPROFILE%\Open-GitHubDesktop.ps1 을 제거한다.
    3. 패키지를 등록한다 (Add-AppxPackage -Register).
    4. GitHub Desktop CLI 존재 여부를 확인하고, Explorer 재시작 필요 여부를 안내한다.
#>
[CmdletBinding()]
param(
    [string]$PackageDir = (Join-Path $PSScriptRoot '..\dist\package')
)

$ErrorActionPreference = 'Stop'
. (Join-Path $PSScriptRoot 'Cleanup-Legacy.ps1')

$manifest = Join-Path $PackageDir 'AppxManifest.xml'
if (-not (Test-Path -LiteralPath $manifest)) { throw "AppxManifest.xml 이 없습니다. 먼저 build.ps1 을 실행하세요: $manifest" }

$devMode = Get-ItemProperty -LiteralPath 'HKLM:\SOFTWARE\Microsoft\Windows\CurrentVersion\AppModelUnlock' -ErrorAction SilentlyContinue
if (-not $devMode -or $devMode.AllowDevelopmentWithoutDevLicense -ne 1) {
    throw 'Windows 개발자 모드가 꺼져 있어 서명되지 않은 패키지를 등록할 수 없습니다.'
}

Remove-LegacyContextMenu

$existing = Get-AppxPackage -Name 'Runedia.GitHubDesktopExplorer'
if ($existing) { Remove-AppxPackage -Package $existing.PackageFullName }

# 빌드 출력(dist\package)이 재빌드로 삭제되어도 등록이 유지되도록 고정 위치로 복사해 등록한다.
# %LOCALAPPDATA% 아래에서는 Add-AppxPackage -Register 가 실패하므로 사용자 프로필 아래를 쓴다.
$installDir = Join-Path $env:USERPROFILE '.GitHubDesktopExplorer\package'
if (Test-Path -LiteralPath $installDir) { Remove-Item -LiteralPath $installDir -Recurse -Force }
New-Item -ItemType Directory -Path $installDir -Force | Out-Null
# 등록 시 Windows 가 만드는 숨김 폴더(microsoft.system.package.metadata)는 복사하지 않는다.
Get-ChildItem -LiteralPath $PackageDir -Force |
    Where-Object Name -ne 'microsoft.system.package.metadata' |
    Copy-Item -Destination $installDir -Recurse -Force
Write-Output "설치 위치: $installDir"

Add-AppxPackage -Register (Join-Path $installDir 'AppxManifest.xml')

$cli = Get-Command -Name github -CommandType Application -ErrorAction SilentlyContinue
if ($cli) {
    Write-Output "GitHub Desktop CLI: $($cli.Source)"
} else {
    Write-Warning 'PATH 에서 github 명령줄 도구를 찾지 못했습니다. GitHub Desktop 에서 명령줄 도구 설치를 확인하세요.'
}

Write-Output '설치 완료. 메뉴가 보이지 않으면 Explorer 를 재시작하세요 (열려 있는 창이 닫힙니다):'
Write-Output '  Stop-Process -Name explorer -Force; Start-Process explorer.exe'
