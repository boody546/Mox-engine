@echo off
:: ═══════════════════════════════════════════════════════════════════
::  Mox HUB — Build Engine + Embed into HUB (One-Click Export)
::  Run this ONCE to:
::    1. Compile MoxEngine from source (C++ / MinGW)
::    2. Embed the binary into HUB/engine_payload.py
::    3. Launch Mox HUB (ready to Install with single click)
:: ═══════════════════════════════════════════════════════════════════

set "ROOT=%~dp0.."
set "HUB=%~dp0"
set "DK=C:\Users\walaa\w64devkit"
set "PATH=%DK%\bin;%PATH%"
set "SDL2_DIR=%ROOT%\third_party\SDL2-2.30.2\x86_64-w64-mingw32"
set "BUILD_DIR=%ROOT%\build"

echo.
echo  ╔══════════════════════════════════════════════╗
echo  ║   Mox HUB — Build ^& Embed Engine v1.0      ║
echo  ╚══════════════════════════════════════════════╝
echo.

:: ─── Step 1: Kill any locked process ──────────────────────────────
echo  [1/5] Cleaning old processes...
taskkill /F /IM Nova2D.exe /IM MoxEngine.exe /IM g++.exe 2>nul

:: ─── Step 2: Create build directory ───────────────────────────────
echo  [2/5] Preparing build directory...
if not exist "%BUILD_DIR%" mkdir "%BUILD_DIR%"

:: ─── Step 3: Compile all source files ─────────────────────────────
echo  [3/5] Compiling Mox Engine source (C++17)...
set CXXFLAGS=-std=c++17 -Wall -Wextra -O2 -I "%ROOT%\src" -I "%SDL2_DIR%\include\SDL2" -I "%ROOT%\third_party\stb" -I "%ROOT%\third_party\nlohmann"

set COMPILE_OK=1

g++ %CXXFLAGS% -c "%ROOT%\src\core\Engine.cpp"         -o "%BUILD_DIR%\Engine.o"         || set COMPILE_OK=0
g++ %CXXFLAGS% -c "%ROOT%\src\input\InputManager.cpp"   -o "%BUILD_DIR%\InputManager.o"   || set COMPILE_OK=0
g++ %CXXFLAGS% -c "%ROOT%\src\rendering\Renderer2D.cpp" -o "%BUILD_DIR%\Renderer2D.o"     || set COMPILE_OK=0
g++ %CXXFLAGS% -c "%ROOT%\src\rendering\Texture.cpp"    -o "%BUILD_DIR%\Texture.o"        || set COMPILE_OK=0
g++ %CXXFLAGS% -c "%ROOT%\src\rendering\Camera2D.cpp"   -o "%BUILD_DIR%\Camera2D.o"       || set COMPILE_OK=0
g++ %CXXFLAGS% -c "%ROOT%\src\rendering\SpriteAnimation.cpp" -o "%BUILD_DIR%\SpriteAnimation.o" || set COMPILE_OK=0
g++ %CXXFLAGS% -c "%ROOT%\src\rendering\TileMap.cpp"    -o "%BUILD_DIR%\TileMap.o"        || set COMPILE_OK=0
g++ %CXXFLAGS% -c "%ROOT%\src\rendering\ParticleSystem.cpp" -o "%BUILD_DIR%\ParticleSystem.o" || set COMPILE_OK=0
g++ %CXXFLAGS% -c "%ROOT%\src\physics\PhysicsWorld.cpp" -o "%BUILD_DIR%\PhysicsWorld.o"   || set COMPILE_OK=0
g++ %CXXFLAGS% -c "%ROOT%\src\scene\Node.cpp"           -o "%BUILD_DIR%\Node.o"           || set COMPILE_OK=0
g++ %CXXFLAGS% -c "%ROOT%\src\scene\SceneTree.cpp"      -o "%BUILD_DIR%\SceneTree.o"      || set COMPILE_OK=0
g++ %CXXFLAGS% -c "%ROOT%\src\scene\ScriptNode.cpp"     -o "%BUILD_DIR%\ScriptNode.o"     || set COMPILE_OK=0
g++ %CXXFLAGS% -c "%ROOT%\src\scripting\Lexer.cpp"      -o "%BUILD_DIR%\Lexer.o"          || set COMPILE_OK=0
g++ %CXXFLAGS% -c "%ROOT%\src\scripting\Parser.cpp"     -o "%BUILD_DIR%\Parser.o"         || set COMPILE_OK=0
g++ %CXXFLAGS% -c "%ROOT%\src\scripting\Interpreter.cpp" -o "%BUILD_DIR%\Interpreter.o"   || set COMPILE_OK=0
g++ %CXXFLAGS% -c "%ROOT%\src\scripting\ScriptBinding.cpp" -o "%BUILD_DIR%\ScriptBinding.o" || set COMPILE_OK=0
g++ %CXXFLAGS% -c "%ROOT%\src\audio\AudioManager.cpp"   -o "%BUILD_DIR%\AudioManager.o"   || set COMPILE_OK=0
g++ %CXXFLAGS% -c "%ROOT%\src\assets\ResourceManager.cpp" -o "%BUILD_DIR%\ResourceManager.o" || set COMPILE_OK=0
g++ %CXXFLAGS% -c "%ROOT%\src\main.cpp"                 -o "%BUILD_DIR%\main.o"           || set COMPILE_OK=0

if "%COMPILE_OK%"=="0" (
    echo.
    echo  ❌ Compilation failed! Check errors above.
    pause
    exit /b 1
)

:: ─── Step 4: Link executable ───────────────────────────────────────
echo  [4/5] Linking MoxEngine.exe...
g++ "%BUILD_DIR%\*.o" -o "%BUILD_DIR%\MoxEngine.exe" ^
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

:: Copy SDL2.dll into build dir
copy /Y "%SDL2_DIR%\bin\SDL2.dll" "%BUILD_DIR%\" >nul 2>&1

:: Copy scripts folder into build dir if present
if exist "%ROOT%\scripts" (
    xcopy /E /Y /I /Q "%ROOT%\scripts" "%BUILD_DIR%\scripts\" >nul 2>&1
)

echo  ✓  MoxEngine.exe built successfully!

:: ─── Step 5: Embed into HUB payload ───────────────────────────────
echo  [5/5] Embedding engine into HUB/engine_payload.py...
python "%HUB%embed_engine.py"

if %ERRORLEVEL% neq 0 (
    echo.
    echo  ❌ Embedding failed! Make sure Python is installed.
    pause
    exit /b 1
)

echo.
echo  ╔══════════════════════════════════════════════╗
echo  ║   ✅  Done! Launching Mox HUB...            ║
echo  ╚══════════════════════════════════════════════╝
echo.

start "" python "%HUB%main.py"
