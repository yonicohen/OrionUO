# One-time setup: point this package at an Ultima Online installation and play.
#
#   .\setup.ps1
#   .\setup.ps1 "C:\Program Files (x86)\Electronic Arts\Ultima Online Classic"
#   .\setup.ps1 -NoLaunch "C:\...\Ultima Online Classic"
#
# UO's data files are copyright and are not redistributed with this client.
# You need them from an existing install - your shard's package, or the free
# Classic Client from https://uo.com/client-download/ .

[CmdletBinding()]
param(
    [Parameter(Position = 0)][string]$UoPath = "",
    [switch]$NoLaunch
)

$ErrorActionPreference = "Stop"

$here = $PSScriptRoot
if (-not $here) { $here = Split-Path -Parent $MyInvocation.MyCommand.Path }
Set-Location $here
$dataDir = Join-Path $here "data"

function Say  { param($m) Write-Host $m }
function Step { param($m) Write-Host ""; Write-Host "==> $m" }
function Die  { param($m) Write-Host "error: $m" -ForegroundColor Red; exit 1 }

# ---------------------------------------------------------------------------
# 1. Where is Ultima Online?
# ---------------------------------------------------------------------------
if (-not $UoPath) {
    Say "This client needs the data files from an Ultima Online installation."
    Say "They are copyright and cannot be shipped with it."
    Say ""
    Say "That is the folder holding art.mul / ArtLegacyMUL.uop and map0.mul -"
    Say "either your shard's download, or an install of the free Classic"
    Say "Client from https://uo.com/client-download/ ."
    Say ""
    $UoPath = Read-Host "Path to your Ultima Online folder"
}

$UoPath = $UoPath.Trim().Trim('"').Trim("'")
if (-not $UoPath)                    { Die "no path given" }
if (-not (Test-Path -LiteralPath $UoPath -PathType Container)) {
    Die "'$UoPath' is not a directory"
}
$UoPath = (Resolve-Path -LiteralPath $UoPath).ProviderPath

# ---------------------------------------------------------------------------
# 2. Does it look like a real install?
# ---------------------------------------------------------------------------
Step "Checking $UoPath"

$entries = Get-ChildItem -LiteralPath $UoPath -Force
function HasEntry { param($name) return [bool]($entries | Where-Object { $_.Name -ieq $name }) }

$haveArt = (HasEntry "art.mul")  -or (HasEntry "ArtLegacyMUL.uop")
$haveMap = (HasEntry "map0.mul") -or (HasEntry "map0LegacyMUL.uop")

if (-not ($haveArt -and $haveMap)) {
    Say "    This does not look like a UO client directory."
    if (-not $haveArt) { Say "    missing: art.mul or ArtLegacyMUL.uop" }
    if (-not $haveMap) { Say "    missing: map0.mul or map0LegacyMUL.uop" }
    Say ""
    Say "    Point this at the folder that holds the game data itself, not at"
    Say "    the installer, the .zip, or the folder containing it."
    Die "no UO data found in '$UoPath'"
}
Say "    art and map data: found"

$haveClientCuo = HasEntry "Client.cuo"
if ($haveClientCuo) { Say "    Client.cuo: found" }
else                { Say "    Client.cuo: not present - one will be generated" }

# ---------------------------------------------------------------------------
# 3. Link the data into .\data
# ---------------------------------------------------------------------------
# The client reads UO data from one directory and writes its own settings back
# into that same directory. Staging into .\data keeps those writes out of your
# UO folder.
#
# Links are tried hardest-first: a hard link costs no disk and needs no
# privileges, but only works on the same volume. A copy always works.
Step "Staging UO data into $dataDir"

if (Test-Path -LiteralPath $dataDir) { Remove-Item -LiteralPath $dataDir -Recurse -Force }
New-Item -ItemType Directory -Path $dataDir | Out-Null

$skip = @("models", "orion launcher", "oa", "desktop.ini", "thumbs.db",
          # Orion's own settings, not UO data. The client rewrites these in
          # place, so linking them would push this package's settings - and any
          # saved account - back into your UO folder. Start from defaults.
          "orion_options.cfg", "macros_debug.cuo", "uo_debug.cfg",
          "options_debug.cuo", "skills_debug.cuo", "gumps_debug.cuo")

