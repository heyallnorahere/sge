#!/usr/bin/python3

from subprocess import call
from platform import system
import os.path as path

SCRIPT_DIR = path.dirname(path.realpath(__file__))
MONO_DIR = path.join(SCRIPT_DIR, "build")

MONO_URL = "https://github.com/mono/mono.git"
INSTALL_PREFIX = "/usr/local"
VS_PATH = path.join("C:\\Program Files\\Microsoft Visual Studio\\2022")

def clone():
    print("Cloning mono...")

    if path.isdir(MONO_DIR):
        print(f"{MONO_DIR} already exists - skipping")
        return True

    if call(["git", "clone", MONO_URL, MONO_DIR]) != 0:
        print("Failed to clone mono!")
        return False
    else:
        print("Successfully cloned mono!")
        return True

def buildUnix():
    configureScript = path.join(MONO_DIR, "autogen.sh")
    params = [configureScript, f"--prefix={INSTALL_PREFIX}"]

    if system() == "Darwin":
        params.append("--disable-nls")

    if call(params, cwd=MONO_DIR) != 0:
        print("Failed to configure mono!")
        return False

    if call(["make"], cwd=MONO_DIR) != 0:
        print("Failed to build mono!")
        return False

    shouldInstall = None
    while not shouldInstall is None:
        answer = input(f"Install to {INSTALL_PREFIX}? [y/N] ").lower()

        if len(answer) == 0 or answer == "n":
            shouldInstall = False
        elif answer == "y":
            shouldInstall = True
        else:
            print("Invalid answer - try again")

    if shouldInstall:
        if call(["sudo", "make", "install"], cwd=MONO_DIR) != 0:
            print("Failed to install mono!")
            return False

    return True

def buildWindows():
    vsVersions = ["Enterprise", "Professional", "Community"]

    vsPath = None
    for version in vsVersions:
        currentPath = path.join(VS_PATH, version)

        if path.isdir(currentPath):
            vsPath = currentPath
            break

    if vsPath is None:
        print("Visual Studio 2022 not found!")
        return False

    msbuildPath = path.join(vsPath, "MSBuild", "Current", "Bin", "MSBuild.exe")
    if not path.exists(msbuildPath):
        print("MSBuild not found!")
        return False

    slnPath = path.join(MONO_DIR, "msvc", "mono.sln")
    if call([msbuildPath, "-noLogo", "-p:Configuration=Release", slnPath]) != 0:
        print("Could not compile mono!")
        return False

    return True

def main():
    print("If any error occurs, please refer to the documentation on building mono (https://www.mono-project.com/docs/compiling-mono/)")

    if not clone():
        return 1

    buildFunctions = {
        "Darwin": buildUnix,
        "Linux": buildUnix,
        "Windows": buildWindows
    }

    targetPlatform = system()
    try:
        func = buildFunctions[targetPlatform]

        print(f"Building mono for platform: {targetPlatform}")
        if not func():
            return 1
    except KeyError:
        print(f"Unsupported platform: {targetPlatform}")
        return 1

    return 0

if __name__ == "__main__":
    exit(main())