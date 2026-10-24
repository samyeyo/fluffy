@echo off
setlocal

set BUILD_DIR=build
set DO_INSTALL=0

if /I "%1"=="clean" (
    echo Cleaning build directory...
    if exist "%BUILD_DIR%\third_party\lucide_svg" (
        move "%BUILD_DIR%\third_party\lucide_svg" "%TEMP%\fluffy_lucide_svg_backup" >nul
    )
    if exist "%BUILD_DIR%" rmdir /S /Q "%BUILD_DIR%"
    if exist "%BUILD_DIR%_release" rmdir /S /Q "%BUILD_DIR%_release"
    if exist "%TEMP%\fluffy_lucide_svg_backup" (
        mkdir "%BUILD_DIR%\third_party"
        move "%TEMP%\fluffy_lucide_svg_backup" "%BUILD_DIR%\third_party\lucide_svg" >nul
    )
    echo Done.
    exit /b 0
)

if /I "%1"=="examples" (
    echo Configuring with examples...
    cmake -S . -B "%BUILD_DIR%" -DFLUFFY_BUILD_EXAMPLES=ON
) else if /I "%1"=="debug" (
    echo Configuring Debug...
    cmake -S . -B "%BUILD_DIR%" -DCMAKE_BUILD_TYPE=Debug
) else if /I "%1"=="install" (
    echo Configuring release for install...
    if defined PREFIX (
        cmake -S . -B "%BUILD_DIR%" -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX="%PREFIX%"
    ) else (
        cmake -S . -B "%BUILD_DIR%" -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX="%LOCALAPPDATA%\Fluffy"
    )
    set DO_INSTALL=1
) else (
    echo Configuring...
    cmake -S . -B "%BUILD_DIR%"
)

echo.
echo Building...
if "%DO_INSTALL%"=="1" (
    cmake --build "%BUILD_DIR%" --target fluffy --config Release --parallel
) else (
    cmake --build "%BUILD_DIR%" --parallel
)

echo.
if "%DO_INSTALL%"=="1" (
    if defined PREFIX (
        echo Installing Fluffy to "%PREFIX%"...
        cmake --install "%BUILD_DIR%" --config Release
    ) else (
        echo Installing Fluffy to "%LOCALAPPDATA%\Fluffy"...
        cmake --install "%BUILD_DIR%" --config Release
    )
    echo Installed.
) else (
    echo Build complete.
)

endlocal