$linked = 0; $copied = 0; $skipped = 0

foreach ($entry in $entries) {
    $name  = $entry.Name
    $lower = $name.ToLowerInvariant()

    if ($skip -contains $lower) { $skipped++; continue }
    if ($lower -like "*.bik" -or $lower -like "*.exe" -or
        $lower -like "*.dll" -or $lower -like "*.pdb") { $skipped++; continue }

    $src = $entry.FullName
    $dst = Join-Path $dataDir $name

    $done = $false
    if ($entry.PSIsContainer) {
        # A junction needs no administrator rights, unlike a directory symlink.
        try {
            New-Item -ItemType Junction -Path $dst -Target $src -ErrorAction Stop | Out-Null
            $linked++; $done = $true
        } catch { }
    } else {
        try {
            New-Item -ItemType HardLink -Path $dst -Target $src -ErrorAction Stop | Out-Null
            $linked++; $done = $true
        } catch { }
    }

    if (-not $done) {
        Copy-Item -LiteralPath $src -Destination $dst -Recurse -Force
        $copied++
    }
}

Say "    $linked linked, $copied copied, $skipped skipped as unused or not UO data"
Say ""
Say "    Not staged, because this client never reads them: Models\,"
Say "    'Orion Launcher'\, the .bik intro videos and the Windows .exe/.dll."
Say "    Music\ is staged but is only needed for in-game music."
if ($copied -gt 20) {
    Say ""
    Say "    Most files had to be copied rather than linked, which means this"
    Say "    package and your UO install are on different drives. Moving them"
    Say "    onto the same drive and re-running saves the disk space."
}

# ---------------------------------------------------------------------------
# 4. Client.cuo
# ---------------------------------------------------------------------------
# Client.cuo names the UO version and login encryption. It normally comes from
# the Orion Launcher, so a stock uo.com install has none.
if (-not $haveClientCuo) {
    Step "Generating Client.cuo (client 7.0.20.0)"
    $python = Get-Command python3 -ErrorAction SilentlyContinue
    if (-not $python) { $python = Get-Command python -ErrorAction SilentlyContinue }
    if ($python -and (Test-Path -LiteralPath (Join-Path $here "make_client_cuo.py"))) {
        & $python.Source (Join-Path $here "make_client_cuo.py") `
            -o (Join-Path $dataDir "Client.cuo") `
            --client-version CV_70180 --encryption ET_TFISH --version-text 7.0.20.0
        Say "    wrote $(Join-Path $dataDir 'Client.cuo')"
    } else {
        Say "    Python is not installed, so it could not be generated. If your"
        Say "    shard ships a Client.cuo, copy it into $dataDir and it will be"
        Say "    used instead. Otherwise install Python from https://python.org"
        Die "Client.cuo is required and could not be produced"
    }
}

# ---------------------------------------------------------------------------
# 5. Tell the client where the data is
# ---------------------------------------------------------------------------
Step "Writing uo_debug.cfg"
"CustomPath=$dataDir" | Set-Content -LiteralPath (Join-Path $here "uo_debug.cfg") -Encoding ASCII
Say "    CustomPath=$dataDir"

# ---------------------------------------------------------------------------
# 6. Play
# ---------------------------------------------------------------------------
$shardHost = "uo.jmaul.co.uk"; $shardPort = "2593"
if (Test-Path -LiteralPath (Join-Path $here "shard.conf")) {
    foreach ($line in Get-Content -LiteralPath (Join-Path $here "shard.conf")) {
        if ($line -match '^\s*SHARD_HOST=(.*)$') { $shardHost = $Matches[1].Trim() }
        if ($line -match '^\s*SHARD_PORT=(.*)$') { $shardPort = $Matches[1].Trim() }
    }
}

Step "Ready"
Say "    Play with:   $here\play-ignis.cmd"
Say "    Equivalent:  OrionUO.exe `"-login $shardHost,$shardPort`""
Say ""

if (-not $NoLaunch) {
    Say "Starting the client..."
    & (Join-Path $here "play-ignis.cmd")
}
