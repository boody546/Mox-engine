@echo off
:: ═══════════════════════════════════════════════════════════════════
::  Mox HUB — Compile Engine & Embed (HUB/compile_engine.bat)
:: ═══════════════════════════════════════════════════════════════════

set "HUB_DIR=%~dp0"
set "ROOT=%HUB_DIR%.."
set "ENGINE_DIR=%ROOT%\Engine"
set "TARGET_BUILD=%ENGINE_DIR%\build"

echo.
echo  ════════════════════════════════════════════════
echo     Mox Engine Compiler ^& Package Pipeline
echo  ════════════════════════════════════════════════
echo.

:: 1. Find g++ compiler (from w64devkit or system PATH)
set "GXX="
if exist "C:\Users\walaa\w64devkit\bin\g++.exe" (
    set "PATH=C:\Users\walaa\w64devkit\bin;%PATH%"
    set "GXX=C:\Users\walaa\w64devkit\bin\g++.exe"
) else (
    where g++ >nul 2>&1
    if %ERRORLEVEL% equ 0 (
        set "GXX=g++"
    )
)

if "%GXX%"=="" (
    echo  ❌ g++ compiler not found! Please install w64devkit or add g++ to PATH.
    pause
    exit /b 1
)

echo  [1/5] Using compiler: %GXX%

:: 2. Ensure Engine/build directory exists
echo  [2/5] Preparing Engine\build directory...
if not exist "%TARGET_BUILD%" mkdir "%TARGET_BUILD%"

:: 3. Locate source directory (Engine/src or root src)
if exist "%ENGINE_DIR%\src" (
    set "SRC_DIR=%ENGINE_DIR%\src"
) else if exist "%ROOT%\src" (
    set "SRC_DIR=%ROOT%\src"
) else (
    echo  ❌ Source directory not found!
    pause
    exit /b 1
)

:: Locate third_party and SDL2
if exist "%ENGINE_DIR%\third_party" (
    set "THIRD_PARTY=%ENGINE_DIR%\third_party"
) else (
    set "THIRD_PARTY=%ROOT%\third_party"
)
set "SDL2_DIR=%THIRD_PARTY%\SDL2-2.30.2\x86_64-w64-mingw32"

echo  [3/5] Compiling C++ source files from %SRC_DIR%...
set CXXFLAGS=-std=c++17 -Wall -Wextra -O2 -I "%SRC_DIR%" -I "%SDL2_DIR%\include\SDL2" -I "%THIRD_PARTY%\stb" -I "%THIRD_PARTY%\nlohmann"

set COMPILE_OK=1

