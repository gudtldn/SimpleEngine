@echo off
echo Setting up Python virtual environment...

REM Check if Python is available
python --version >nul 2>&1
if %errorlevel% neq 0 (
    echo Error: Python is not found in PATH. Please install Python 3.
    exit /b 1
)

REM Create virtual environment in .venv directory if it doesn't exist
if not exist .\.venv (
    echo Creating virtual environment...
    python -m venv .venv
)

REM Activate the virtual environment and install dependencies
echo Installing dependencies from Scripts/requirements.txt...
call .\.venv\Scripts\activate.bat
pip install -r Scripts\requirements.txt

echo.
echo Setup complete. You can now configure the CMake project.
pause
