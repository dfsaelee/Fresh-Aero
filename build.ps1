<#
.SYNOPSIS
    Unified build script for G3X Fresh Air VST3/Standalone plugin.
#>
[CmdletBinding()]
param(
    [switch]$Clean,
    [switch]$SkipConfigure,
    [ValidateSet('All', 'VST3', 'Standalone')]
    [string]$Target = 'All',
    [int]$Jobs = 0
)

$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest

$root = $PSScriptRoot
$toolsMingw = Join-Path $root "tools\mingw64\bin"
$toolsCmake = Join-Path $root "tools\cmake\bin"
$buildDir   = Join-Path $root "build"
$vst3Bundle = Join-Path $buildDir "G3XFreshAir_artefacts\VST3\G3X Fresh Air.vst3"
$vst3Binary = Join-Path $vst3Bundle "Contents\x86_64-win\G3X Fresh Air.vst3"
$moduleInfo = Join-Path $vst3Bundle "Contents\Resources\moduleinfo.json"
$standalone = Join-Path $buildDir "G3XFreshAir_artefacts\Standalone\G3X Fresh Air.exe"

function Write-Header([string]$text) {
    Write-Host ""
    Write-Host "==================================================" -ForegroundColor Cyan
    Write-Host "  $text" -ForegroundColor Cyan
    Write-Host "==================================================" -ForegroundColor Cyan
}

function Write-Status([string]$label, [string]$status, [string]$color = 'Green') {
    Write-Host ("  {0,-14}: " -f $label) -NoNewline
    Write-Host $status -ForegroundColor $color
}

function Invoke-Build([string[]]$arguments) {
    & cmake @arguments
    if ($LASTEXITCODE -ne 0) {
        throw "Build command failed with exit code $LASTEXITCODE"
    }
}

try {
    Write-Header "G3X Fresh Air Build System"

    if (-not (Test-Path $toolsMingw)) {
        throw "MinGW toolchain not found at: $toolsMingw"
    }
    if (-not (Test-Path $toolsCmake)) {
        $toolsCmake = $toolsMingw
    }

    $env:PATH = "$toolsMingw;$toolsCmake;$($env:PATH)"

    $cmakeVer = (& cmake --version)[0]
    $gccVer   = (& g++ --version)[0]
    Write-Host "  CMake:  $cmakeVer" -ForegroundColor DarkGray
    Write-Host "  GCC:    $gccVer" -ForegroundColor DarkGray

    if ($Jobs -le 0) {
        $Jobs = [Environment]::ProcessorCount
        if ($Jobs -lt 1) { $Jobs = 4 }
    }
    Write-Host "  Jobs:   $Jobs" -ForegroundColor DarkGray

    if ($Clean) {
        if (Test-Path $buildDir) {
            Write-Host "`n  Cleaning build directory..." -ForegroundColor Yellow
            Remove-Item -LiteralPath $buildDir -Recurse -Force
        }
    }

    if (-not $SkipConfigure) {
        Write-Header "Configuring (MinGW Makefiles)"
        Invoke-Build @('-S', $root, '-B', $buildDir, '-G', 'MinGW Makefiles')
        Write-Host "  Configure complete." -ForegroundColor Green
    }

    Write-Header "Building ($Target, Release, -j $Jobs)"
    $buildArgs = @('--build', $buildDir, '--config', 'Release', '-j', "$Jobs")
    switch ($Target) {
        'VST3'       { $buildArgs += @('--target', 'G3XFreshAir_VST3') }
        'Standalone' { $buildArgs += @('--target', 'G3XFreshAir_Standalone') }
    }
    Invoke-Build $buildArgs
    Write-Host "  Build complete." -ForegroundColor Green

    Write-Header "Validation"
    $allPassed = $true

    if ($Target -eq 'All' -or $Target -eq 'VST3') {
        if (Test-Path $vst3Binary) {
            $size = (Get-Item -LiteralPath $vst3Binary).Length
            $sizeMB = [math]::Round($size / 1MB, 1)
            Write-Status "VST3" "[OK] $sizeMB MB"
        } else {
            Write-Status "VST3" "[FAIL] Binary not found!" Red
            $allPassed = $false
        }

        if (Test-Path $moduleInfo) {
            $json = Get-Content -LiteralPath $moduleInfo -Raw
            if ($json -match '"Name"' -and $json -match '"Classes"') {
                Write-Status "moduleinfo" "[OK] Valid manifest"
            } else {
                Write-Status "moduleinfo" "[WARN] Present but may be invalid" Yellow
            }
        } else {
            Write-Status "moduleinfo" "[WARN] Not found" Yellow
        }

        if ((Test-Path $vst3Binary) -and (Get-Command objdump -ErrorAction SilentlyContinue)) {
            $deps = & objdump -p $vst3Binary 2>$null | Select-String "DLL Name:"
            $mingwDeps = $deps | Where-Object { $_ -match "libstdc|libgcc|libwinpthread" }
            if ($mingwDeps) {
                Write-Status "MinGW DLLs" "[FAIL] Found: $($mingwDeps -join ', ')" Red
                $allPassed = $false
            } else {
                Write-Status "MinGW DLLs" "[OK] None (statically linked)"
            }
        }
    }

    if ($Target -eq 'All' -or $Target -eq 'Standalone') {
        if (Test-Path $standalone) {
            Write-Status "Standalone" "[OK] Built"
        } else {
            Write-Status "Standalone" "[FAIL] Not found!" Red
            $allPassed = $false
        }
    }

    Write-Host ""
    if ($allPassed) {
        Write-Host "================ BUILD SUCCEEDED ================" -ForegroundColor Green
    } else {
        Write-Host "========= BUILD COMPLETED WITH WARNINGS =========" -ForegroundColor Yellow
    }
    Write-Host ""
}
catch {
    Write-Host "`n  BUILD FAILED" -ForegroundColor Red
    Write-Host "  Error: $_" -ForegroundColor Red
    exit 1
}
