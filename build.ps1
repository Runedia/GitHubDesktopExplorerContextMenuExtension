#Requires -Version 7
<#
.SYNOPSIS
    MSVC 환경을 불러와 CMake/Ninja로 빌드하고 테스트를 실행한 뒤 dist\package 를 구성한다.
#>
[CmdletBinding()]
param(
    [ValidateSet('Release', 'Debug')]
    [string]$Configuration = 'Release',
    [switch]$SkipTests
)

$ErrorActionPreference = 'Stop'
$root = $PSScriptRoot
$buildDir = Join-Path $root "build\$Configuration"
$packageDir = Join-Path $root 'dist\package'

$vswhere = Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio\Installer\vswhere.exe'
$vsPath = & $vswhere -latest -products * -property installationPath
if ($LASTEXITCODE -ne 0 -or -not $vsPath) { throw 'Visual Studio Build Tools를 찾을 수 없습니다.' }

# vcvars64.bat 가 설정한 환경 변수를 현재 프로세스로 가져온다.
$vcvars = Join-Path $vsPath 'VC\Auxiliary\Build\vcvars64.bat'
$envDump = & cmd.exe /d /c "`"$vcvars`" >NUL && set"
if ($LASTEXITCODE -ne 0) { throw 'vcvars64.bat 실행에 실패했습니다.' }
foreach ($line in $envDump) {
    if ($line -match '^([^=]+)=(.*)$') { Set-Item -LiteralPath "Env:$($Matches[1])" -Value $Matches[2] }
}

& cmake.exe -S $root -B $buildDir -G Ninja "-DCMAKE_BUILD_TYPE=$Configuration"
if ($LASTEXITCODE -ne 0) { throw 'CMake configure 실패' }
& cmake.exe --build $buildDir
if ($LASTEXITCODE -ne 0) { throw 'CMake build 실패' }

if (-not $SkipTests) {
    & ctest.exe --test-dir $buildDir --output-on-failure
    if ($LASTEXITCODE -ne 0) { throw '테스트 실패' }
}

# 배포용 패키지 폴더 구성
if (Test-Path -LiteralPath $packageDir) { Remove-Item -LiteralPath $packageDir -Recurse -Force }
New-Item -ItemType Directory -Path $packageDir | Out-Null
Copy-Item -LiteralPath (Join-Path $buildDir 'GitHubDesktopExplorer.dll') -Destination $packageDir
Copy-Item -LiteralPath (Join-Path $buildDir 'GitHubDesktopExplorerStub.exe') -Destination $packageDir
Copy-Item -LiteralPath (Join-Path $root 'package\AppxManifest.xml') -Destination $packageDir
Copy-Item -LiteralPath (Join-Path $root 'package\Assets') -Destination $packageDir -Recurse
Write-Output "패키지 폴더: $packageDir"
