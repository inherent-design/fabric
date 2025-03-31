#!/usr/bin/env python3

import os
import subprocess
import sys

# Define global project variables
DEBUG_DIR = os.path.join(os.path.dirname(os.path.abspath(__file__)), "debug")
PROFILE_DIR = os.path.join(os.path.dirname(os.path.abspath(__file__)), "profile")
RELEASE_DIR = os.path.join(os.path.dirname(os.path.abspath(__file__)), "release")
BUILD_DIR = DEBUG_DIR  # Default to debug build


def get_executable(build_dir):
    executable = os.path.join(build_dir, "bin", "fabric")
    if os.name == "nt":  # Check if the operating system is Windows
        executable = os.path.join(build_dir, "bin", "fabric.exe")
    return executable


def clean():
    try:
        for dir_path in [DEBUG_DIR, PROFILE_DIR, RELEASE_DIR]:
            if os.path.exists(dir_path):
                for root, dirs, files in os.walk(dir_path, topdown=False):
                    for name in files:
                        os.remove(os.path.join(root, name))
                    for name in dirs:
                        os.rmdir(os.path.join(root, name))
                os.rmdir(dir_path)
        print("Cleaned build directories.")
    except Exception as e:
        print(f"An error occurred during cleaning: {e}")


def configure(build_dir, build_type, options=None):
    try:
        if not os.path.exists(build_dir):
            os.makedirs(build_dir)

        cmake_args = ["cmake", "-G", "Ninja", "-S", ".", "-B", build_dir]

        # Add build type
        cmake_args.extend(["-DCMAKE_BUILD_TYPE=" + build_type])

        # Add any additional options
        if options:
            cmake_args.extend(options)

        subprocess.run(cmake_args, check=True)
        print(f"Configuration complete for {build_type} build.")
    except subprocess.CalledProcessError as e:
        print(f"Configuration failed: {e}")
        sys.exit(1)


def debug():
    """Build with debug symbols and no optimization"""
    try:
        configure(DEBUG_DIR, "Debug", ["-DCMAKE_CXX_FLAGS_DEBUG=-g -O0"])
        subprocess.run(["cmake", "--build", DEBUG_DIR], check=True)
        print("Debug build complete.")
    except subprocess.CalledProcessError as e:
        print(f"Debug build failed: {e}")
        sys.exit(1)


def profile():
    """Build with debug symbols and full optimization"""
    try:
        configure(
            PROFILE_DIR, "RelWithDebInfo", ["-DCMAKE_CXX_FLAGS_RELWITHDEBINFO=-g -O3"]
        )
        subprocess.run(["cmake", "--build", PROFILE_DIR], check=True)
        print("Profile build complete.")
    except subprocess.CalledProcessError as e:
        print(f"Profile build failed: {e}")
        sys.exit(1)


def release():
    """Build with no debug symbols and full optimization"""
    try:
        configure(RELEASE_DIR, "Release", ["-DCMAKE_CXX_FLAGS_RELEASE=-O3"])
        subprocess.run(["cmake", "--build", RELEASE_DIR], check=True)
        print("Release build complete.")
    except subprocess.CalledProcessError as e:
        print(f"Release build failed: {e}")
        sys.exit(1)


# Keep the original compile function but make it use debug
def compile():
    debug()


def run(build_type="debug"):
    """Run the executable with optional arguments"""
    build_dirs = {"debug": DEBUG_DIR, "profile": PROFILE_DIR, "release": RELEASE_DIR}

    build_dir = build_dirs.get(build_type.lower(), DEBUG_DIR)
    executable = get_executable(build_dir)

    try:
        if not os.path.exists(executable):
            print(f"Executable not found. Building {build_type}...")
            globals()[build_type.lower()]()

        args = sys.argv[2:] if len(sys.argv) > 2 else []
        if args:
            print(f"Running with arguments: {args}")
            subprocess.run([executable] + args, check=True)
        else:
            print("Running without arguments")
            subprocess.run([executable], check=True)
        print("Execution complete.")
    except subprocess.CalledProcessError as e:
        print(f"Execution failed: {e}")
        sys.exit(1)


def print_help():
    print("Usage: ./manage.py <command>")
    print("Available commands:")
    print("  debug       - Build with debug symbols and no optimization")
    print("  profile     - Build with debug symbols and full optimization")
    print("  release     - Build with no debug symbols and full optimization")
    print("  compile     - Alias for debug")
    print("  run         - Run the debug build")
    print("  run-debug   - Run the debug build")
    print("  run-profile - Run the profile build")
    print("  run-release - Run the release build")
    print("  clean       - Clean all build directories")
    print("  help        - Show this help message")


if __name__ == "__main__":
    if len(sys.argv) < 2:
        print_help()
        sys.exit(1)

    command = sys.argv[1]
    commands = {
        "configure": lambda: configure(DEBUG_DIR, "Debug"),
        "debug": debug,
        "profile": profile,
        "release": release,
        "compile": compile,  # Keep for backward compatibility
        "run": lambda: run("debug"),
        "clean": clean,
        "help": print_help,
    }

    if command in commands:
        commands[command]()
    elif command.startswith("run-"):
        # Allow run-debug, run-profile, run-release
        build_type = command[4:]
        if build_type in ["debug", "profile", "release"]:
            run(build_type)
        else:
            print(f"Unknown build type: {build_type}")
            sys.exit(1)
    else:
        print(f"Unknown command: {command}")
        print_help()
        sys.exit(1)
