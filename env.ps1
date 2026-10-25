$env:RTT_ROOT = 'D:\env\rt-thread-5.3.0\rt-thread-5.3.0'
$env:GD32F50X_SDK = "$PSScriptRoot\thirdparty\GD32F50x_Firmware_Library_V1.0.3"
$env:Path = "$env:RTT_ROOT\tools;$env:Path"
Write-Host "RTT_ROOT=$env:RTT_ROOT"
Write-Host "GD32F50X_SDK=$env:GD32F50X_SDK"
