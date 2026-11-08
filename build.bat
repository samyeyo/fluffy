@echo off
setlocal

set BUILD_DIR=build
set DEBUG_DIR=build-debug
set BUILD_CONFIG=Release
set DO_INSTALL=0

if /I "%1"=="clean" (
    echo Cleaning build directories...
    if exist "%BUILD_DIR%\third_party\lucide_svg" (
        move "%BUILD_DIR%\third_party\lucide_svg" "%TEMP%\fluffy_lucide_svg_backup" >nul
    )
    if exist "%BUILD_DIR%" rmdir /S /Q "%BUILD_DIR%"
    if exist "%DEBUG_DIR%" rmdir /S /Q "%DEBUG_DIR%"
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
    cmake -S . -B "%BUILD_DIR%" -DCMAKE_CONFIGURATION_TYPES=Release -DFLUFFY_BUILD_EXAMPLES=ON
) else if /I "%1"=="clang-cl" (
    echo Configuring with clang-cl ^(Visual Studio ClangCL toolset^)...
    rem A build directory cannot switch compilers in place: CMake caches the
    rem generator and compiler.  Refuse early instead of failing on a stale cache.
    set "TOOLSET_MISMATCH="
    if exist "%BUILD_DIR%\CMakeCache.txt" (
        findstr /I /C:"clang-cl" "%BUILD_DIR%\CMakeCache.txt" >nul 2>&1
        if errorlevel 1 set "TOOLSET_MISMATCH=1"
    )
    if defined TOOLSET_MISMATCH (
        echo Error: "%BUILD_DIR%" was configured with a different compiler.
        echo Run "build.bat clean" first, then "build.bat clang-cl".
        exit /b 1
    )
    cmake -S . -B "%BUILD_DIR%" -T ClangCL -DCMAKE_CONFIGURATION_TYPES=Release -DFLUFFY_BUILD_EXAMPLES=ON
) else if /I "%1"=="debug" (
    echo Configuring Debug in %DEBUG_DIR%...
    cmake -S . -B "%DEBUG_DIR%" -DCMAKE_CONFIGURATION_TYPES=Debug -DFLUFFY_BUILD_EXAMPLES=ON
    set BUILD_DIR=%DEBUG_DIR%
    set BUILD_CONFIG=Debug
) else if /I "%1"=="install" (
    echo Configuring release for install...
    if defined PREFIX (
        cmake -S . -B "%BUILD_DIR%" -DCMAKE_CONFIGURATION_TYPES=Release -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX="%PREFIX%"
    ) else (
        cmake -S . -B "%BUILD_DIR%" -DCMAKE_CONFIGURATION_TYPES=Release -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX="%LOCALAPPDATA%\Fluffy"
    )
    set DO_INSTALL=1
) else (
    echo Configuring...
    cmake -S . -B "%BUILD_DIR%" -DCMAKE_CONFIGURATION_TYPES=Release
)

echo.
echo Building...
if "%DO_INSTALL%"=="1" (
    cmake --build "%BUILD_DIR%" --target fluffy --config %BUILD_CONFIG% --parallel
) else (
    cmake --build "%BUILD_DIR%" --config %BUILD_CONFIG% --parallel
)

echo.
if "%DO_INSTALL%"=="1" (
    if defined PREFIX (
        echo Installing Fluffy to "%PREFIX%"...
        cmake --install "%BUILD_DIR%" --config %BUILD_CONFIG%
    ) else (
        echo Installing Fluffy to "%LOCALAPPDATA%\Fluffy"...
        cmake --install "%BUILD_DIR%" --config %BUILD_CONFIG%
    )
    echo Installed.
) else (
    echo Build complete.
)

endlocal