"%GXX%" %CXXFLAGS% -c "%SRC_DIR%\core\Engine.cpp"         -o "%TARGET_BUILD%\Engine.o"         || set COMPILE_OK=0
"%GXX%" %CXXFLAGS% -c "%SRC_DIR%\input\InputManager.cpp"   -o "%TARGET_BUILD%\InputManager.o"   || set COMPILE_OK=0
"%GXX%" %CXXFLAGS% -c "%SRC_DIR%\rendering\Renderer2D.cpp" -o "%TARGET_BUILD%\Renderer2D.o"     || set COMPILE_OK=0
"%GXX%" %CXXFLAGS% -c "%SRC_DIR%\rendering\Texture.cpp"    -o "%TARGET_BUILD%\Texture.o"        || set COMPILE_OK=0
"%GXX%" %CXXFLAGS% -c "%SRC_DIR%\rendering\Camera2D.cpp"   -o "%TARGET_BUILD%\Camera2D.o"       || set COMPILE_OK=0
"%GXX%" %CXXFLAGS% -c "%SRC_DIR%\rendering\SpriteAnimation.cpp" -o "%TARGET_BUILD%\SpriteAnimation.o" || set COMPILE_OK=0
"%GXX%" %CXXFLAGS% -c "%SRC_DIR%\rendering\TileMap.cpp"    -o "%TARGET_BUILD%\TileMap.o"        || set COMPILE_OK=0
"%GXX%" %CXXFLAGS% -c "%SRC_DIR%\rendering\ParticleSystem.cpp" -o "%TARGET_BUILD%\ParticleSystem.o" || set COMPILE_OK=0
"%GXX%" %CXXFLAGS% -c "%SRC_DIR%\physics\PhysicsWorld.cpp" -o "%TARGET_BUILD%\PhysicsWorld.o"   || set COMPILE_OK=0
"%GXX%" %CXXFLAGS% -c "%SRC_DIR%\scene\Node.cpp"           -o "%TARGET_BUILD%\Node.o"           || set COMPILE_OK=0
"%GXX%" %CXXFLAGS% -c "%SRC_DIR%\scene\SceneTree.cpp"      -o "%TARGET_BUILD%\SceneTree.o"      || set COMPILE_OK=0
"%GXX%" %CXXFLAGS% -c "%SRC_DIR%\scene\ScriptNode.cpp"     -o "%TARGET_BUILD%\ScriptNode.o"     || set COMPILE_OK=0
"%GXX%" %CXXFLAGS% -c "%SRC_DIR%\scripting\Lexer.cpp"      -o "%TARGET_BUILD%\Lexer.o"          || set COMPILE_OK=0
"%GXX%" %CXXFLAGS% -c "%SRC_DIR%\scripting\Parser.cpp"     -o "%TARGET_BUILD%\Parser.o"         || set COMPILE_OK=0
"%GXX%" %CXXFLAGS% -c "%SRC_DIR%\scripting\Interpreter.cpp" -o "%TARGET_BUILD%\Interpreter.o"   || set COMPILE_OK=0
"%GXX%" %CXXFLAGS% -c "%SRC_DIR%\scripting\ScriptBinding.cpp" -o "%TARGET_BUILD%\ScriptBinding.o" || set COMPILE_OK=0
"%GXX%" %CXXFLAGS% -c "%SRC_DIR%\audio\AudioManager.cpp"   -o "%TARGET_BUILD%\AudioManager.o"   || set COMPILE_OK=0
"%GXX%" %CXXFLAGS% -c "%SRC_DIR%\assets\ResourceManager.cpp" -o "%TARGET_BUILD%\ResourceManager.o" || set COMPILE_OK=0
"%GXX%" %CXXFLAGS% -c "%SRC_DIR%\main.cpp"                 -o "%TARGET_BUILD%\main.o"           || set COMPILE_OK=0

if "%COMPILE_OK%"=="0" (
    echo.
    echo  ❌ Compilation failed! Check error output above.
    pause
    exit /b 1
)

:: 4. Link executable directly to Engine/build/MoxEngine.exe
echo  [4/5] Linking Engine\build\MoxEngine.exe...
"%GXX%" "%TARGET_BUILD%\*.o" -o "%TARGET_BUILD%\MoxEngine.exe" ^
    -L "%SDL2_DIR%\lib" ^
    -lmingw32 -lSDL2main -lSDL2 ^
    -lm -ldinput8 -ldxguid -ldxerr8 -luser32 -lgdi32 ^
    -lwinmm -limm32 -lole32 -loleaut32 -lshell32 ^
    -lsetupapi -lversion -luuid -lhid -lopengl32

if %ERRORLEVEL% neq 0 (
    echo.
    echo  ❌ Linking failed!
    pause
    exit /b 1
)

:: Copy SDL2.dll and scripts folder into Engine/build/
if exist "%SDL2_DIR%\bin\SDL2.dll" (
    copy /Y "%SDL2_DIR%\bin\SDL2.dll" "%TARGET_BUILD%\" >nul 2>&1
)

if exist "%ENGINE_DIR%\scripts" (
    xcopy /E /Y /I /Q "%ENGINE_DIR%\scripts" "%TARGET_BUILD%\scripts\" >nul 2>&1
) else if exist "%ROOT%\scripts" (
    xcopy /E /Y /I /Q "%ROOT%\scripts" "%TARGET_BUILD%\scripts\" >nul 2>&1
)

echo  ✓ MoxEngine.exe created successfully in Engine\build\

:: 5. Execute embed_engine.py automatically right after compilation
echo  [5/5] Executing HUB\embed_engine.py...
python "%HUB_DIR%embed_engine.py"

if %ERRORLEVEL% neq 0 (
    echo  ❌ HUB\embed_engine.py execution failed!
    pause
    exit /b 1
)

echo.
echo  ════════════════════════════════════════════════
echo     SUCCESS! MoxEngine compiled ^& embedded!
echo  ════════════════════════════════════════════════
