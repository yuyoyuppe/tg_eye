param (
    [string]$rootDirectory
)

# Check if the root directory parameter is provided
if (-not $rootDirectory) {
    Write-Host "Usage: .\<name>.ps1 -rootDirectory <path>"
    exit 1
}

# Define the function to update the RuntimeLibrary elements in vcxproj files
function Update-RuntimeLibrary {
    param (
        [string]$filePath
    )

    # Read the content of the file
    $content = Get-Content -Path $filePath

    # Replace MultiThreadedDLL with MultiThreaded
    $content = $content -replace '<RuntimeLibrary>MultiThreadedDLL</RuntimeLibrary>', '<RuntimeLibrary>MultiThreaded</RuntimeLibrary>'
    
    # Replace MultiThreadedDebugDLL with MultiThreadedDebug
    $content = $content -replace '<RuntimeLibrary>MultiThreadedDebugDLL</RuntimeLibrary>', '<RuntimeLibrary>MultiThreadedDebug</RuntimeLibrary>'

    # Write the updated content back to the file
    Set-Content -Path $filePath -Value $content
}

# Get all .vcxproj files recursively from the root directory
$vcxprojFiles = Get-ChildItem -Path $rootDirectory -Recurse -Filter *.vcxproj

# Loop through each vcxproj file and update it
foreach ($file in $vcxprojFiles) {
    Update-RuntimeLibrary -filePath $file.FullName
    Write-Host "Updated: $($file.FullName)"
}
