$ErrorActionPreference = 'Stop'

$root = Split-Path -Parent $PSScriptRoot
$jdk = if ($env:JAVA_HOME) { $env:JAVA_HOME } else { 'C:\Users\reyes\toolchains\jdk21\jdk-21.0.12.1+1' }
$javac = Join-Path $jdk 'bin\javac.exe'

if (-not (Test-Path $javac)) {
    throw "javac not found at $javac. Update JAVA_HOME or scripts/build-java.ps1."
}

$sources = Get-ChildItem (Join-Path $root 'java') -Recurse -Filter '*.java' | Select-Object -ExpandProperty FullName
$output = Join-Path $root 'java\out'
$libraries = Join-Path $root 'java\lib\*'

if ($sources.Count -eq 0) {
    throw "No Java source files found."
}

Remove-Item $output -Recurse -Force -ErrorAction SilentlyContinue
New-Item -ItemType Directory -Force -Path $output | Out-Null

& $javac --release 17 -encoding UTF-8 -cp $libraries -d $output $sources
if ($LASTEXITCODE -ne 0) {
    exit $LASTEXITCODE
}

Write-Host "Built Java classes in $output"
