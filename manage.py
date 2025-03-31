#!/usr/bin/env python3

import os
import subprocess
import sys

# Define global project variables
BUILD_DIR = "./build"
EXECUTABLE = os.path.join(BUILD_DIR, "bin", "fabric")
if os.name == "nt":  # Check if the operating system is Windows
    EXECUTABLE = os.path.join(BUILD_DIR, "bin", "fabric.exe")


def clean():
    try:
        if os.path.exists(BUILD_DIR):
            for root, dirs, files in os.walk(BUILD_DIR, topdown=False):
                for name in files:
                    os.remove(os.path.join(root, name))
                for name in dirs:
                    os.rmdir(os.path.join(root, name))
            os.rmdir(BUILD_DIR)
        print("Cleaned build directory.")
    except Exception as e:
        print(f"An error occurred during cleaning: {e}")


def configure():
    try:
        subprocess.run(["cmake", "-G", "Ninja", "-B", BUILD_DIR], check=True)
        print("Configuration complete.")
    except subprocess.CalledProcessError as e:
        print(f"An error occurred during configuration: {e}")


def compile():
    try:
        if not os.path.exists(BUILD_DIR):
            configure()
        subprocess.run(["cmake", "--build", BUILD_DIR], check=True)
        print("Compilation complete.")
    except subprocess.CalledProcessError as e:
        print(f"An error occurred during compilation: {e}")


def run():
    try:
        if not os.path.exists(EXECUTABLE):
            print("Executable not found. Compiling...")
            compile()
        args = sys.argv[2:]
        if args:
            print(f"Running with arguments: {args}")
            subprocess.run([EXECUTABLE] + args, check=True)
        else:
            print("Running without arguments.")
            subprocess.run([EXECUTABLE], check=True)
        print("Execution complete.")
    except subprocess.CalledProcessError as e:
        print(f"An error occurred during execution: {e}")


def main(task):
    if task == "clean":
        clean()
    elif task == "configure":
        configure()
    elif task == "compile":
        compile()
    elif task == "all":
        clean()
        configure()
        compile()
    elif task == "run":
        run()
    else:
        print("Invalid task. Please specify 'clean', 'configure', 'compile', or 'all'.")


if __name__ == "__main__":
    if len(sys.argv) < 2:
        print("Usage: ./manage.py <task>")
        print("Available tasks:")
        print("  clean")
        print("  configure")
        print("  compile")
        print("  all")
    else:
        main(sys.argv[1])
