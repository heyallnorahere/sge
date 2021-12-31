from os import mkdir, getenv
from requests import get
from sys import argv
from subprocess import call
from platform import system
import os.path as path
VULKAN_SDK_VERSION = "1.2.182.0"
INSTALLER_DIR_NAME = "build"
def retrieve_installer(url: str, output_name: str):
    response = get(url, allow_redirects=True)
    installer_path = path.join(INSTALLER_DIR_NAME, output_name)
    with open(installer_path, "wb") as stream:
        stream.write(response.content)
        stream.close()
    return installer_path
def set_env_variable(location: str):
    if len(argv) > 1:
        if argv[1] == "--gh-actions":
            print(f"::set-env name=VULKAN_SDK::{location}")
def windows():
    installer_url = f"https://sdk.lunarg.com/sdk/download/{VULKAN_SDK_VERSION}/windows/VulkanSDK-{VULKAN_SDK_VERSION}-Installer.exe"
    installer_path = retrieve_installer(installer_url, "vulkan_installer.exe")
    return_code = call([ installer_path, "/S" ], shell=True)
    if return_code != 0:
        return 2
    set_env_variable(f"C:\\VulkanSDK\\{VULKAN_SDK_VERSION}")
    return 0
def macosx():
    installer_url = f"https://sdk.lunarg.com/sdk/download/{VULKAN_SDK_VERSION}/mac/vulkansdk-macos-{VULKAN_SDK_VERSION}.dmg"
    disk_image_name = f"vulkansdk-macos-{VULKAN_SDK_VERSION}"
    disk_image_path = retrieve_installer(installer_url, f"{disk_image_name}.dmg")
    if call(["sudo", "hdiutil", "attach", disk_image_path]) != 0:
        return 2
    if call(["sudo", f"/Volumes/{disk_image_name}/InstallVulkan.app/Contents/MacOS/InstallVulkan", "in", "--al", "-c"]) != 0:
        return 3
    home_dir = getenv("HOME")
    set_env_variable(f"{home_dir}/VulkanSDK/{VULKAN_SDK_VERSION}/macOS")
    return 0
PLATFORM_CALLBACKS = {
    "Windows": windows,
    "Darwin": macosx
}
def main():
    if not path.exists(INSTALLER_DIR_NAME):
        mkdir(INSTALLER_DIR_NAME)
    try:
        callback = PLATFORM_CALLBACKS[system()]
    except KeyError:
        print("This script may only run on Windows and MacOS X!")
        return 1
    return callback()
if __name__ == "__main__":
    exit(main())