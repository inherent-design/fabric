#!/usr/bin/env python3

import os
import subprocess
import sys
import platform
import argparse
from typing import List, Optional


def get_system_key() -> str:
    """Get the system key for CMAKE_CONFIG.

    Returns:
        str: The system key ('win', 'mac', or 'linux')
    """
    system = platform.system().lower()
    if system == "windows":
        return "win"
    elif system == "darwin":
        return "mac"
    return system


CMAKE_CONFIG = {
    "debug": {
        "dir": os.path.join(os.path.dirname(os.path.abspath(__file__)), "debug"),
        "type": "Debug",
        "flags": {
            "win": ['-DCMAKE_CXX_FLAGS_DEBUG="/Zi /Od"'],
            "linux": ["-DCMAKE_CXX_FLAGS_DEBUG=-g -O0"],
            "mac": ["-DCMAKE_CXX_FLAGS_DEBUG=-g -O0"],
        },
    },
    "profile": {
        "dir": os.path.join(os.path.dirname(os.path.abspath(__file__)), "profile"),
        "type": "RelWithDebInfo",
        "flags": {
            "win": ['-DCMAKE_CXX_FLAGS_RELWITHDEBINFO="/Zi /dynamicdeopt"'],
            "linux": ["-DCMAKE_CXX_FLAGS_RELWITHDEBINFO=-g -O3"],
            "mac": ["-DCMAKE_CXX_FLAGS_RELWITHDEBINFO=-g -O3"],
        },
    },
    "release": {
        "dir": os.path.join(os.path.dirname(os.path.abspath(__file__)), "release"),
        "type": "Release",
        "flags": {
            "win": ['-DCMAKE_CXX_FLAGS_RELEASE="/O2"'],
            "linux": ["-DCMAKE_CXX_FLAGS_RELEASE=-O3"],
            "mac": ["-DCMAKE_CXX_FLAGS_RELEASE=-O3"],
        },
    },
}

SYSTEM = get_system_key()


def print_separator(message: str = "") -> None:
    """Print a separator line with an optional message.

    Args:
        message (str, optional): Message to display in the separator. Defaults to "".
    """
    width = 80
    if message:
        padding = (width - len(message) - 2) // 2
        print(
            "=" * padding
            + " "
            + message
            + " "
            + "=" * (width - padding - len(message) - 2)
        )
    else:
        print("=" * width)


def configure(build_type: str = "debug", flags: Optional[List[str]] = None) -> None:
    """Configure the build environment using CMake.

    Args:
        build_type (str, optional): Build type (debug, profile, release). Defaults to "debug".
        flags (List[str], optional): Additional CMake flags. Defaults to None.

    Raises:
        SystemExit: If configuration fails
    """
    if flags is None:
        flags = []

    try:
        print_separator(f"BEGIN CONFIGURE ({build_type.upper()})")
        build_dir = CMAKE_CONFIG[build_type]["dir"]
        build_flags = CMAKE_CONFIG[build_type]["flags"].get(SYSTEM, [])

        os.makedirs(build_dir, exist_ok=True)

        cmake_args = (
            [
                "cmake",
                "-G",
                "Ninja",
                "-S",
                ".",
                "-B",
                build_dir,
                "-DCMAKE_BUILD_TYPE=" + CMAKE_CONFIG[build_type]["type"],
                f"-DCMAKE_CACHEFILE_DIR={build_dir}",
            ]
            + build_flags
            + flags
        )

        subprocess.run(cmake_args, check=True)
        print_separator(f"END CONFIGURE ({build_type.upper()})")
    except subprocess.CalledProcessError as e:
        print(f"Configuration failed: {e}")
        sys.exit(1)
    except KeyError:
        print(f"Invalid build type: {build_type}")
        print(f"Available build types: {', '.join(CMAKE_CONFIG.keys())}")
        sys.exit(1)


def build(build_type: str = "debug", flags: Optional[List[str]] = None) -> None:
    """Build the specified configuration.

    Args:
        build_type (str, optional): Build type (debug, profile, release). Defaults to "debug".
        flags (List[str], optional): Additional CMake flags. Defaults to None.
    """
    if flags is None:
        flags = []

    try:
        configure(build_type, flags)

        print_separator(f"BEGIN BUILD OUTPUT ({build_type.upper()})")

        build_dir = CMAKE_CONFIG[build_type]["dir"]
        current_dir = os.getcwd()
        os.chdir(build_dir)
        subprocess.run(["cmake", "--build", "."], check=True)
        os.chdir(current_dir)

        print_separator(f"END BUILD OUTPUT ({build_type.upper()})")
    except subprocess.CalledProcessError as e:
        print(f"{build_type.capitalize()} build failed: {e}")
        sys.exit(1)
    except KeyError:
        print(f"Invalid build type: {build_type}")
        print(f"Available build types: {', '.join(CMAKE_CONFIG.keys())}")
        sys.exit(1)


