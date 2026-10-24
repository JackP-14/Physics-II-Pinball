param(
	# Zip name required by the assignment: Lastname1Name1_Lastname2Name2_etc
	[Parameter(Mandatory = $true)][string]$ZipName,
	[string]$MSBuild = "C:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\MSBuild.exe"
)
# Builds the game in Release|Win32 and packs the executable, the assets it uses and README.txt into <ZipName>.zip
$ErrorActionPreference = "Stop"
$root = Split-Path $PSScriptRoot -Parent
$src = Join-Path $root "src"
$stage = Join-Path $src "Build\Package\$ZipName"
$zip = Join-Path $src "Build\Package\$ZipName.zip"

& $MSBuild (Join-Path $src "PhysicsGame.sln") /t:Build /p:Configuration=Release /p:Platform=Win32 /m /v:minimal /nologo
if ($LASTEXITCODE -ne 0) { throw "Release build failed" }

if (Test-Path $stage) { Remove-Item $stage -Recurse -Force }
New-Item -ItemType Directory -Force (Join-Path $stage "Assets\Audio\Fx") | Out-Null

Copy-Item (Join-Path $src "Build\PhysicsGame\Release\Win32\PhysicsGame.exe") (Join-Path $stage "HalloweenPinball.exe")

# Only the assets the game loads
$assets = @("table_background.png", "bumpers.png", "shooter.png", "bonus.wav")
foreach ($a in $assets) { Copy-Item (Join-Path $src "Assets\$a") (Join-Path $stage "Assets\$a") }
Copy-Item (Join-Path $src "Assets\Audio\Fx\*.wav") (Join-Path $stage "Assets\Audio\Fx")

Copy-Item (Join-Path $root "README.txt") (Join-Path $stage "README.txt")

if (Test-Path $zip) { Remove-Item $zip -Force }
Compress-Archive -Path (Join-Path $stage "*") -DestinationPath $zip
Write-Output "Package: $zip"
