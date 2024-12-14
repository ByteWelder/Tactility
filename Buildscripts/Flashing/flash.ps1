param(
    $port
)

if ((Get-Command "esptool.py" -ErrorAction SilentlyContinue) -eq $null)
{
    Write-Host "Unable to find esptool.py in your path. Make sure you have Python installed and on your path. Then run `pip install esptool`."
}


# Create flash command based on partitions
$json = Get-Content .\build\flasher_args.json -Raw | ConvertFrom-Json
$jsonClean = $json.flash_files -replace '[\{\}\@\;]', ''
$jsonClean = $jsonClean -replace '[\=]', ' '

cd Binaries
$command = "esptool.py --connect-attemps 10 --port $port -b 460800 write_flash $jsonClean"
Invoke-Expression $command
cd ..

