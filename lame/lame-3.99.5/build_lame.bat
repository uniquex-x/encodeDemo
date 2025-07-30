@echo off
setlocal EnableDelayedExpansion

REM LAME Windows 构建脚本
REM 支持: Windows (x86, x86_64), Android交叉编译

title LAME 构建脚本

echo ======================================
echo LAME Windows 构建脚本
echo ======================================
echo.

REM 默认配置
set TARGET_PLATFORM=windows
set TARGET_ARCH=x86_64
set BUILD_TYPE=Release
set SHARED_LIBS=ON
set WITH_FRONTEND=ON
set WITH_DECODER=ON
set CLEAN_BUILD=0

REM 路径配置
set SCRIPT_DIR=%~dp0
set SOURCE_DIR=%SCRIPT_DIR%
set BUILD_ROOT=%SCRIPT_DIR%build
set INSTALL_PREFIX=%BUILD_ROOT%\install

REM Android NDK配置
set ANDROID_NDK_PATH=
set ANDROID_API_LEVEL=21

REM 解析命令行参数
:parse_args
if "%~1"=="" goto validate_args
if "%~1"=="-p" (
    set TARGET_PLATFORM=%~2
    shift
    shift
    goto parse_args
)
if "%~1"=="--platform" (
    set TARGET_PLATFORM=%~2
    shift
    shift
    goto parse_args
)
if "%~1"=="-a" (
    set TARGET_ARCH=%~2
    shift
    shift
    goto parse_args
)
if "%~1"=="--arch" (
    set TARGET_ARCH=%~2
    shift
    shift
    goto parse_args
)
if "%~1"=="-t" (
    set BUILD_TYPE=%~2
    shift
    shift
    goto parse_args
)
if "%~1"=="--build-type" (
    set BUILD_TYPE=%~2
    shift
    shift
    goto parse_args
)
if "%~1"=="-s" (
    set SHARED_LIBS=%~2
    shift
    shift
    goto parse_args
)
if "%~1"=="--shared" (
    set SHARED_LIBS=%~2
    shift
    shift
    goto parse_args
)
if "%~1"=="-f" (
    set WITH_FRONTEND=%~2
    shift
    shift
    goto parse_args
)
if "%~1"=="--frontend" (
    set WITH_FRONTEND=%~2
    shift
    shift
    goto parse_args
)
if "%~1"=="-d" (
    set WITH_DECODER=%~2
    shift
    shift
    goto parse_args
)
if "%~1"=="--decoder" (
    set WITH_DECODER=%~2
    shift
    shift
    goto parse_args
)
if "%~1"=="-n" (
    set ANDROID_NDK_PATH=%~2
    shift
    shift
    goto parse_args
)
if "%~1"=="--ndk-path" (
    set ANDROID_NDK_PATH=%~2
    shift
    shift
    goto parse_args
)
if "%~1"=="-c" (
    set CLEAN_BUILD=1
    shift
    goto parse_args
)
if "%~1"=="--clean" (
    set CLEAN_BUILD=1
    shift
    goto parse_args
)
if "%~1"=="-h" goto show_help
if "%~1"=="--help" goto show_help

echo 错误: 未知参数 %~1
goto show_help

:show_help
echo LAME Windows 构建脚本
echo.
echo 用法: %~nx0 [选项]
echo.
echo 选项:
echo   -p, --platform PLATFORM    目标平台 (windows^|android)
echo   -a, --arch ARCH            目标架构 (x86^|x86_64^|armv7^|armv8)
echo   -t, --build-type TYPE       构建类型 (Debug^|Release^|MinSizeRel^|RelWithDebInfo)
echo   -s, --shared               构建共享库 (ON^|OFF, 默认: ON)
echo   -f, --frontend             构建前端工具 (ON^|OFF, 默认: ON)
echo   -d, --decoder              包含MP3解码器 (ON^|OFF, 默认: ON)
echo   -n, --ndk-path PATH        Android NDK路径 (Android构建需要)
echo   -c, --clean                清理构建目录
echo   -h, --help                 显示此帮助信息
echo.
echo 示例:
echo   # Windows x86_64构建
echo   %~nx0
echo.
echo   # Windows x86构建
echo   %~nx0 --arch x86
echo.
echo   # Android armv8构建
echo   %~nx0 --platform android --arch armv8 --ndk-path C:\Android\ndk
echo.
echo   # 构建静态库
echo   %~nx0 --shared OFF
goto end_script

