@echo off
rem Runs tt.py from the directory of this script, on Windows
where py >nul 2>nul
if %ERRORLEVEL% equ 0 (
    py -3 "%~dp0tt.py" %*
) else (
    python "%~dp0tt.py" %*
)
exit /b %ERRORLEVEL%
