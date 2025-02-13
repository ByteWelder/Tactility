<#
.SYNOPSIS
    Usage: build.ps1 [boardname]
    Example: build.ps1 lilygo-tdeck
    Description: Makes a clean build for the specified board.
#>

function EchoNewPhase {
    param (
        [string]$message
    )
    Write-Host "⏳ $message" -ForegroundColor Cyan
}

function FatalError {
    param (
        [string]$message
    )
    Write-Host "⚠️ $message" -ForegroundColor Red
    exit 0
}

$sdkconfig_file = "sdkconfig.board.$($args[0])"

if ($args.Count -lt 1) {
    FatalError "Must pass board name as first argument. (e.g. lilygo_tdeck)"
}

if (-Not (Test-Path $sdkconfig_file)) {
    FatalError "Board not found: $sdkconfig_file"
}

EchoNewPhase "Cleaning build folder"
$BuildFolder = "build"
if (Test-Path $BuildFolder) {
    Remove-Item -Path $BuildFolder -Recurse -Force
    EchoNewPhase "Build folder deleted"
} else {
    EchoNewPhase "Build folder doesn't exist."
}

EchoNewPhase "Building $sdkconfig_file"

Copy-Item -Path $sdkconfig_file -Destination "sdkconfig"

try {
    & idf.py build
} catch {
    FatalError "Failed to build esp32s3 SDK"
}
