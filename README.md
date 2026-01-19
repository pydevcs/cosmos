# Cosmos - Flipper Zero Application

A custom application for the Flipper Zero device.

## Installation Guide

This guide explains how to install the Cosmos application by symlinking it to your Flipper Zero firmware's `applications_user` folder. This approach allows you to maintain the application outside of the firmware repository, making it easier to build for multiple firmware versions if needed.

### Prerequisites

- A Flipper Zero device
- Flipper Zero firmware repository cloned locally
- Basic familiarity with the command line
- `fbt` (Flipper Build Tool) installed and configured

### Step-by-Step Installation

#### 1. Clone the Flipper Zero Firmware

If you haven't already, clone the Flipper Zero firmware repository:

```bash
git clone https://github.com/flipperdevices/flipperzero-firmware.git
cd flipperzero-firmware
```

#### 2. Symlink the `applications_user` Folder

Navigate to the root of your Flipper Zero firmware repository and create a symlink to the `applications_user` folder in this Cosmos repository:

```bash
# Navigate to the firmware root
cd /path/to/flipperzero-firmware

# Create a symlink to the Cosmos applications_user folder
ln -s /path/to/cosmos/applications_user applications_user
```

Replace `/path/to/cosmos` with the actual path to this Cosmos repository on your system.

#### 3. Build the Application

With the symlink in place, you can now build the Cosmos application using the Flipper Build Tool (`fbt`). Navigate to the firmware root and run:

```bash
cd /path/to/flipperzero-firmware
./fbt launch APPSRC="cosmos"
```

This command will:
1. Build the Cosmos application
2. Launch it on your connected Flipper Zero device

#### 4. (Optional) Building for Multiple Firmwares

If you want to build the Cosmos application for multiple firmware versions, you can create symlinks to the `applications_user` folder in each firmware repository. This allows you to maintain a single source of truth for the Cosmos application while building it for different firmware versions.

For example:

```bash
# Symlink to another firmware repository
ln -s /path/to/cosmos/applications_user /path/to/another-firmware/applications_user

# Build for the second firmware
cd /path/to/another-firmware
./fbt launch APPSRC="cosmos"
```

### Troubleshooting

- **Symlink Issues**: Ensure that the symlink is correctly pointing to the `applications_user` folder in the Cosmos repository. You can verify this by running `ls -la` in the firmware root and checking the symlink target.

- **Build Errors**: If you encounter build errors, ensure that all dependencies are installed and that the `fbt` tool is correctly configured. Refer to the [Flipper Zero documentation](https://docs.flipper.net/) for more information.

- **Permission Issues**: If you encounter permission issues when creating symlinks, ensure that you have the necessary permissions to modify the firmware repository.

### Notes

- The `APPSRC="cosmos"` parameter in the `fbt launch` command specifies the source folder for the application. In this case, it refers to the `cosmos` folder within the `applications_user` directory.

- Ensure that your Flipper Zero device is connected and recognized by your system before running the `fbt launch` command.

### License

This project is licensed under the MIT License. See the [LICENSE](LICENSE) file for details.

### Contributing

Contributions are welcome! Please open an issue or submit a pull request for any improvements or bug fixes.

### Support

For support, please open an issue on the GitHub repository or refer to the [Flipper Zero documentation](https://docs.flipper.net/).