:validate_args
REM 验证平台
if "%TARGET_PLATFORM%"=="windows" goto platform_ok
if "%TARGET_PLATFORM%"=="android" goto platform_ok
echo 错误: 不支持的平台 %TARGET_PLATFORM%
goto end_script
:platform_ok

REM 验证架构
if "%TARGET_ARCH%"=="x86" goto arch_ok
if "%TARGET_ARCH%"=="x86_64" goto arch_ok
if "%TARGET_ARCH%"=="armv7" goto arch_ok
if "%TARGET_ARCH%"=="armv8" goto arch_ok
echo 错误: 不支持的架构 %TARGET_ARCH%
goto end_script
:arch_ok

REM Android构建需要NDK
if "%TARGET_PLATFORM%"=="android" (
    if "%ANDROID_NDK_PATH%"=="" (
        echo 错误: Android构建需要指定NDK路径
        goto end_script
    )
    if not exist "%ANDROID_NDK_PATH%" (
        echo 错误: NDK路径不存在: %ANDROID_NDK_PATH%
        goto end_script
    )
)

REM 显示构建信息
echo [信息] 构建配置:
echo   目标平台: %TARGET_PLATFORM%
echo   目标架构: %TARGET_ARCH%
echo   构建类型: %BUILD_TYPE%
echo   共享库: %SHARED_LIBS%
echo   前端工具: %WITH_FRONTEND%
echo   MP3解码器: %WITH_DECODER%
if "%TARGET_PLATFORM%"=="android" (
    echo   Android NDK: %ANDROID_NDK_PATH%
    echo   Android API: %ANDROID_API_LEVEL%
)
echo.

REM 检查CMake
where cmake >nul 2>&1
if errorlevel 1 (
    echo 错误: 未找到CMake，请安装CMake并添加到PATH
    goto end_script
)

REM 设置构建目录
set BUILD_DIR=%BUILD_ROOT%\%TARGET_PLATFORM%-%TARGET_ARCH%

REM 清理构建目录
if %CLEAN_BUILD%==1 (
    if exist "%BUILD_DIR%" (
        echo [信息] 清理构建目录: %BUILD_DIR%
        rmdir /s /q "%BUILD_DIR%"
    )
)

REM 创建构建目录
if not exist "%BUILD_DIR%" mkdir "%BUILD_DIR%"
echo [信息] 构建目录: %BUILD_DIR%

REM 设置CMake参数
set CMAKE_ARGS=-DCMAKE_BUILD_TYPE=%BUILD_TYPE%
set CMAKE_ARGS=%CMAKE_ARGS% -DBUILD_SHARED_LIBS=%SHARED_LIBS%
set CMAKE_ARGS=%CMAKE_ARGS% -DWITH_FRONTEND=%WITH_FRONTEND%
set CMAKE_ARGS=%CMAKE_ARGS% -DWITH_DECODER=%WITH_DECODER%
set CMAKE_ARGS=%CMAKE_ARGS% -DCMAKE_INSTALL_PREFIX=%INSTALL_PREFIX%

REM 平台特定配置
if "%TARGET_PLATFORM%"=="windows" (
    call :setup_windows_toolchain
) else if "%TARGET_PLATFORM%"=="android" (
    call :setup_android_toolchain
)

REM 配置项目
echo [信息] 配置项目...
cd /d "%BUILD_DIR%"

echo [信息] CMake命令: cmake %CMAKE_ARGS% "%SOURCE_DIR%"
cmake %CMAKE_ARGS% "%SOURCE_DIR%"
if errorlevel 1 (
    echo 错误: 配置失败
    goto end_script
)
echo [成功] 配置完成

