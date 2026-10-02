---
title: Installation
description: Install Crescent TUI on Linux or Windows from a prebuilt binary or from source.
sidebar:
  order: 1
---

You can use a prebuilt binary or build from source. Prebuilt binaries are also attached to the [v0.1.0 release](https://github.com/ba-sheer/crescenttt/releases/tag/v0.1.0).

## Linux

### Prebuilt binary

The repository ships an x86-64 Linux binary.

```bash
git clone https://github.com/ba-sheer/crescenttt.git
cd crescenttt

tar -xzf release/crescent-tui-linux-x64.tar.gz
cd linux-x64

chmod +x crescent-tui
./crescent-tui
```

To install it system-wide:

```bash
sudo install -Dm755 crescent-tui /usr/local/bin/crescent-tui
```

Then run it from anywhere:

```bash
crescent-tui
```

### Build from source

Install the dependencies for your distribution:

```bash title="Fedora"
sudo dnf install gcc make ncurses-devel libcurl-devel
```

```bash title="Debian / Ubuntu"
sudo apt install gcc make libncurses-dev libcurl4-openssl-dev
```

```bash title="Arch Linux"
sudo pacman -S gcc make ncurses curl
```

Then build:

```bash
git clone https://github.com/ba-sheer/crescenttt.git
cd crescenttt

make
./crescent-tui
```

To install the binary to `/usr/local/bin/crescent-tui`:

```bash
sudo make install
```

## Windows

:::caution
The Windows build is **not tested** (per the v0.1.0 release notes). Please report any problems you hit.
:::

### Prebuilt package

The Windows x64 package includes all required DLLs.

1. Get `crescent-tui-windows-x64.zip` from the [releases page](https://github.com/ba-sheer/crescenttt/releases) (or `release/` in the repository, if present).
2. Extract it.
3. Run it:

```powershell
crescent-tui.exe
```

:::danger
Keep the DLL files in the **same directory** as `crescent-tui.exe`, or it will not start.
:::

### Build from source

See [Building from source](../../development/building/) and [Windows](../../development/windows/).

## First run

Start the app. If no API key is configured, it asks for one. Continue with [Getting an API key](../api-key/).