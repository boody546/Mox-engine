#!/bin/bash
echo "════════════════════════════════════════════════"
echo "   Building Nova2D Engine..."
echo "════════════════════════════════════════════════"

mkdir -p build

# Detect OS
OS="$(uname -s)"
if [ "$OS" = "Darwin" ]; then
    echo "Platform detected: macOS"
    COMPILER="clang++"
    SDL2_FLAGS=$(sdl2-config --cflags --libs 2>/dev/null || echo "-I/usr/local/include/SDL2 -L/usr/local/lib -lSDL2")
else
    echo "Platform detected: Linux"
    COMPILER="g++"
    SDL2_FLAGS=$(sdl2-config --cflags --libs 2>/dev/null || echo "-I/usr/include/SDL2 -lSDL2")
fi

echo "Compiling source files using $COMPILER..."
$COMPILER -std=c++17 -Wall -Wextra -O2 \
    -I src -I third_party/stb -I third_party/nlohmann $SDL2_FLAGS \
    src/core/Engine.cpp \
    src/input/InputManager.cpp \
    src/rendering/Renderer2D.cpp src/rendering/Texture.cpp src/rendering/Camera2D.cpp src/rendering/SpriteAnimation.cpp src/rendering/TileMap.cpp src/rendering/ParticleSystem.cpp \
    src/physics/PhysicsWorld.cpp \
    src/scene/Node.cpp src/scene/SceneTree.cpp \
    src/scripting/Lexer.cpp src/scripting/Parser.cpp src/scripting/Interpreter.cpp \
    src/audio/AudioManager.cpp \
    src/assets/ResourceManager.cpp \
    src/main.cpp \
    -o build/Nova2D

if [ $? -eq 0 ]; then
    echo "Build Successful!"
    # Copy assets & scripts to build folder
    if [ -d "assets" ]; then
        cp -r assets build/
    fi
    if [ -d "scripts" ]; then
        cp -r scripts build/
    fi
    echo "════════════════════════════════════════════════"
    echo "   Nova2D is ready at: build/Nova2D"
    echo "════════════════════════════════════════════════"
else
    echo "❌ Build Failed!"
    exit 1
fi