def run(
    build_type: str = "debug", execution_params: Optional[List[str]] = None
) -> None:
    """Run the executable for the specified build type.

    Args:
        build_type (str, optional): Build type (debug, profile, release). Defaults to "debug".
        execution_params (List[str], optional): Parameters to pass to the executable. Defaults to None.

    Raises:
        SystemExit: If execution fails
    """
    if execution_params is None:
        execution_params = []

    try:
        build_dir = CMAKE_CONFIG[build_type]["dir"]
        executable = os.path.join(build_dir, "bin", "Fabric")
        if os.name == "nt":
            executable += ".exe"

        if not os.path.exists(executable):
            print(f"Executable not found. Building {build_type}...")
            build(build_type)

        print(
            f"Running with arguments: {execution_params}"
            if execution_params
            else "Running without arguments"
        )

        print_separator(f"BEGIN FABRIC OUTPUT ({build_type.upper()})")
        subprocess.run([executable] + execution_params, check=True)
        print_separator(f"END FABRIC OUTPUT ({build_type.upper()})")
    except subprocess.CalledProcessError as e:
        print(f"Execution failed: {e}")
        sys.exit(1)
    except KeyError:
        print(f"Invalid build type: {build_type}")
        print(f"Available build types: {', '.join(CMAKE_CONFIG.keys())}")
        sys.exit(1)


def clean_directory(directory: str) -> None:
    """Remove all files and subdirectories in the specified directory.

    Args:
        directory (str): Directory to clean
    """
    try:
        if os.path.exists(directory):
            for root, dirs, files in os.walk(directory, topdown=False):
                for name in files:
                    os.remove(os.path.join(root, name))
                for name in dirs:
                    os.rmdir(os.path.join(root, name))
            os.rmdir(directory)
        print(f"Cleaned {os.path.basename(directory)} build directory.")
    except Exception as e:
        print(f"An error occurred during cleaning {os.path.basename(directory)}: {e}")


def clean(build_type: str = "all") -> None:
    """Clean build directories.

    Args:
        build_type (str, optional): Build type to clean (debug, profile, release, all). Defaults to "all".
    """
    if build_type == "all":
        for config in CMAKE_CONFIG:
            clean_directory(CMAKE_CONFIG[config]["dir"])
    elif build_type in CMAKE_CONFIG:
        clean_directory(CMAKE_CONFIG[build_type]["dir"])
    else:
        print(f"Invalid build type: {build_type}")
        print(f"Available build types: {', '.join(CMAKE_CONFIG.keys())} or 'all'")


def build_all() -> None:
    """Build all configurations."""
    for build_type in CMAKE_CONFIG:
        build(build_type)


def print_help() -> None:
    """Print the help message with available commands."""
    print("Usage: ./ActionMan.py <command> [options]")
    print("")
    print("Available commands:")
    print("  clean       - (default: all; auto-cleans)")
    print("    - debug   - Clean debug build directory")
    print("    - profile - Clean profile build directory")
    print("    - release - Clean release build directory")
    print("    - all     - Clean all build directories")
    print("  build       - (default: debug; auto-builds)")
    print("    - debug   - Build with debug symbols and no optimization")
    print("    - profile - Build with debug symbols and full optimization")
    print("    - release - Build with no debug symbols and full optimization")
    print("    - all     - Build all configurations")
    print("  run         - (default: debug; auto-runs build)")
    print("    - debug   - Run the debug build")
    print("    - profile - Run the profile build")
    print("    - release - Run the release build")
    print("  help        - Show this help message")


class CommandParser:
    """Parser for ActionMan commands."""

    def __init__(self):
        """Initialize the command parser."""
        self.parser = argparse.ArgumentParser(
            description="ActionMan - Build and run management tool", add_help=False
        )
        self.parser.add_argument(
            "command",
            nargs="?",
            default="help",
            help="Command to execute (clean, build, run, help)",
        )
        self.parser.add_argument("options", nargs="*", help="Options for the command")

    def parse_args(self, args: List[str]) -> argparse.Namespace:
        """Parse command line arguments.

        Args:
            args (List[str]): Command line arguments

        Returns:
            argparse.Namespace: Parsed arguments
        """
        return self.parser.parse_args(args)

    def execute(self, args: List[str]) -> None:
        """Execute the command based on parsed arguments.

        Args:
            args (List[str]): Command line arguments
        """
        parsed_args = self.parse_args(args)
        command = parsed_args.command.lower()
        options = parsed_args.options

        if command == "help" or not command:
            print_help()
            return

        if command == "clean":
            build_type = options[0] if options else "all"
            clean(build_type)
            return

        if command == "build":
            if options and options[0] == "all":
                build_all()
            else:
                build_type = options[0] if options else "debug"
                build(build_type)
            return

        if command == "run":
            build_type = options[0] if options else "debug"
            execution_params = options[1:] if len(options) > 1 else []
            run(build_type, execution_params)
            return

        print(f"Unknown command: {command}")
        print_help()


if __name__ == "__main__":
    parser = CommandParser()
    parser.execute(sys.argv[1:])
