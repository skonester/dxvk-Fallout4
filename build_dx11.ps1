# build_dx11.ps1 - Native Windows build script for DXVK D3D11 and DXGI.
# Requires Visual Studio Build Tools, LLVM/Clang, Ninja, Meson, and glslangValidator.

$ErrorActionPreference = "Stop"

function Prepend-Path {
    param([Parameter(Mandatory = $true)][string]$Path)

    if (-not (Test-Path -LiteralPath $Path)) {
        return
    }

    $fullPath = [System.IO.Path]::GetFullPath($Path)
    $parts = @($env:PATH -split ';' | Where-Object { $_ })
    if ($parts -notcontains $fullPath) {
        $env:PATH = "$fullPath;" + $env:PATH
    }
}

function Get-CommandSource {
    param([Parameter(Mandatory = $true)][string]$Name)

    $cmd = Get-Command $Name -ErrorAction SilentlyContinue
    if ($cmd) {
        return $cmd.Source
    }

    return $null
}

function Find-VcVarsAll {
    $vswhereCandidates = @(
        (Join-Path ${env:ProgramFiles(x86)} "Microsoft Visual Studio\Installer\vswhere.exe"),
        (Join-Path $env:ProgramFiles "Microsoft Visual Studio\Installer\vswhere.exe")
    ) | Where-Object { $_ -and (Test-Path -LiteralPath $_) }

    foreach ($vswhere in $vswhereCandidates) {
        $installPath = & $vswhere -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
        if ($installPath) {
            $vcvars = Join-Path $installPath "VC\Auxiliary\Build\vcvarsall.bat"
            if (Test-Path -LiteralPath $vcvars) {
                return $vcvars
            }
        }
    }

    $roots = @(
        (Join-Path ${env:ProgramFiles(x86)} "Microsoft Visual Studio"),
        (Join-Path $env:ProgramFiles "Microsoft Visual Studio")
    ) | Where-Object { $_ -and (Test-Path -LiteralPath $_) }

    foreach ($root in $roots) {
        $vcvars = Get-ChildItem -Path $root -Filter vcvarsall.bat -Recurse -ErrorAction SilentlyContinue |
            Where-Object { $_.FullName -match "\\VC\\Auxiliary\\Build\\vcvarsall\.bat$" } |
            Select-Object -ExpandProperty FullName -First 1

        if ($vcvars) {
            return $vcvars
        }
    }

    return $null
}

function Find-LlvmBin {
    if ($env:DXVK_LLVM_BIN -and (Test-Path -LiteralPath (Join-Path $env:DXVK_LLVM_BIN "llvm-lib.exe"))) {
        return [System.IO.Path]::GetFullPath($env:DXVK_LLVM_BIN)
    }

    $llvmLib = Get-CommandSource "llvm-lib.exe"
    if ($llvmLib) {
        return Split-Path -Parent $llvmLib
    }

    $candidates = @(
        (Join-Path $env:ProgramFiles "LLVM\bin"),
        (Join-Path ${env:ProgramFiles(x86)} "LLVM\bin"),
        (Join-Path $env:USERPROFILE "scoop\apps\llvm\current\bin")
    ) | Where-Object { $_ -and (Test-Path -LiteralPath (Join-Path $_ "llvm-lib.exe")) }

    return $candidates | Select-Object -First 1
}

function Find-GlslangDir {
    function Test-GlslangValidator {
        param([Parameter(Mandatory = $true)][string]$File)

        if (-not (Test-Path -LiteralPath $File)) {
            return $false
        }

        $helpText = & $File -h 2>&1
        return [string]::Join("`n", $helpText) -match "vulkan1\.3"
    }

    if ($env:DXVK_GLSLANG_DIR -and (Test-Path -LiteralPath (Join-Path $env:DXVK_GLSLANG_DIR "glslangValidator.exe"))) {
        $candidate = Join-Path $env:DXVK_GLSLANG_DIR "glslangValidator.exe"
        if (Test-GlslangValidator $candidate) {
            return [System.IO.Path]::GetFullPath($env:DXVK_GLSLANG_DIR)
        }
        throw "DXVK_GLSLANG_DIR points to glslangValidator.exe, but it does not support --target-env vulkan1.3."
    }

    $roots = @()
    if ($env:VCPKG_ROOT) {
        $roots += (Join-Path $env:VCPKG_ROOT "installed")
    }
    $roots += @(
        (Join-Path $env:USERPROFILE "scoop\apps\vcpkg\current\installed"),
        (Join-Path $env:USERPROFILE "vcpkg\installed"),
        (Join-Path $PSScriptRoot "vcpkg_installed")
    )

    foreach ($root in ($roots | Where-Object { $_ -and (Test-Path -LiteralPath $_) })) {
        $matches = Get-ChildItem -Path $root -Filter glslangValidator.exe -Recurse -ErrorAction SilentlyContinue
        foreach ($match in $matches) {
            if (Test-GlslangValidator $match.FullName) {
                return $match.DirectoryName
            }
        }
    }

    $glslang = Get-CommandSource "glslangValidator.exe"
    if ($glslang -and (Test-GlslangValidator $glslang)) {
        return Split-Path -Parent $glslang
    }

    return $null
}

