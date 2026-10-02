---
title: Windows
description: Run or build Crescent TUI on Windows x64.
sidebar:
  order: 3
---

:::caution
The Windows build is marked **not tested** in the v0.1.0 release. Expect rough edges and please report issues on [GitHub](https://github.com/ba-sheer/crescenttt/issues).
:::

## Prebuilt package

No compiler or libraries are needed. Extract `crescent-tui-windows-x64.zip` and run:

```powershell
crescent-tui.exe
```

Keep the bundled DLLs next to `crescent-tui.exe`.

## Build from source

### Requirements

- Windows x64 with an **MSYS2 MinGW-w64** environment
- MinGW-w64 GCC
- PDCurses
- libcurl
- GNU Make

Install the toolchain and libraries inside MSYS2, then build:

```bash
git clone https://github.com/ba-sheer/crescenttt.git
cd crescenttt

make windows
```

The `windows` target runs:

```text
x86_64-w64-mingw32-gcc -std=c11 -Wall -Wextra -O2 -Iinclude src/*.c -o crescent-tui.exe -lcurl -lpdcurses -lws2_32
```

To remove the result:

```bash
make clean-windows
```

:::note
`x86_64-w64-mingw32-gcc` must be on your `PATH`. Windows builds use `<curses.h>` (PDCurses) instead of `<ncurses.h>`, selected automatically by `curses_compat.h`.
:::

## Configuring your API key on Windows

The config path is built from `XDG_CONFIG_HOME` or `HOME`. Windows does not normally set `HOME`, so the config file may end up in a `.config\crescent-tui` folder relative to the directory you launch the app from.

The most reliable option is the environment variable:

```powershell
$env:CRESCENT_API_KEY = "YOUR_API_KEY"
.\crescent-tui.exe
```

To set it permanently:

```powershell
setx CRESCENT_API_KEY "YOUR_API_KEY"
```

(Open a new terminal afterwards.) See [API Key](../../getting-started/api-key/) for more.

## Tips

- Use **Windows Terminal** for the best colour and key handling.
- If the app reports a missing DLL, confirm all DLLs from the package sit beside the `.exe`.