<#
.SYNOPSIS
  Packs a JavaScript plugin folder into a .pnplugin file.

.DESCRIPTION
  The Windows counterpart of build-pnplugin.sh. A .pnplugin is a zip with
  plugin.json at its root, which users install from Settings > Plugins.

    packaging\plugins\build-pnplugin.ps1 extras\ifs-cloud [output-dir]

  The file is named <id>-<version>.pnplugin. Editor backups, dot files and
  TypeScript declaration files are left out. Needs nothing beyond Windows
  PowerShell 5.1; the .mjs files are syntax-checked when node is on PATH.

.PARAMETER PluginDir
  The plugin folder, holding plugin.json.

.PARAMETER OutputDir
  Where the package goes. Defaults to dist in the current directory.
#>
param(
    [Parameter(Mandatory = $true, Position = 0)][string]$PluginDir,
    [Parameter(Position = 1)][string]$OutputDir = 'dist'
)

$ErrorActionPreference = 'Stop'
Add-Type -AssemblyName System.IO.Compression
Add-Type -AssemblyName System.IO.Compression.FileSystem

$src = (Resolve-Path $PluginDir).Path.TrimEnd('\', '/')
$manifestPath = Join-Path $src 'plugin.json'
if (-not (Test-Path $manifestPath -PathType Leaf)) { throw "$src has no plugin.json" }

$manifest = Get-Content $manifestPath -Raw -Encoding UTF8 | ConvertFrom-Json
if (-not $manifest.id -or -not $manifest.version) { throw 'plugin.json needs an id and a version.' }
$main = if ($manifest.main) { $manifest.main } else { 'main.mjs' }
if (-not (Test-Path (Join-Path $src $main) -PathType Leaf)) { throw "main module $main is missing" }

# Everything except dot files, files in dot folders, editor backups and .d.ts files.
# EnumerateFiles keeps $src's spelling (Get-ChildItem expands 8.3 short names),
# so the relative paths come out right.
$files = [System.IO.Directory]::EnumerateFiles($src, '*', [System.IO.SearchOption]::AllDirectories) | ForEach-Object {
    $_.Substring($src.Length + 1).Replace('\', '/')
} | Where-Object {
    $parts = $_ -split '/'
    $name = $parts[-1]
    -not ($parts | Where-Object { $_.StartsWith('.') }) -and -not $name.EndsWith('~') -and -not $name.EndsWith('.d.ts')
} | Sort-Object -CaseSensitive

$node = Get-Command node -ErrorAction SilentlyContinue
if ($node) {
    foreach ($file in $files | Where-Object { $_.EndsWith('.mjs') }) {
        & $node.Source --check (Join-Path $src $file)
        if ($LASTEXITCODE -ne 0) { throw "$file has a syntax error" }
    }
} else {
    Write-Warning 'node is not on PATH; skipping the .mjs syntax check.'
}

New-Item -ItemType Directory -Force $OutputDir | Out-Null
$package = Join-Path (Resolve-Path $OutputDir).Path "$($manifest.id)-$($manifest.version).pnplugin"
if (Test-Path $package) { Remove-Item $package -Force }

# Entry names use forward slashes, as zip does; Compress-Archive in 5.1 writes backslashes.
$zip = [System.IO.Compression.ZipFile]::Open($package, [System.IO.Compression.ZipArchiveMode]::Create)
try {
    foreach ($file in $files) {
        [System.IO.Compression.ZipFileExtensions]::CreateEntryFromFile(
            $zip, (Join-Path $src $file), $file, [System.IO.Compression.CompressionLevel]::Optimal) | Out-Null
    }
} finally {
    $zip.Dispose()
}
$package