function Invoke-MesonSetup {
    param(
        [Parameter(Mandatory = $true)][string]$BuildDir,
        [Parameter(Mandatory = $true)][string[]]$MesonOptions
    )

    $meson = Get-CommandSource "meson.exe"
    if ($meson) {
        & $meson setup $BuildDir @MesonOptions
        if ($LASTEXITCODE -ne 0) {
            throw "Meson setup failed with exit code $LASTEXITCODE."
        }
        return
    }

    $python = Get-CommandSource "python.exe"
    if (-not $python) {
        $python = Get-CommandSource "py.exe"
    }
    if (-not $python) {
        throw "Could not find meson.exe, python.exe, or py.exe. Install Meson or Python with the meson package."
    }

    if ((Split-Path -Leaf $python) -ieq "py.exe") {
        & $python -3 -m mesonbuild.mesonmain setup $BuildDir @MesonOptions
    } else {
        & $python -m mesonbuild.mesonmain setup $BuildDir @MesonOptions
    }

    if ($LASTEXITCODE -ne 0) {
        throw "Meson setup failed with exit code $LASTEXITCODE."
    }
}

$buildDir = if ($env:DXVK_BUILD_DIR) { $env:DXVK_BUILD_DIR } else { "build_clang" }
$mesonOptions = @(
    "--backend", "ninja",
    "-Denable_d3d8=false",
    "-Denable_d3d9=false",
    "-Denable_d3d10=false",
    "-Db_vscrt=mt"
)

if ($env:DXVK_EXPERIMENTAL_SIMD -eq "1") {
    $mesonOptions += "-Ddxvk_experimental_simd=true"
}

Write-Host "Locating Visual Studio C++ build environment..." -ForegroundColor Cyan
$vcvars = Find-VcVarsAll
if (-not $vcvars) {
    throw "Could not locate vcvarsall.bat. Install Visual Studio Build Tools with the C++ workload."
}
Write-Host "Found vcvarsall.bat at: $vcvars" -ForegroundColor Green

Write-Host "Initializing MSVC x64 build environment..." -ForegroundColor Cyan
$envLines = cmd.exe /c "call `"$vcvars`" amd64 && set"
foreach ($line in $envLines) {
    if ($line -match "^([^=]+)=(.*)$") {
        $name = $Matches[1]
        $val = $Matches[2]
        if ($name -in "PATH", "INCLUDE", "LIB", "LIBPATH") {
            Set-Item "env:$name" $val
        }
    }
}

Write-Host "Locating LLVM clang-cl tools..." -ForegroundColor Cyan
$llvmBin = Find-LlvmBin
if (-not $llvmBin) {
    throw "Could not find llvm-lib.exe. Install LLVM/Clang, add LLVM\bin to PATH, or set DXVK_LLVM_BIN."
}
Prepend-Path $llvmBin
$env:CC = "clang-cl.exe"
$env:CXX = "clang-cl.exe"
$env:AR = "llvm-lib.exe"
$env:LD = "lld-link.exe"
$env:RC = "llvm-rc.exe"
Write-Host "Prioritized LLVM tools at: $llvmBin" -ForegroundColor Green

foreach ($tool in @($env:CC, $env:CXX, $env:AR, $env:LD, $env:RC, "ninja.exe")) {
    if (-not (Get-CommandSource $tool)) {
        throw "Required build tool '$tool' was not found on PATH."
    }
}

Write-Host "Locating glslang validator..." -ForegroundColor Cyan
$glslangDir = Find-GlslangDir
if (-not $glslangDir) {
    throw "Could not find glslangValidator.exe. Install glslang[tools] with vcpkg, add it to PATH, or set DXVK_GLSLANG_DIR."
}
Prepend-Path $glslangDir
Write-Host "Prioritized glslang at: $glslangDir" -ForegroundColor Green

if (Test-Path $buildDir) {
    $mesonInfoPath = Join-Path $buildDir "meson-info\meson-info.json"
    if (Test-Path $mesonInfoPath) {
        $mesonInfo = Get-Content $mesonInfoPath -Raw | ConvertFrom-Json
        $sourceDir = [System.IO.Path]::GetFullPath($mesonInfo.directories.source)
        $currentDir = [System.IO.Path]::GetFullPath((Get-Location).Path)

        if ($sourceDir -ne $currentDir) {
            Write-Host "Build directory belongs to a different source tree. Reconfiguring..." -ForegroundColor Yellow
            Remove-Item -LiteralPath $buildDir -Recurse -Force
        } elseif ($env:DXVK_RECONFIGURE -eq "1") {
            Write-Host "DXVK_RECONFIGURE=1 set. Reconfiguring build directory..." -ForegroundColor Yellow
            Remove-Item -LiteralPath $buildDir -Recurse -Force
        }
    } else {
        Write-Host "Build directory exists without Meson metadata. Reconfiguring..." -ForegroundColor Yellow
        Remove-Item -LiteralPath $buildDir -Recurse -Force
    }
}

if (-not (Test-Path $buildDir)) {
    Write-Host "Configuring build directory with Meson..." -ForegroundColor Cyan
    Invoke-MesonSetup -BuildDir $buildDir -MesonOptions $mesonOptions
} else {
    Write-Host "Build directory already exists. Skipping setup configuration." -ForegroundColor Yellow
    Write-Host "Set DXVK_RECONFIGURE=1 or DXVK_BUILD_DIR to a new path if Meson cached older tool choices." -ForegroundColor Yellow
}

Write-Host "Starting build..." -ForegroundColor Cyan
ninja -C $buildDir
if ($LASTEXITCODE -ne 0) {
    throw "Ninja build failed with exit code $LASTEXITCODE."
}

Write-Host "Build complete! Output binaries:" -ForegroundColor Green
Get-ChildItem "$buildDir/src/**/*.dll" | Select-Object Name, FullName, Length | Format-Table
