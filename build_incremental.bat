@echo off
set "DK=C:\Users\walaa\w64devkit"
set "PATH=%DK%\bin;%PATH%"
set "SDL2_DIR=third_party\SDL2-2.30.2\x86_64-w64-mingw32"

echo ════════════════════════════════════════════════
echo    Nova2D Engine — Incremental Builder
echo ════════════════════════════════════════════════

echo [1/4] Terminating any locked processes...
taskkill /F /IM Nova2D.exe /IM g++.exe 2>nul

if not exist build mkdir build

echo [2/4] Compiling object files...
set CXXFLAGS=-std=c++17 -Wall -Wextra -O2 -I src -I "%SDL2_DIR%\include\SDL2" -I third_party\stb -I third_party\nlohmann

g++ %CXXFLAGS% -c src\core\Engine.cpp -o build\Engine.o
g++ %CXXFLAGS% -c src\input\InputManager.cpp -o build\InputManager.o
g++ %CXXFLAGS% -c src\rendering\Renderer2D.cpp -o build\Renderer2D.o
g++ %CXXFLAGS% -c src\rendering\Texture.cpp -o build\Texture.o
g++ %CXXFLAGS% -c src\rendering\Camera2D.cpp -o build\Camera2D.o
g++ %CXXFLAGS% -c src\rendering\SpriteAnimation.cpp -o build\SpriteAnimation.o
g++ %CXXFLAGS% -c src\rendering\TileMap.cpp -o build\TileMap.o
g++ %CXXFLAGS% -c src\rendering\ParticleSystem.cpp -o build\ParticleSystem.o
g++ %CXXFLAGS% -c src\physics\PhysicsWorld.cpp -o build\PhysicsWorld.o
g++ %CXXFLAGS% -c src\scene\Node.cpp -o build\Node.o
g++ %CXXFLAGS% -c src\scene\SceneTree.cpp -o build\SceneTree.o
g++ %CXXFLAGS% -c src\scene\ScriptNode.cpp -o build\ScriptNode.o
g++ %CXXFLAGS% -c src\scripting\Lexer.cpp -o build\Lexer.o
g++ %CXXFLAGS% -c src\scripting\Parser.cpp -o build\Parser.o
g++ %CXXFLAGS% -c src\scripting\Interpreter.cpp -o build\Interpreter.o
g++ %CXXFLAGS% -c src\scripting\ScriptBinding.cpp -o build\ScriptBinding.o
g++ %CXXFLAGS% -c src\audio\AudioManager.cpp -o build\AudioManager.o
g++ %CXXFLAGS% -c src\assets\ResourceManager.cpp -o build\ResourceManager.o
g++ %CXXFLAGS% -c src\main.cpp -o build\main.o

echo [3/4] Linking Nova2D.exe...
g++ build\*.o -o build\Nova2D.exe ^
    -L "%SDL2_DIR%\lib" ^
    -lmingw32 -lSDL2main -lSDL2 ^
    -lm -ldinput8 -ldxguid -ldxerr8 -luser32 -lgdi32 -lwinmm -limm32 -lole32 -loleaut32 -lshell32 -lsetupapi -lversion -luuid -lhid -lopengl32

if %ERRORLEVEL% equ 0 (
    echo [4/4] Copying SDL2.dll...
    copy /Y "%SDL2_DIR%\bin\SDL2.dll" "build\" > nul
    echo.
    echo ════════════════════════════════════════════════
    echo    SUCCESS! Starting Nova2D Engine...
    echo ════════════════════════════════════════════════
    start "" build\Nova2D.exe
) else (
    echo.
    echo ❌ Compilation/Linking failed!
    pause
)
