$ErrorActionPreference = 'Stop'

$root = Split-Path -Parent $PSScriptRoot
$executable = Join-Path $root 'build\cpp\task-cpp.exe'
$dataDirectory = Join-Path $root 'data'

if (-not (Test-Path $executable)) {
    throw "C++ program not built. Run the Build C++ task first."
}

New-Item -ItemType Directory -Force -Path $dataDirectory | Out-Null
$env:TASKMAN_DB = Join-Path $dataDirectory 'tasks.db'

Push-Location $root
try {
    & $executable
    exit $LASTEXITCODE
} finally {
    Pop-Location
}
