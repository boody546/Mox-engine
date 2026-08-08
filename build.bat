@echo off
set "DK=C:\Users\walaa\w64devkit"
set "PATH=%DK%\bin;%PATH%"

set "SDL2_DIR=third_party\SDL2-2.30.2\x86_64-w64-mingw32"

echo ════════════════════════════════════════════════
echo    Building Nova2D Engine...
echo ════════════════════════════════════════════════

if not exist build mkdir build

echo Compiling source files...
g++ -std=c++17 -Wall -Wextra -O2 ^
    -I src -I "%SDL2_DIR%\include\SDL2" -I third_party\stb -I third_party\nlohmann ^
    src\core\Engine.cpp ^
    src\input\InputManager.cpp ^
    src\rendering\Renderer2D.cpp src\rendering\Texture.cpp src\rendering\Camera2D.cpp src\rendering\SpriteAnimation.cpp src\rendering\TileMap.cpp src\rendering\ParticleSystem.cpp ^
    src\physics\PhysicsWorld.cpp ^
    src\scene\Node.cpp src\scene\SceneTree.cpp src\scene\ScriptNode.cpp ^
    src\scripting\Lexer.cpp src\scripting\Parser.cpp src\scripting\Interpreter.cpp src\scripting\ScriptBinding.cpp ^
    src\audio\AudioManager.cpp ^
    src\assets\ResourceManager.cpp ^
    src\main.cpp ^
    -o build\Nova2D.exe ^
    -L "%SDL2_DIR%\lib" ^
    -lmingw32 -lSDL2main -lSDL2 ^
    -lm -ldinput8 -ldxguid -ldxerr8 -luser32 -lgdi32 -lwinmm -limm32 -lole32 -loleaut32 -lshell32 -lsetupapi -lversion -luuid -lhid -lopengl32

if %ERRORLEVEL% equ 0 (
    echo Build Successful!
    echo Copying required DLLs...
    copy /Y "%SDL2_DIR%\bin\SDL2.dll" "build\" > nul
    echo.
    echo ════════════════════════════════════════════════
    echo    Nova2D is ready at: build\Nova2D.exe
    echo ════════════════════════════════════════════════
) else (
    echo.
    echo ❌ Build Failed!
)
