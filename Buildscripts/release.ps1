<#
.SYNOPSIS
Releases the current build labeled as a release for the specified board name.

.DESCRIPTION
Usage: .\release.ps1 [boardname]
Example: .\release.ps1 lilygo-tdeck

.PARAMETER board
The name of the board to release.
#>

function EchoNewPhase {
    param(
        [string]$Message
    )
    Write-Host "⏳ $message" -ForegroundColor Cyan
}

function FatalError {
    param(
        [string]$Message
    )
    Write-Host "⚠️ $message" -ForegroundColor Red
    exit 0
}

function Release-Symbols {
    param(
        [string]$TargetPath
    )
    
    EchoNewPhase "Making symbols release at '$TargetPath'"
    New-Item -ItemType Directory -Path $TargetPath -Force | Out-Null
    Copy-Item -Path "build\*.elf" -Destination $TargetPath -Force
}

function Release-Build {
    param(
        [string]$TargetPath
    )
    
    EchoNewPhase "Making release at '$TargetPath'"

    $binPath = Join-Path $TargetPath "Binaries"
    $partitionTablePath = Join-Path $binPath "partition_table"
    $bootloaderPath = Join-Path $binPath "bootloader"

    New-Item -ItemType Directory -Path $binPath -Force | Out-Null
    New-Item -ItemType Directory -Path $partitionTablePath -Force | Out-Null
    New-Item -ItemType Directory -Path $bootloaderPath -Force | Out-Null

    Copy-Item -Path "build\*.bin" -Destination $binPath -Force
    Copy-Item -Path "build\bootloader\*.bin" -Destination $bootloaderPath -Force
    Copy-Item -Path "build\partition_table\*.bin" -Destination $partitionTablePath -Force
    Copy-Item -Path "build\flash_args" -Destination $binPath -Force
    Copy-Item -Path "build\flasher_args.json" -Destination $binPath -Force

    Copy-Item -Path "Buildscripts\Flashing\*" -Destination $TargetPath -Force
}

# Script start
$releasePath = "release"
$sdkconfig_file = "sdkconfig.board.$($args[0])"
$board = "$($args[0])"

if ($args.Count -lt 1) {
    FatalError "Must pass board name as first argument. (e.g. lilygo_tdeck)"
}

if (-not (Test-Path $sdkconfig_file)) {
    FatalError "Board not found: $sdkconfig_file"
}

$targetReleasePath = Join-Path $releasePath "Tactility-$board"
$targetSymbolsPath = Join-Path $releasePath "Tactility-$board-symbols"

Release-Build $targetReleasePath
Release-Symbols $targetSymbolsPath
