#!/usr/bin/env python3
import os
import sys
import shutil
import argparse
import subprocess
import urllib.request

# --- Configuration -------------------------------------------------
SRC_FILES = [
    "src/core/Engine.cpp",
    "src/input/InputManager.cpp",
    "src/rendering/Renderer2D.cpp",
    "src/rendering/Texture.cpp",
    "src/rendering/Camera2D.cpp",
    "src/rendering/SpriteAnimation.cpp",
    "src/rendering/TileMap.cpp",
    "src/rendering/ParticleSystem.cpp",
    "src/physics/PhysicsWorld.cpp",
    "src/scene/Node.cpp",
    "src/scene/SceneTree.cpp",
    "src/scene/ScriptNode.cpp",
    "src/scripting/Lexer.cpp",
    "src/scripting/Parser.cpp",
    "src/scripting/Interpreter.cpp",
    "src/scripting/ScriptBinding.cpp",
    "src/audio/AudioManager.cpp",
    "src/assets/ResourceManager.cpp",
    "src/main.cpp",
]

SDL2_WIN_DIR = "third_party/SDL2-2.30.2/x86_64-w64-mingw32"

# --- Helper Functions ----------------------------------------------
def clean_dir(path):
    if os.path.exists(path):
        print(f"Cleaning existing directory: {path}")
        shutil.rmtree(path)
    os.makedirs(path, exist_ok=True)

def copy_assets_and_scripts(dest_dir):
    print("Copying game assets and scripts...")
    if os.path.exists("assets"):
        shutil.copytree("assets", os.path.join(dest_dir, "assets"), dirs_exist_ok=True)
    if os.path.exists("scripts"):
        shutil.copytree("scripts", os.path.join(dest_dir, "scripts"), dirs_exist_ok=True)

def run_command(cmd, env=None):
    print(f"Executing: {' '.join(cmd)}")
    result = subprocess.run(cmd, env=env, shell=True if os.name == 'nt' else False)
    if result.returncode != 0:
        print(f"[ERROR] Command failed with code {result.returncode}")
        sys.exit(1)

# --- Exporters -----------------------------------------------------
def export_windows(clean):
    dist_dir = "dist/windows"
    if clean:
        clean_dir(dist_dir)
    else:
        os.makedirs(dist_dir, exist_ok=True)

    print("================================================")
    print("Exporting Nova2D for Windows...")
    print("================================================")

    # Add compiler path if present
    env = os.environ.copy()
    devkit_path = r"C:\Users\walaa\w64devkit\bin"
    if os.path.exists(devkit_path):
        env["PATH"] = devkit_path + os.pathsep + env["PATH"]

    # Compile command
    cmd = [
        "g++", "-std=c++17", "-Wall", "-Wextra", "-O2",
        "-I", "src",
        "-I", f"{SDL2_WIN_DIR}/include/SDL2",
        "-I", "third_party/stb",
        "-I", "third_party/nlohmann",
    ] + SRC_FILES + [
        "-o", os.path.join(dist_dir, "Nova2D.exe"),
        "-L", f"{SDL2_WIN_DIR}/lib",
        "-lmingw32", "-lSDL2main", "-lSDL2", "-mwindows",
        "-lm", "-ldinput8", "-ldxguid", "-ldxerr8", "-luser32", "-lgdi32", 
        "-lwinmm", "-limm32", "-lole32", "-loleaut32", "-lshell32", 
        "-lsetupapi", "-lversion", "-luuid", "-lhid", "-lopengl32"
    ]

    run_command(cmd, env=env)
    
    # Copy DLL
    shutil.copy(f"{SDL2_WIN_DIR}/bin/SDL2.dll", os.path.join(dist_dir, "SDL2.dll"))
    copy_assets_and_scripts(dist_dir)

    print(f"[SUCCESS] Export Successful! Location: {dist_dir}")


