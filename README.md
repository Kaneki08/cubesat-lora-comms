# HopeRF LoRa Firmware for ESP32-C3

This repository contains firmware for testing and developing communication with a HopeRF LoRa module using the WeAct Studio ESP32-C3 Core Board. The project uses PlatformIO, the Arduino framework, and the RadioLib library.

## Project Overview

This project is intended for bring-up and development of CubeSat communication firmware. The current codebase focuses on transmitting data over a HopeRF LoRa radio from an ESP32-C3 board.

## Required Software

Install the following before doing anything else:

1. Visual Studio Code
2. PlatformIO IDE extension for VS Code
3. Git

Important: install the PlatformIO IDE extension before opening, building, or uploading this project. Without it, VS Code will not recognize the firmware environment correctly.

## Installing PlatformIO in VS Code

1. Open VS Code.
2. Open the Extensions view with `Cmd+Shift+X`.
3. Search for `PlatformIO IDE`.
4. Select the extension published by PlatformIO.
5. Click Install.
6. Restart VS Code if prompted.

## Opening the Project

Open the folder that contains `platformio.ini`. In this repository, that is the project root.

## Building the Project

### Using VS Code

1. Open the Command Palette with `Cmd+Shift+P`.
2. Run `PlatformIO: Build`.

### Using the CLI

```bash
platformio run
```

## Uploading Firmware

Connect the WeAct Studio ESP32-C3 board over USB before uploading.

### Using VS Code

1. Open the Command Palette with `Cmd+Shift+P`.
2. Run `PlatformIO: Upload`.

### Using the CLI

```bash
platformio run --target upload
```

## Serial Monitor

Use the serial monitor to view debug output from the board.

### Using VS Code

1. Open the Command Palette with `Cmd+Shift+P`.
2. Run `PlatformIO: Monitor`.

### Using the CLI

```bash
platformio device monitor
```

The firmware currently uses baud rate `115200`.

## GitHub Workflow for Team Collaboration

Clone the repository:

```bash
git clone https://github.com/Kaneki08/cubesat-lora-comms.git
cd cubesat-lora-comms
```

Create a feature branch:

```bash
git checkout -b feature/short-description
```

Commit changes:

```bash
git status
git add .
git commit -m "Add short description of change"
```

Push your branch:

```bash
git push origin feature/short-description
```

Pull the latest `main` updates:

```bash
git checkout main
git pull origin main
```

## Project Structure

```text
project-root/
|
+-- src/
|   +-- HopeRFTX.cpp
|
+-- include/
|   +-- README
|
+-- lib/
|   +-- README
|
+-- test/
|   +-- README
|
+-- platformio.ini
+-- README.md
+-- .gitignore
```

## What Each Folder Is For

- `src/`: application source files.
- `include/`: project header files.
- `lib/`: local project-specific libraries if needed later.
- `test/`: tests for firmware code.
- `platformio.ini`: board target, framework, and dependency configuration.
- `README.md`: setup and contributor guide.
- `.gitignore`: files and folders that should not be committed.

## Dependency Management

Dependencies are managed through `platformio.ini`.

Current dependency:

```ini
lib_deps =
    jgromes/RadioLib@^7.6.0
```

PlatformIO installs these automatically during build. Do not manually copy external libraries into the repository unless there is a specific reason to vendor them.

## Contributors  
Derick Barrientos  
Ibrahim Hussein  
Jonathan Lin  
