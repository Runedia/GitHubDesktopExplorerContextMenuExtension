#Requires -Version 7
<#
.SYNOPSIS
    패키지 등록(COM, Context Menu 포함)을 제거하고 레거시 항목을 정리한다.
    사용자의 Git 저장소와 GitHub Desktop 설치는 건드리지 않는다.
#>
[CmdletBinding()]
param()

$ErrorActionPreference = 'Stop'
. (Join-Path $PSScriptRoot 'Cleanup-Legacy.ps1')

$existing = Get-AppxPackage -Name 'Runedia.GitHubDesktopExplorer'
if ($existing) {
    Remove-AppxPackage -Package $existing.PackageFullName
    Write-Output '패키지 등록을 제거했습니다.'
} else {
    Write-Output '등록된 패키지가 없습니다.'
}

Remove-LegacyContextMenu
Write-Output '제거 완료. 메뉴가 남아 있으면 Explorer 를 재시작하세요.'
