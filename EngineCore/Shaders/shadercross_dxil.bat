@echo off
setlocal

set SHADERCROSS_EXE=..\..\..\cmake-build-release-visual-studio\ThirdParty\SDL_shadercross\shadercross.exe
set OUTPUT_DIR=Compiled

if not exist %OUTPUT_DIR% (
    mkdir %OUTPUT_DIR%
)

echo Compiling HLSL shaders to DXIL...

for %%f in (*.vert.hlsl) do (
    echo   Compiling %%f...
    %SHADERCROSS_EXE% "%%f" -d DXIL -t vertex -o "%OUTPUT_DIR%\%%~nf.dxil"
    if errorlevel 1 (
        echo ERROR: Failed to compile %%f
        pause
    )
)

for %%f in (*.frag.hlsl) do (
    echo   Compiling %%f...
    %SHADERCROSS_EXE% "%%f" -d DXIL -t fragment -o "%OUTPUT_DIR%\%%~nf.dxil"
    if errorlevel 1 (
        echo ERROR: Failed to compile %%f
        pause
    )
)

for %%f in (*.comp.hlsl) do (
    echo   Compiling %%f...
    %SHADERCROSS_EXE% "%%f" -d DXIL -t compute -o "%OUTPUT_DIR%\%%~nf.dxil"
    if errorlevel 1 (
        echo ERROR: Failed to compile %%f
        pause
    )
)

echo Done.
