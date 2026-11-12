$ErrorActionPreference = 'Stop'

$root = Split-Path -Parent $PSScriptRoot
$mingw = 'C:\Users\reyes\AppData\Local\Microsoft\WinGet\Packages\BrechtSanders.WinLibs.POSIX.UCRT_Microsoft.Winget.Source_8wekyb3d8bbwe\mingw64\bin'
$gcc = Join-Path $mingw 'gcc.exe'
$gxx = Join-Path $mingw 'g++.exe'

if (-not (Test-Path $gxx)) {
    # No WinLibs install here: use whatever g++ is on PATH.
    $gcc = 'gcc'
    $gxx = 'g++'
    if (-not (Get-Command $gxx -ErrorAction SilentlyContinue)) {
        throw "No C++ compiler found. Install WinLibs (g++), or add g++ to PATH."
    }
}

$build = Join-Path $root 'build\cpp'
$sqliteSource = Join-Path $root 'cpp\third_party\sqlite\sqlite3.c'
$sqliteObject = Join-Path $build 'sqlite3.o'
$sources = Get-ChildItem (Join-Path $root 'cpp\src') -Filter '*.cpp' | Select-Object -ExpandProperty FullName
$output = Join-Path $build 'task-cpp.exe'

New-Item -ItemType Directory -Force -Path $build | Out-Null

$needsSqliteBuild = -not (Test-Path $sqliteObject)
if (-not $needsSqliteBuild) {
    $needsSqliteBuild = (Get-Item $sqliteSource).LastWriteTime -gt (Get-Item $sqliteObject).LastWriteTime
}

if ($needsSqliteBuild) {
    & $gcc -O2 -c $sqliteSource -o $sqliteObject
    if ($LASTEXITCODE -ne 0) {
        exit $LASTEXITCODE
    }
}

& $gxx -std=c++17 -g -Wall -Wextra -static -static-libgcc -static-libstdc++ -I (Join-Path $root 'cpp\include') -I (Join-Path $root 'cpp\third_party\sqlite') $sources $sqliteObject -o $output
if ($LASTEXITCODE -ne 0) {
    exit $LASTEXITCODE
}

Write-Host "Built $output"
