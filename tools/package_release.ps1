$zipName = "ArbSim_Release.zip"
if (Test-Path $zipName) { Remove-Item $zipName }

# Create a temporary folder for the release
$releaseDir = "ArbSim_Release"
if (Test-Path $releaseDir) { Remove-Item $releaseDir -Recurse }
New-Item -ItemType Directory -Path $releaseDir

# Copy Executable (Update path to look in build/Release or similar if needed, 
# but for now assume user builds it or we bundle differently. 
# Actually, let's just keep it simple or deprecate it if we rely on CMake now.)
# ...
