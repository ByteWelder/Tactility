# Runs tt.py from the folder of this script, on Windows.
# PowerShell passes the arguments to Python as they are, without parsing them again.
$script = Join-Path $PSScriptRoot "tt.py"
if (Get-Command py -ErrorAction SilentlyContinue) {
    & py -3 $script @args
} else {
    & python $script @args
}
exit $LASTEXITCODE
