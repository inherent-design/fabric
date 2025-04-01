#!/usr/bin/env python3

import os
import subprocess
import sys
import abc

# Define global project directories
DEBUG_DIR = os.path.join(os.path.dirname(os.path.abspath(__file__)), "debug")
PROFILE_DIR = os.path.join(os.path.dirname(os.path.abspath(__file__)), "profile")
RELEASE_DIR = os.path.join(os.path.dirname(os.path.abspath(__file__)), "release")


def configure(build_dir, build_type, options=None):
    """Configure the build environment using CMake."""
    try:
        os.makedirs(build_dir, exist_ok=True)
        cmake_args = [
            "cmake",
            "-G",
            "Ninja",
            "-S",
            ".",
            "-B",
            build_dir,
            "-DCMAKE_BUILD_TYPE=" + build_type,
        ]
        if options:
            cmake_args.extend(options)
        subprocess.run(cmake_args, check=True)
        print(f"Configuration complete for {build_type} build.")
    except subprocess.CalledProcessError as e:
        print(f"Configuration failed: {e}")
        sys.exit(1)


def print_separator(message=""):
    """Print a separator line with an optional message."""
    width = 80
    if message:
        padding = (width - len(message) - 2) // 2
        print("=" * padding + " " + message + " " + "=" * padding)
    else:
        print("=" * width)


def run(build_type="debug", execution_params=None):
    """Run the executable for the specified build type."""
    build_dirs = {"debug": DEBUG_DIR, "profile": PROFILE_DIR, "release": RELEASE_DIR}
    build_dir = build_dirs.get(build_type.lower(), DEBUG_DIR)
    executable = os.path.join(build_dir, "bin", "Fabric")
    if os.name == "nt":
        executable += ".exe"

    try:
        if not os.path.exists(executable):
            print(f"Executable not found. Building {build_type}...")
            globals()[build_type.lower()]()

        args = execution_params if execution_params else []
        print(
            f"Running with arguments: {args}" if args else "Running without arguments"
        )
        print_separator(f"BEGIN FABRIC OUTPUT ({build_type})")
        subprocess.run([executable] + args, check=True)
        print_separator("END FABRIC OUTPUT")
        print("Execution complete.")
    except subprocess.CalledProcessError as e:
        print_separator("END FABRIC OUTPUT (WITH ERROR)")
        print(f"Execution failed: {e}")
        sys.exit(1)


def clean_directory(directory):
    """Remove all files and subdirectories in the specified directory."""
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


def clean_debug():
    """Clean the debug build directory."""
    clean_directory(DEBUG_DIR)


def clean_profile():
    """Clean the profile build directory."""
    clean_directory(PROFILE_DIR)


def clean_release():
    """Clean the release build directory."""
    clean_directory(RELEASE_DIR)


def build(build_dir, build_type, flags):
    """Build the specified configuration."""
    try:
        configure(build_dir, build_type, flags)
        print_separator(f"BEGIN BUILD OUTPUT ({build_type.upper()})")
        subprocess.run(["cmake", "--build", build_dir], check=True)
        print_separator("END BUILD OUTPUT")
        print(f"{build_type.capitalize()} build complete.")
    except subprocess.CalledProcessError as e:
        print_separator("END BUILD OUTPUT (WITH ERROR)")
        print(f"{build_type.capitalize()} build failed: {e}")
        sys.exit(1)


def debug():
    """Build the debug configuration."""
    build(DEBUG_DIR, "Debug", ["-DCMAKE_CXX_FLAGS_DEBUG=-g -O0"])


def profile():
    """Build the profile configuration."""
    build(PROFILE_DIR, "RelWithDebInfo", ["-DCMAKE_CXX_FLAGS_RELWITHDEBINFO=-g -O3"])


def release():
    """Build the release configuration."""
    build(RELEASE_DIR, "Release", ["-DCMAKE_CXX_FLAGS_RELEASE=-O3"])


class Action(abc.ABC):
    """Abstract base class for actions."""

    @property
    @abc.abstractmethod
    def name(self):
        pass

    @abc.abstractmethod
    def execute(self):
        pass

    def conflicts_with(self, other_action):
        return False


