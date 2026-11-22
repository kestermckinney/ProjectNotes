<#
.SYNOPSIS
  Builds the Microsoft Store (MSIX) package for Project Notes.

.DESCRIPTION
  Packages the files the `deploy` CMake target staged, the same payload the
  GitHub installer ships, minus the Remote Host. See Releasing.md next to this
  script for the one-time Partner Center setup.

  Store identity defaults to the Project Notes Pro Product identity in Partner
  Center. These environment variables can override the defaults:
    PN_MSIX_IDENTITY_NAME           Package/Identity/Name
    PN_MSIX_PUBLISHER               Package/Identity/Publisher (CN=...)
    PN_MSIX_PUBLISHER_DISPLAY_NAME  Package/Properties/PublisherDisplayName

.PARAMETER Local
  Skip Partner Center identity and sign with a throwaway self-signed
  certificate so the package can be installed on this machine for testing.
  Such a package cannot be uploaded to the Store.

.PARAMETER BuildDir
  CMake build directory that holds deploy\. Defaults to the Release kit
  directory build_installer.bat uses.
#>
param(
    [switch]$Local,
    [string]$BuildDir = (Join-Path $PSScriptRoot '..\..\..\build\Desktop_Qt_6_10_0_MSVC2022_64bit-Release'),
    [switch]$SkipDeploy
)

$ErrorActionPreference = 'Stop'
$repo      = (Resolve-Path (Join-Path $PSScriptRoot '..\..\..')).Path
$BuildDir  = (Resolve-Path $BuildDir).Path
$deployDir = Join-Path $BuildDir 'deploy'
$outDir    = Join-Path $PSScriptRoot 'output'
$layout    = Join-Path $outDir 'layout'

# --- Version: project(ProjectNotes VERSION x.y.z). The Store needs a revision of 0.
$cmakeText = Get-Content (Join-Path $repo 'CMakeLists.txt') -Raw
if ($cmakeText -notmatch 'project\(ProjectNotes\s+VERSION\s+(\d+\.\d+\.\d+)') { throw 'Could not read the version from CMakeLists.txt.' }
$version = "$($Matches[1]).0"

# --- Identity
if ($Local) {
    $identityName = 'ProjectNotes.LocalTest'
    $publisher    = 'CN=ProjectNotesLocalTest'
    $publisherDisplay = 'Project Notes (local test)'
} else {
    $identityName     = if ($env:PN_MSIX_IDENTITY_NAME) { $env:PN_MSIX_IDENTITY_NAME } else { 'ProjectNotesPro.ProjectNotesPro' }
    $publisher        = if ($env:PN_MSIX_PUBLISHER) { $env:PN_MSIX_PUBLISHER } else { 'CN=33262805-4644-4CFE-9F20-931ECA1E1ACF' }
    $publisherDisplay = if ($env:PN_MSIX_PUBLISHER_DISPLAY_NAME) { $env:PN_MSIX_PUBLISHER_DISPLAY_NAME } else { 'Project Notes Pro' }
    foreach ($pair in @(@('PN_MSIX_IDENTITY_NAME', $identityName), @('PN_MSIX_PUBLISHER', $publisher), @('PN_MSIX_PUBLISHER_DISPLAY_NAME', $publisherDisplay))) {
        if (-not $pair[1]) { throw "Set $($pair[0]) to the value on Partner Center's Product identity page, or pass -Local." }
    }
}

# --- Windows SDK tools
function Find-SdkTool([string]$name) {
    $kits = 'C:\Program Files (x86)\Windows Kits\10\bin'
    $hit = Get-ChildItem $kits -Directory | Where-Object Name -Match '^10\.' | Sort-Object { [version]$_.Name } -Descending |
        ForEach-Object { Join-Path $_.FullName "x64\$name" } | Where-Object { Test-Path $_ } | Select-Object -First 1
    if (-not $hit) { throw "$name not found. Install the Windows 10/11 SDK." }
    $hit
}
$makeappx = Find-SdkTool 'makeappx.exe'

# --- Stage the payload
if (-not $SkipDeploy) {
    & cmake --build $BuildDir --target deploy
    if ($LASTEXITCODE -ne 0) { throw 'cmake deploy target failed.' }
}
if (-not (Test-Path (Join-Path $deployDir 'Project Notes.exe'))) { throw "No staged build in $deployDir." }

