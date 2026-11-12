$ErrorActionPreference = 'Stop'

$root = Split-Path -Parent $PSScriptRoot
$jdk = if ($env:JAVA_HOME) { $env:JAVA_HOME } else { 'C:\Users\reyes\toolchains\jdk21\jdk-21.0.12.1+1' }
$java = Join-Path $jdk 'bin\java.exe'
$classes = Join-Path $root 'java\out'
$libraries = Join-Path $root 'java\lib\*'
$dataDirectory = Join-Path $root 'data'

if (-not (Test-Path (Join-Path $classes 'Main.class'))) {
    throw "Java program not built. Run the Build Java task first."
}

New-Item -ItemType Directory -Force -Path $dataDirectory | Out-Null
$env:TASKMAN_DB = Join-Path $dataDirectory 'tasks.db'

Push-Location $root
try {
    # JDK 22+ warns when a library loads native code (sqlite-jdbc does). JDK 21
    # does not know the option, so read the installed version from the JDK itself.
    $majorVersion = 21
    $releaseFile = Join-Path $jdk 'release'
    if (Test-Path $releaseFile) {
        $releaseLine = Select-String -Path $releaseFile -Pattern '^JAVA_VERSION="(\d+)' |
            Select-Object -First 1
        if ($releaseLine) {
            $majorVersion = [int]$releaseLine.Matches[0].Groups[1].Value
        }
    }

    $javaArgs = @('-cp', "$classes;$libraries", 'Main')
    if ($majorVersion -ge 22) {
        $javaArgs = @('--enable-native-access=ALL-UNNAMED') + $javaArgs
    }

    & $java @javaArgs
    exit $LASTEXITCODE
} finally {
    Pop-Location
}
