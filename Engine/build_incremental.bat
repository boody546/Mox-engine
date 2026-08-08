@echo off
set "DK=C:\Users\walaa\w64devkit"
set "PATH=%DK%\bin;%PATH%"

set "ENGINE_ROOT=%~dp0"
cd /d "%ENGINE_ROOT%"

if exist "third_party\SDL2-2.30.2" (
    set "SDL2_DIR=third_party\SDL2-2.30.2\x86_64-w64-mingw32"
    set "SRC_DIR=src"
    set "THIRD_PARTY=third_party"
) else (
    set "SDL2_DIR=..\third_party\SDL2-2.30.2\x86_64-w64-mingw32"
    set "SRC_DIR=..\src"
    set "THIRD_PARTY=..\third_party"
)

echo ════════════════════════════════════════════════
echo    Nova2D Engine — Incremental Builder
echo ════════════════════════════════════════════════

echo [1/4] Terminating any locked processes...
taskkill /F /IM Nova2D.exe /IM g++.exe 2>nul

if not exist build mkdir build

echo [2/4] Compiling object files...
set CXXFLAGS=-std=c++17 -Wall -Wextra -O2 -I "%SRC_DIR%" -I "%SDL2_DIR%\include\SDL2" -I "%THIRD_PARTY%\stb" -I "%THIRD_PARTY%\nlohmann"

g++ %CXXFLAGS% -c "%SRC_DIR%\core\Engine.cpp" -o build\Engine.o
g++ %CXXFLAGS% -c "%SRC_DIR%\input\InputManager.cpp" -o build\InputManager.o
g++ %CXXFLAGS% -c "%SRC_DIR%\rendering\Renderer2D.cpp" -o build\Renderer2D.o
g++ %CXXFLAGS% -c "%SRC_DIR%\rendering\Texture.cpp" -o build\Texture.o
g++ %CXXFLAGS% -c "%SRC_DIR%\rendering\Camera2D.cpp" -o build\Camera2D.o
g++ %CXXFLAGS% -c "%SRC_DIR%\rendering\SpriteAnimation.cpp" -o build\SpriteAnimation.o
g++ %CXXFLAGS% -c "%SRC_DIR%\rendering\TileMap.cpp" -o build\TileMap.o
g++ %CXXFLAGS% -c "%SRC_DIR%\rendering\ParticleSystem.cpp" -o build\ParticleSystem.o
g++ %CXXFLAGS% -c "%SRC_DIR%\physics\PhysicsWorld.cpp" -o build\PhysicsWorld.o
g++ %CXXFLAGS% -c "%SRC_DIR%\scene\Node.cpp" -o build\Node.o
g++ %CXXFLAGS% -c "%SRC_DIR%\scene\SceneTree.cpp" -o build\SceneTree.o
g++ %CXXFLAGS% -c "%SRC_DIR%\scene\ScriptNode.cpp" -o build\ScriptNode.o
g++ %CXXFLAGS% -c "%SRC_DIR%\scripting\Lexer.cpp" -o build\Lexer.o
g++ %CXXFLAGS% -c "%SRC_DIR%\scripting\Parser.cpp" -o build\Parser.o
g++ %CXXFLAGS% -c "%SRC_DIR%\scripting\Interpreter.cpp" -o build\Interpreter.o
g++ %CXXFLAGS% -c "%SRC_DIR%\scripting\ScriptBinding.cpp" -o build\ScriptBinding.o
g++ %CXXFLAGS% -c "%SRC_DIR%\audio\AudioManager.cpp" -o build\AudioManager.o
g++ %CXXFLAGS% -c "%SRC_DIR%\assets\ResourceManager.cpp" -o build\ResourceManager.o
g++ %CXXFLAGS% -c "%SRC_DIR%\main.cpp" -o build\main.o

echo [3/4] Linking Nova2D.exe...
g++ build\*.o -o build\Nova2D.exe ^
    -L "%SDL2_DIR%\lib" ^
    -lmingw32 -lSDL2main -lSDL2 ^
    -lm -ldinput8 -ldxguid -ldxerr8 -luser32 -lgdi32 -lwinmm -limm32 -lole32 -loleaut32 -lshell32 -lsetupapi -lversion -luuid -lhid -lopengl32

if %ERRORLEVEL% equ 0 (
    echo [4/4] Copying SDL2.dll and Assets...
    copy /Y "%SDL2_DIR%\bin\SDL2.dll" "build\" > nul
    if exist scripts xcopy /E /I /Y scripts build\scripts > nul
    if exist ..\scripts xcopy /E /I /Y ..\scripts build\scripts > nul
    if exist assets xcopy /E /I /Y assets build\assets > nul
    if exist ..\assets xcopy /E /I /Y ..\assets build\assets > nul
    echo.
    echo ════════════════════════════════════════════════
    echo    BUILD SUCCESSFUL! Nova2D.exe is ready.
    echo ════════════════════════════════════════════════
) else (
    echo.
    echo ❌ Compilation/Linking failed!
)