def export_linux(clean):
    dist_dir = "dist/linux"
    if clean:
        clean_dir(dist_dir)
    else:
        os.makedirs(dist_dir, exist_ok=True)

    print("================================================")
    print("Exporting Nova2D for Linux...")
    print("================================================")

    # Determine SDL2 flags via sdl2-config or default fallback
    sdl_flags = ["-I/usr/include/SDL2", "-lSDL2"]
    try:
        flags = subprocess.check_output(["sdl2-config", "--cflags", "--libs"]).decode().strip().split()
        if flags:
            sdl_flags = flags
    except Exception:
        print("Warning: sdl2-config not found. Using default paths.")

    cmd = [
        "g++", "-std=c++17", "-Wall", "-Wextra", "-O2",
        "-I", "src",
        "-I", "third_party/stb",
        "-I", "third_party/nlohmann",
    ] + sdl_flags + SRC_FILES + [
        "-o", os.path.join(dist_dir, "Nova2D")
    ]

    run_command(cmd)
    copy_assets_and_scripts(dist_dir)
    print(f"[SUCCESS] Export Successful! Location: {dist_dir}")


def export_macos(clean):
    dist_dir = "dist/macos"
    app_dir = os.path.join(dist_dir, "Nova2D.app")
    contents_dir = os.path.join(app_dir, "Contents")
    macos_dir = os.path.join(contents_dir, "MacOS")
    resources_dir = os.path.join(contents_dir, "Resources")

    if clean:
        clean_dir(dist_dir)

    os.makedirs(macos_dir, exist_ok=True)
    os.makedirs(resources_dir, exist_ok=True)

    print("================================================")
    print("Exporting Nova2D for macOS (.app)...")
    print("================================================")

    # Find SDL2 via brew or config
    sdl_flags = ["-I/usr/local/include/SDL2", "-L/usr/local/lib", "-lSDL2"]
    try:
        flags = subprocess.check_output(["sdl2-config", "--cflags", "--libs"]).decode().strip().split()
        if flags:
            sdl_flags = flags
    except Exception:
        pass

    cmd = [
        "clang++", "-std=c++17", "-Wall", "-Wextra", "-O2",
        "-I", "src",
        "-I", "third_party/stb",
        "-I", "third_party/nlohmann",
    ] + sdl_flags + SRC_FILES + [
        "-o", os.path.join(macos_dir, "Nova2D")
    ]

    run_command(cmd)

    # Generate Info.plist
    plist_content = """<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE plist PUBLIC "-//Apple//DTD PLIST 1.0//EN" "http://www.apple.com/DTDs/PropertyList-1.0.dtd">
<plist version="1.0">
<dict>
    <key>CFBundleExecutable</key>
    <string>Nova2D</string>
    <key>CFBundleIdentifier</key>
    <string>org.nova2d.engine</string>
    <key>CFBundleName</key>
    <string>Nova2D</string>
    <key>CFBundlePackageType</key>
    <string>APPL</string>
    <key>CFBundleShortVersionString</key>
    <string>1.0</string>
</dict>
</plist>
"""
    with open(os.path.join(contents_dir, "Info.plist"), "w") as f:
        f.write(plist_content)

    # Copy assets/scripts to app resources
    copy_assets_and_scripts(resources_dir)
    print(f"[SUCCESS] Export Successful! Location: {app_dir}")