class CleanAction(Action):
    @property
    def name(self):
        return "clean"

    def execute(self):
        clean_debug()


class CleanAllAction(Action):
    @property
    def name(self):
        return "clean-all"

    def execute(self):
        clean_debug()
        clean_profile()
        clean_release()


class CleanDebugAction(CleanAction):
    @property
    def name(self):
        return "clean-debug"

    def execute(self):
        clean_debug()


class CleanProfileAction(CleanAction):
    @property
    def name(self):
        return "clean-profile"

    def execute(self):
        clean_profile()


class CleanReleaseAction(CleanAction):
    @property
    def name(self):
        return "clean-release"

    def execute(self):
        clean_release()


class BuildAction(Action):
    @property
    def name(self):
        return "build"

    def execute(self):
        debug()


class BuildAllAction(Action):
    @property
    def name(self):
        return "build-all"

    def execute(self):
        debug()
        profile()
        release()


class DebugAction(Action):
    @property
    def name(self):
        return "debug"

    def execute(self):
        debug()


class ProfileAction(Action):
    @property
    def name(self):
        return "profile"

    def execute(self):
        profile()


class ReleaseAction(Action):
    @property
    def name(self):
        return "release"

    def execute(self):
        release()


class RunAction(Action):
    def __init__(self, build_type="debug"):
        self.build_type = build_type
        self.execution_params = []  # Initialize execution parameters list

    @property
    def name(self):
        return f"run-{self.build_type}"

    def execute(self):
        run(self.build_type, self.execution_params)


def parse_actions(commands):
    """Parse and optimize actions based on the provided commands."""
    action_map = {
        "clean": CleanAction(),
        "clean-debug": CleanDebugAction(),
        "clean-profile": CleanProfileAction(),
        "clean-release": CleanReleaseAction(),
        "clean-all": CleanAllAction(),
        "build": BuildAction(),
        "build-all": BuildAllAction(),
        "debug": DebugAction(),
        "profile": ProfileAction(),
        "release": ReleaseAction(),
        "run": RunAction("debug"),
        "run-debug": RunAction("debug"),
        "run-profile": RunAction("profile"),
        "run-release": RunAction("release"),
    }

    actions = []
    run_action_found = False
    clean_all_found = False
    build_all_found = False

    for command in commands:
        if run_action_found:
            # Treat all subsequent commands as execution parameters
            actions[-1].execution_params.append(command)
            continue

        action = action_map.get(command)
        if action:
            if isinstance(action, CleanAllAction):
                clean_all_found = True
            elif isinstance(action, BuildAllAction):
                build_all_found = True

            if not any(
                existing_action.conflicts_with(action) for existing_action in actions
            ):
                actions.append(action)

            if isinstance(action, RunAction):
                run_action_found = True
                action.execution_params = []  # Initialize execution parameters list

    # If clean-all is found, remove all other clean actions
    if clean_all_found:
        actions = [action for action in actions if not isinstance(action, CleanAction)]

    # If build-all is found, remove all other build actions
    if build_all_found:
        actions = [action for action in actions if not isinstance(action, BuildAction)]

    return actions


def execute_actions(actions):
    """Execute the list of actions."""
    for action in actions:
        action.execute()


def print_help():
    """Print the help message with available commands."""
    print("Usage: ./manage.py <command>")
    print("Available commands:")
    print("  clean       - Clean debug build directory")
    print("  clean-debug - Clean debug build directory")
    print("  clean-profile - Clean profile build directory")
    print("  clean-release - Clean release build directory")
    print("  clean-all   - Clean all build directories")
    print("  build       - Build with debug symbols and no optimization")
    print("  debug       - Build with debug symbols and no optimization")
    print("  profile     - Build with debug symbols and full optimization")
    print("  release     - Build with no debug symbols and full optimization")
    print("  build-all   - Build all configurations")
    print("  run         - Run the debug build")
    print("  run-debug   - Run the debug build")
    print("  run-profile - Run the profile build")
    print("  run-release - Run the release build")
    print("  help        - Show this help message")


if __name__ == "__main__":
    if len(sys.argv) < 2:
        print_help()
        sys.exit(1)

    commands = sys.argv[1:]
    actions = parse_actions(commands)
    execute_actions(actions)