REM 构建项目
echo [信息] 构建项目...
cmake --build . --config %BUILD_TYPE% --parallel
if errorlevel 1 (
    echo 错误: 构建失败
    goto end_script
)
echo [成功] 构建完成

REM 安装项目
echo [信息] 安装项目...
cmake --install . --config %BUILD_TYPE%
if errorlevel 1 (
    echo 错误: 安装失败
    goto end_script
)
echo [成功] 安装完成: %INSTALL_PREFIX%

echo.
echo ======================================
echo LAME构建完成！
echo 构建输出位于: %BUILD_DIR%
echo 安装文件位于: %INSTALL_PREFIX%
echo ======================================
goto end_script

:setup_windows_toolchain
echo [信息] 设置Windows工具链 (%TARGET_ARCH%)

if "%TARGET_ARCH%"=="x86_64" (
    REM 使用默认的x64工具链
    set CMAKE_ARGS=%CMAKE_ARGS% -A x64
) else if "%TARGET_ARCH%"=="x86" (
    REM 使用Win32工具链
    set CMAKE_ARGS=%CMAKE_ARGS% -A Win32
)

REM 检查Visual Studio
where cl >nul 2>&1
if errorlevel 1 (
    echo [警告] 未找到Visual Studio编译器，尝试使用MinGW
    if "%TARGET_ARCH%"=="x86_64" (
        set CMAKE_ARGS=%CMAKE_ARGS% -G "MinGW Makefiles"
        set CMAKE_ARGS=%CMAKE_ARGS% -DCMAKE_C_COMPILER=gcc
        set CMAKE_ARGS=%CMAKE_ARGS% -DCMAKE_CXX_COMPILER=g++
    )
) else (
    echo [信息] 使用Visual Studio编译器
    set CMAKE_ARGS=%CMAKE_ARGS% -G "Visual Studio 16 2019"
)
goto :eof

:setup_android_toolchain
echo [信息] 设置Android工具链 (%TARGET_ARCH%)

REM 设置Android ABI映射
if "%TARGET_ARCH%"=="armv7" (
    set ANDROID_ABI=armeabi-v7a
) else if "%TARGET_ARCH%"=="armv8" (
    set ANDROID_ABI=arm64-v8a
) else if "%TARGET_ARCH%"=="x86" (
    set ANDROID_ABI=x86
) else if "%TARGET_ARCH%"=="x86_64" (
    set ANDROID_ABI=x86_64
) else (
    echo 错误: 不支持的Android架构: %TARGET_ARCH%
    goto end_script
)

set CMAKE_ARGS=%CMAKE_ARGS% -DCMAKE_SYSTEM_NAME=Android
set CMAKE_ARGS=%CMAKE_ARGS% -DCMAKE_ANDROID_NDK=%ANDROID_NDK_PATH%
set CMAKE_ARGS=%CMAKE_ARGS% -DCMAKE_ANDROID_ARCH_ABI=%ANDROID_ABI%
set CMAKE_ARGS=%CMAKE_ARGS% -DCMAKE_ANDROID_API=%ANDROID_API_LEVEL%

REM 使用NDK工具链文件
set TOOLCHAIN_FILE=%ANDROID_NDK_PATH%\build\cmake\android.toolchain.cmake
if exist "%TOOLCHAIN_FILE%" (
    set CMAKE_ARGS=%CMAKE_ARGS% -DCMAKE_TOOLCHAIN_FILE="%TOOLCHAIN_FILE%"
) else (
    echo 错误: 找不到Android工具链文件: %TOOLCHAIN_FILE%
    goto end_script
)

echo [信息] Android配置:
echo   NDK: %ANDROID_NDK_PATH%
echo   ABI: %ANDROID_ABI%
echo   API Level: %ANDROID_API_LEVEL%
goto :eof

:end_script
cd /d "%SCRIPT_DIR%"
pause 