def export_android(clean):
    dist_dir = "dist/android"
    if clean:
        clean_dir(dist_dir)
    else:
        os.makedirs(dist_dir, exist_ok=True)

    print("================================================")
    print("Scaffolding Nova2D for Android (Gradle Project)...")
    print("================================================")

    # 1. Copy android scaffolding files
    scaffold_src = "platforms/android"
    if os.path.exists(scaffold_src):
        shutil.copytree(scaffold_src, dist_dir, dirs_exist_ok=True)
    else:
        print("[ERROR] Scaffolding source directory not found. Run from project root.")
        return

    # 2. Setup folders
    java_app_dir = os.path.join(dist_dir, "app/src/main/java/org/libsdl/app")
    os.makedirs(java_app_dir, exist_ok=True)
    
    # 3. Download SDL2 Java files if not present
    print("Downloading SDL2 Android Java Bindings...")
    sdl_java_files = [
        "SDLActivity.java",
        "SDLControllerManager.java",
        "SDLAudioManager.java",
        "SDLJoystickHandler.java",
        "SDL.java",
    ]
    github_url = "https://raw.githubusercontent.com/libsdl-org/SDL/SDL2/android-project/app/src/main/java/org/libsdl/app/"
    
    import ssl
    try:
        ssl_context = ssl._create_unverified_context()
    except AttributeError:
        ssl_context = None

    for f in sdl_java_files:
        dest_file = os.path.join(java_app_dir, f)
        if not os.path.exists(dest_file):
            print(f"  Fetching {f}...")
            try:
                if ssl_context:
                    req = urllib.request.Request(
                        github_url + f,
                        headers={'User-Agent': 'Mozilla/5.0'}
                    )
                    with urllib.request.urlopen(req, context=ssl_context) as response, open(dest_file, 'wb') as out_file:
                        out_file.write(response.read())
                else:
                    urllib.request.urlretrieve(github_url + f, dest_file)
            except Exception as e:
                print(f"  [WARNING] Could not download {f}: {e}")

    # 4. Copy engine src files & assets/scripts
    assets_dest = os.path.join(dist_dir, "app/src/main/assets")
    copy_assets_and_scripts(assets_dest)

    print("\n[SUCCESS] Android Project Scaffolding Successful!")
    print(f"Location: {dist_dir}")
    print("\nTo build the APK:")
    print(f"1. Open '{dist_dir}' in Android Studio or command-line.")
    print("2. Ensure Android NDK and CMake are installed.")
    print("3. Run: ./gradlew assembleDebug")


def export_ios(clean):
    dist_dir = "dist/ios"
    if clean:
        clean_dir(dist_dir)
    else:
        os.makedirs(dist_dir, exist_ok=True)

    print("================================================")
    print("Generating Nova2D iOS Project using CMake...")
    print("================================================")

    # Generate Xcode project using CMake
    build_dir = os.path.join(dist_dir, "build")
    os.makedirs(build_dir, exist_ok=True)

    # Command to configure CMake for iOS
    cmd = [
        "cmake",
        "-G", "Xcode",
        "-DCMAKE_SYSTEM_NAME=iOS",
        "-DCMAKE_OSX_SYSROOT=iphoneos",
        "-S", ".",
        "-B", build_dir
    ]

    try:
        run_command(cmd)
        copy_assets_and_scripts(os.path.join(dist_dir, "assets"))
        print("\n[SUCCESS] iOS Xcode Project Generation Successful!")
        print(f"Project path: {build_dir}/Nova2D.xcodeproj")
        print("\nTo compile and run:")
        print("1. Open the project in Xcode on a Mac.")
        print("2. Set up your Provisioning Profile and target device.")
        print("3. Build and Run!")
    except Exception as e:
        print(f"[ERROR] Failed to generate Xcode project: {e}")
        print("Note: Xcode build generation requires CMake and Xcode Command Line Tools installed.")


# --- CLI Entry -----------------------------------------------------
def main():
    parser = argparse.ArgumentParser(description="Nova2D Game Engine Cross-Platform Exporter")
    parser.add_argument(
        "--platform",
        choices=["windows", "linux", "macos", "android", "ios"],
        required=True,
        help="Target platform to export the game to"
    )
    parser.add_argument(
        "--clean",
        action="store_true",
        help="Clean the build directory before building"
    )

    args = parser.parse_args()

    if args.platform == "windows":
        export_windows(args.clean)
    elif args.platform == "linux":
        export_linux(args.clean)
    elif args.platform == "macos":
        export_macos(args.clean)
    elif args.platform == "android":
        export_android(args.clean)
    elif args.platform == "ios":
        export_ios(args.clean)

if __name__ == "__main__":
    main()
