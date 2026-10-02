---
title: Linux
description: Build and package Crescent TUI on Linux.
sidebar:
  order: 2
---

## Dependencies

| Tool / library | Purpose |
| :-- | :-- |
| GCC or another C11 compiler | Compiling |
| GNU Make | Build system |
| `ncurses` (dev headers) | Terminal UI |
| `libcurl` (dev headers) | HTTP requests |
| Git | Cloning |
| `pkg-config` (optional) | Finds link flags; the Makefile falls back to `-lncurses -lcurl` without it |

Install them with your package manager:

```bash title="Fedora"
sudo dnf install gcc make ncurses-devel libcurl-devel
```

```bash title="Debian / Ubuntu"
sudo apt install gcc make libncurses-dev libcurl4-openssl-dev
```

```bash title="Arch Linux"
sudo pacman -S gcc make ncurses curl
```

## Build and run

```bash
git clone https://github.com/ba-sheer/crescenttt.git
cd crescenttt
make
./crescent-tui
```

For a fresh rebuild:

```bash
make clean && make
```

## Install

```bash
sudo make install
```

This copies the binary to `/usr/local/bin/crescent-tui`.

## Using a different compiler

```bash
make CC=clang
```

## Troubleshooting

| Symptom | Fix |
| :-- | :-- |
| `ncurses.h: No such file or directory` | Install the ncurses **development** package (`ncurses-devel` or `libncurses-dev`) |
| `curl/curl.h: No such file or directory` | Install the libcurl **development** package |
| Garbled borders or colours | Use a terminal that supports colour and a UTF-8 locale; check `echo $TERM` |
| "Terminal too small" | Enlarge the window to at least 40 columns by 12 rows |
| Config not found | See [Configuration](../../getting-started/configuration/) for the lookup order |