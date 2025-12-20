$exclude = @(
    "x64",
    ".vs",
    ".git",
    "*.zip",
    "__pycache__"
)

$zipName = "ArbSim_Submission.zip"

if (Test-Path $zipName) { Remove-Item $zipName }

$files = Get-ChildItem -Path * -Exclude $exclude
Compress-Archive -Path $files.FullName -DestinationPath $zipName -Force

Write-Host "Project packaged to $zipName"
