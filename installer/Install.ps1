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
Add-AppxPackage -Register (Resolve-Path -LiteralPath $manifest).Path

$cli = Get-Command -Name github -CommandType Application -ErrorAction SilentlyContinue
if ($cli) {
    Write-Output "GitHub Desktop CLI: $($cli.Source)"
} else {
    Write-Warning 'PATH 에서 github 명령줄 도구를 찾지 못했습니다. GitHub Desktop 에서 명령줄 도구 설치를 확인하세요.'
}

Write-Output '설치 완료. 메뉴가 보이지 않으면 Explorer 를 재시작하세요 (열려 있는 창이 닫힙니다):'
Write-Output '  Stop-Process -Name explorer -Force; Start-Process explorer.exe'
