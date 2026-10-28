function Remove-LegacyContextMenu {
    $key = 'HKCU:\Software\Classes\Directory\Background\shell\GitHubDesktop'
    if (Test-Path -LiteralPath $key) {
        Remove-Item -LiteralPath $key -Recurse -Force
        Write-Output "제거: $key"
    }

    # 이 이름의 파일만 대상으로 한다. 다른 사용자 파일은 건드리지 않는다.
    $script = Join-Path $env:USERPROFILE 'Open-GitHubDesktop.ps1'
    if (Test-Path -LiteralPath $script) {
        Remove-Item -LiteralPath $script -Force
        Write-Output "제거: $script"
    }
}
