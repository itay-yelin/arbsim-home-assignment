$zipName = "ArbSim_Release.zip"
if (Test-Path $zipName) { Remove-Item $zipName }

# Create a temporary folder for the release
$releaseDir = "ArbSim_Release"
if (Test-Path $releaseDir) { Remove-Item $releaseDir -Recurse }
New-Item -ItemType Directory -Path $releaseDir

# Copy Executable
Copy-Item "dist/ArbSimDashboard.exe" -Destination $releaseDir

# Copy Config
Copy-Item "config.cfg" -Destination $releaseDir

# Copy Data (Create folder and copy specific files or all?)
$dataDir = Join-Path $releaseDir "data"
New-Item -ItemType Directory -Path $dataDir
Copy-Item "data/*.csv" -Destination $dataDir

# Zip it
Compress-Archive -Path "$releaseDir/*" -DestinationPath $zipName -Force

# Cleanup
Remove-Item $releaseDir -Recurse

Write-Host "Release packaged to $zipName"
