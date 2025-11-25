# Stop the script when a cmdlet or a native command fails
$ErrorActionPreference = 'Stop'
$PSNativeCommandUseErrorActionPreference = $true

Write-Host "`nRunning emeraude-engine unit tests ..."

Write-Host "`n======= CONFIGURING ======`n"
cmake -S ./ -B ./cmake-build-utest -DCMAKE_BUILD_TYPE=Release -DEMERAUDE_ENABLE_TESTS=ON -G "Visual Studio 17 2022" -A x64
Write-Host "`n======= COMPILING ======`n"
cmake --build ./cmake-build-utest --config Release
Write-Host "`n======= RUNNING ======`n"
ctest --test-dir ./cmake-build-utest --verbose
Write-Host "`n======= FINISHED ======`n"
