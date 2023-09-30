$file1Content = Get-Content -Raw -Path "out-1"
$file2Content = Get-Content -Raw -Path "out.txt"

# Compare the contents of the two files, ignoring white spaces
$comparisonResult = Compare-Object -ReferenceObject $file1Content -DifferenceObject $file2Content -SyncWindow 1000000 -IncludeEqual -PassThru

if ($comparisonResult -eq $null) {
    Write-Host "The files have the same content, regardless of white spaces."
} else {
    Write-Host "The files are different."
}