if (Test-Path $layout) { Remove-Item $layout -Recurse -Force }
New-Item -ItemType Directory -Force $layout | Out-Null
# The Remote Host has its own installer and is not part of the Store listing.
robocopy $deployDir $layout /E /NFL /NDL /NJH /NJS /NP /XF 'Project Notes Remote Host.exe' 'LICENSE.txt' 'icuuc.dll' 'vc_redist.x64.exe' | Out-Null
if ($LASTEXITCODE -ge 8) { throw 'Copying the deploy directory failed.' }

# --- Logos, resized from the 512 px project logo
Add-Type -AssemblyName System.Drawing
$assets = Join-Path $layout 'Assets'
New-Item -ItemType Directory -Force $assets | Out-Null
$source = [System.Drawing.Image]::FromFile((Join-Path $repo 'docs\img\logo1024.png'))
function New-Logo([string]$file, [int]$w, [int]$h) {
    $bmp = New-Object System.Drawing.Bitmap $w, $h
    $g = [System.Drawing.Graphics]::FromImage($bmp)
    $g.InterpolationMode = 'HighQualityBicubic'; $g.SmoothingMode = 'HighQuality'; $g.PixelOffsetMode = 'HighQuality'
    $g.Clear([System.Drawing.Color]::Transparent)
    $side = [Math]::Min($w, $h)
    $g.DrawImage($source, [int](($w - $side) / 2), [int](($h - $side) / 2), $side, $side)
    $g.Dispose()
    $bmp.Save((Join-Path $assets $file), [System.Drawing.Imaging.ImageFormat]::Png)
    $bmp.Dispose()
}
New-Logo 'StoreLogo.png' 50 50
New-Logo 'Square44x44Logo.png' 44 44
New-Logo 'Square71x71Logo.png' 71 71
New-Logo 'Square150x150Logo.png' 150 150
New-Logo 'Wide310x150Logo.png' 310 150
New-Logo 'Square310x310Logo.png' 310 310
$source.Dispose()

# --- Manifest
$manifest = Get-Content (Join-Path $PSScriptRoot 'AppxManifest.xml.in') -Raw
$manifest = $manifest.Replace('@IDENTITY_NAME@', [Security.SecurityElement]::Escape($identityName)).
    Replace('@PUBLISHER@', [Security.SecurityElement]::Escape($publisher)).
    Replace('@PUBLISHER_DISPLAY_NAME@', [Security.SecurityElement]::Escape($publisherDisplay)).
    Replace('@VERSION@', $version)
[IO.File]::WriteAllText((Join-Path $layout 'AppxManifest.xml'), $manifest, (New-Object Text.UTF8Encoding $false))

# --- Pack
$suffix = if ($Local) { '-local-test' } else { '' }
$msix = Join-Path $outDir "ProjectNotes-$version-x64$suffix.msix"
if (Test-Path $msix) { Remove-Item $msix -Force }
& $makeappx pack /d $layout /p $msix /o
if ($LASTEXITCODE -ne 0) { throw 'makeappx failed.' }

if ($Local) {
    $signtool = Find-SdkTool 'signtool.exe'
    $cert = New-SelfSignedCertificate -Type Custom -Subject $publisher -KeyUsage DigitalSignature `
        -FriendlyName 'Project Notes local test' -CertStoreLocation 'Cert:\CurrentUser\My' `
        -TextExtension @('2.5.29.37={text}1.3.6.1.5.5.7.3.3', '2.5.29.19={text}')
    $cer = Join-Path $outDir 'ProjectNotesLocalTest.cer'
    Export-Certificate -Cert $cert -FilePath $cer | Out-Null
    & $signtool sign /fd SHA256 /sha1 $cert.Thumbprint $msix
    if ($LASTEXITCODE -ne 0) { throw 'signtool failed.' }
    Write-Host ''
    Write-Host "Test package: $msix"
    Write-Host "Trust the certificate once (admin PowerShell):"
    Write-Host "  Import-Certificate -FilePath '$cer' -CertStoreLocation Cert:\LocalMachine\TrustedPeople"
    Write-Host "Then: Add-AppxPackage '$msix'"
} else {
    Write-Host ''
    Write-Host "Store package: $msix"
    Write-Host 'Upload it in Partner Center. The Store signs it, so it is left unsigned.'
}
