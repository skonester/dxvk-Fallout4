# build_dx11.ps1 - Native Windows Build Script for DXVK D3D11 and DXGI
# Requires clang-cl and vcpkg (with glslang[tools]) to be installed.

$ErrorActionPreference = "Stop"
$buildDir = "build_clang"

# 1. Locate Visual Studio 2022 vcvarsall.bat
Write-Host "Locating Visual Studio 2022 Build Tools/vcvarsall.bat..." -ForegroundColor Cyan
$vcvars = Resolve-Path "C:\Program Files*\Microsoft Visual Studio\2022\*\VC\Auxiliary\Build\vcvarsall.bat" -ErrorAction SilentlyContinue | Select-Object -ExpandProperty Path -First 1

if (-not $vcvars) {
    Write-Error "Could not locate vcvarsall.bat. Please ensure Visual Studio 2022 is installed."
}
Write-Host "Found vcvarsall.bat at: $vcvars" -ForegroundColor Green

# 2. Extract environment variables from vcvarsall.bat
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

# 3. Prepend vcpkg glslang path
Write-Host "Locating vcpkg glslang validator..." -ForegroundColor Cyan
$glslangDir = "C:\Users\admin\scoop\apps\vcpkg\current\installed\x64-windows\tools\glslang"
if (-not (Test-Path $glslangDir)) {
    # Try finding it dynamically in case path is different
    $dynamicPath = Get-ChildItem -Path "C:\Users\admin\scoop\apps\vcpkg\current\installed" -Filter "glslangValidator.exe" -Recurse -ErrorAction SilentlyContinue | Select-Object -ExpandProperty DirectoryName -First 1
    if ($dynamicPath) {
        $glslangDir = $dynamicPath
    } else {
        Write-Error "Could not find glslangValidator.exe. Please install glslang[tools] via vcpkg."
    }
}
$env:PATH = "$glslangDir;" + $env:PATH
Write-Host "Prioritized glslang at: $glslangDir" -ForegroundColor Green

# 4. Set CC/CXX to clang-cl
$env:CC = "clang-cl"
$env:CXX = "clang-cl"

# 5. Check if build directory needs configuration
if (Test-Path $buildDir) {
    $mesonInfoPath = Join-Path $buildDir "meson-info\meson-info.json"
    if (Test-Path $mesonInfoPath) {
        $mesonInfo = Get-Content $mesonInfoPath -Raw | ConvertFrom-Json
        $sourceDir = [System.IO.Path]::GetFullPath($mesonInfo.directories.source)
        $currentDir = [System.IO.Path]::GetFullPath((Get-Location).Path)

        if ($sourceDir -ne $currentDir) {
            Write-Host "Build directory belongs to a different source tree. Reconfiguring..." -ForegroundColor Yellow
            Remove-Item -LiteralPath $buildDir -Recurse -Force
        }
    }
}

if (-not (Test-Path $buildDir)) {
    Write-Host "Configuring build directory with meson..." -ForegroundColor Cyan
    # Find python / meson
    $pythonExe = "C:\Users\admin\AppData\Local\Programs\Python\Python311\python.exe"
    if (-not (Test-Path $pythonExe)) {
        $pythonExe = "python" # fallback
    }
    & $pythonExe -m mesonbuild.mesonmain setup $buildDir --backend ninja -Denable_d3d8=false -Denable_d3d9=false -Denable_d3d10=false -Db_vscrt=mt
    if ($LASTEXITCODE -ne 0) {
        throw "Meson setup failed with exit code $LASTEXITCODE."
    }
} else {
    Write-Host "Build directory already exists. Skipping setup configuration." -ForegroundColor Yellow
}

# 6. Run Ninja Build
Write-Host "Starting build..." -ForegroundColor Cyan
ninja -C $buildDir
if ($LASTEXITCODE -ne 0) {
    throw "Ninja build failed with exit code $LASTEXITCODE."
}

Write-Host "Build complete! Output binaries:" -ForegroundColor Green
Get-ChildItem "$buildDir/src/**/*.dll" | Select-Object Name, FullName, Length | Format-Table
