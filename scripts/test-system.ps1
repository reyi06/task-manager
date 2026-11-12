$ErrorActionPreference = 'Stop'

# Both programs write UTF-8, so decode their output as UTF-8 before comparing.
[Console]::OutputEncoding = [System.Text.Encoding]::UTF8

$root = Split-Path -Parent $PSScriptRoot
$cpp = Join-Path $root 'build\cpp\task-cpp.exe'
$jdk = if ($env:JAVA_HOME) { $env:JAVA_HOME } else { 'C:\Users\reyes\toolchains\jdk21\jdk-21.0.12.1+1' }
$java = Join-Path $jdk 'bin\java.exe'
$classPath = (Join-Path $root 'java\out') + ';' + (Join-Path $root 'java\lib\*')
$database = Join-Path $root 'data\test-suite.db'
$previousDatabase = $env:TASKMAN_DB

# The screen is what the program writes to stdout. Stderr (crash messages, JDK
# warnings) is not part of it, so it is not captured or compared.
function Invoke-Cpp([string[]]$lines) {
    return (($lines -join "`n") | & $cpp 2>$null | Out-String)
}

function Invoke-Java([string[]]$lines) {
    return (($lines -join "`n") | & $java -cp $classPath Main 2>$null | Out-String)
}

function Assert-Contains([string]$output, [string]$expected, [string]$name) {
    if (-not $output.Contains($expected)) {
        throw "FAIL: $name (missing: $expected)"
    }
    Write-Host "PASS: $name"
}

# The two programs must draw the same screen from the same database. The only
# allowed difference is the edition name in the header.
function Assert-SameScreen([string]$cppOutput, [string]$javaOutput, [string]$name) {
    # The edition label is the only intended difference; hide it and the gap that
    # precedes it, since that gap is wider for the shorter word "C++".
    $normalize = {
        param($text)
        ($text -replace "`r", "") -replace '\s+(?:C\+\+|Java) / SQLite', ' <EDITION>'
    }
    $cpp = & $normalize $cppOutput
    $java = & $normalize $javaOutput
    if ($cpp -ceq $java) {
        Write-Host "PASS: $name"
        return
    }

    $cppLines = $cpp -split "`n"
    $javaLines = $java -split "`n"
    $index = 0
    while ($index -lt $cppLines.Count -and $index -lt $javaLines.Count -and
        $cppLines[$index] -ceq $javaLines[$index]) {
        $index++
    }
    $cppLine = if ($index -lt $cppLines.Count) { $cppLines[$index] } else { '<no more output>' }
    $javaLine = if ($index -lt $javaLines.Count) { $javaLines[$index] } else { '<no more output>' }
    throw "FAIL: $name (first difference on line $($index + 1))`n  C++ : $cppLine`n  Java: $javaLine"
}

try {
    & (Join-Path $PSScriptRoot 'build-cpp.ps1') | Out-Null
    & (Join-Path $PSScriptRoot 'build-java.ps1') | Out-Null

    Remove-Item $database -Force -ErrorAction SilentlyContinue
    $env:TASKMAN_DB = $database

    $output = Invoke-Cpp @(
        '1', 'C++ task', 'high', 'School', '12-01',
        '1', 'Shared task', 'normal', 'Work', '11-15',
        '2', '7', '9'
    )
    Assert-Contains $output 'Added task #1.' 'C++ add task 1'
    Assert-Contains $output 'Added task #2.' 'C++ add task 2'
    Assert-Contains $output 'C++ task' 'C++ list shows own task'
    Assert-Contains $output 'Shared task' 'C++ list shows second task'
    Assert-Contains $output 'TASK STATISTICS' 'C++ statistics panel'
    Assert-Contains $output 'Total tasks' 'C++ statistics labels'
    Assert-Contains $output '2 tasks' 'C++ statistics count'
    Assert-Contains $output '3 HIGH' 'C++ priority code 3 is high'
    Assert-Contains $output '2 NORM' 'C++ priority code 2 is normal'

    $output = Invoke-Java @('2', '9')
    Assert-Contains $output 'C++ task' 'Java reads C++ shared database'
    Assert-Contains $output 'Shared task' 'Java reads second shared task'

    $output = Invoke-Java @(
        '1', 'Java task', 'low', 'Personal', '',
        '4', '3', '2', '9'
    )
    Assert-Contains $output 'Added task #3.' 'Java add task'
    Assert-Contains $output 'Task #3 is now complete.' 'Java toggle complete'
    Assert-Contains $output '[x]' 'Java list shows completed status'
    Assert-Contains $output '1 LOW' 'Java priority code 1 is low'

    $output = Invoke-Cpp @('2', '7', '5', '2', 'y', '8', 'y', '2', '9')
    Assert-Contains $output 'Java task' 'C++ sees Java-created task'
    Assert-Contains $output 'Total tasks' 'C++ sees all shared tasks'
    Assert-Contains $output '1 completed' 'C++ sees shared completion'
    Assert-Contains $output 'SELECT A TASK' 'C++ shows task picker before delete'
    Assert-Contains $output 'Deleted task #2.' 'C++ delete confirmation'
    Assert-Contains $output 'Cleared 1 completed task(s).' 'C++ clear completed'

    $output = Invoke-Cpp @('1', '', '1', 'Bad date', 'normal', 'General', '02-30', '4', '999', '9')
    Assert-Contains $output 'Title cannot be empty.' 'C++ rejects empty title'
    Assert-Contains $output 'Due date must be a valid date' 'C++ rejects invalid date'
    Assert-Contains $output 'Task #999 was not found.' 'C++ rejects missing ID'

    $output = Invoke-Java @('1', '', '1', 'Bad date', 'normal', 'General', '02-29', '4', '999', '9')
    Assert-Contains $output 'Title cannot be empty.' 'Java rejects empty title'
    Assert-Contains $output 'Due date must be a valid date' 'Java rejects invalid date'
    Assert-Contains $output 'Task #999 was not found.' 'Java rejects missing ID'

    # Read-only session: list, two searches, statistics, quit. Both programs see
    # the same rows, so every line of the screen must match.
    $accented = 'Caf' + [char]0x00E9 + ' d' + [char]0x00E9 + 'j' + [char]0x00E0 + ' vu'
    Invoke-Cpp @('1', $accented, 'normal', 'School', '11-20', '9') | Out-Null

    $parity = @('2', '6', 'task', 'all', 'all', '', '6', '', 'done', 'all', '', '7', '9')
    Assert-SameScreen (Invoke-Cpp $parity) (Invoke-Java $parity) 'C++ and Java draw the same screen'
    Assert-Contains (Invoke-Java @('2', '9')) $accented 'Java prints an accented title unchanged'

    Write-Host ''
    Write-Host 'All tests passed.'
} finally {
    Remove-Item $database -Force -ErrorAction SilentlyContinue
    if ($null -eq $previousDatabase) {
        Remove-Item Env:TASKMAN_DB -ErrorAction SilentlyContinue
    } else {
        $env:TASKMAN_DB = $previousDatabase
    }
}
