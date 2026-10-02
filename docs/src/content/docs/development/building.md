---
title: Building
description: Project layout, Makefile targets and how the code fits together.
sidebar:
  order: 1
---

## Project layout

```text
.
├── Makefile
├── include/        # headers
│   ├── api.h       # Crescent API client
│   ├── config.h    # config load/save
│   ├── curses_compat.h
│   ├── http.h      # libcurl wrapper
│   ├── json.h      # JSON parser
│   └── ui.h
├── src/
│   ├── api.c
│   ├── config.c
│   ├── http.c
│   ├── json.c
│   ├── main.c
│   └── ui.c        # ncurses dashboard
├── release/        # prebuilt binaries
└── docs/           # this Astro Starlight site
```

## Modules

| Module | Responsibility |
| :-- | :-- |
| `main.c` | Initialises curl, loads the config, runs the UI |
| `config.c` | Reads `config` and `CRESCENT_API_KEY`; saves with mode 600 |
| `http.c` | A single `http_get` over libcurl (bearer auth, 15 s timeout, follows redirects) |
| `json.c` | A small hand-written JSON parser with Unicode escape support |
| `api.c` | Maps endpoints and HTTP errors to an `api_result` |
| `ui.c` | Views, key handling, colours, dialogs |
| `curses_compat.h` | Includes `<curses.h>` on Windows and `<ncurses.h>` elsewhere |

## API endpoints used

All requests are `GET` against `https://crescent.hackclub.com`.

| Function | Endpoint |
| :-- | :-- |
| `api_get_me` | `/api/v1/me` |
| `api_list_projects` | `/api/v1/me/projects` |
| `api_get_project` | `/api/v1/me/projects/{id}` |
| `api_list_orders` | `/api/v1/me/orders` |
| `api_list_notifications` | `/api/v1/me/notifications?limit=N` (N clamped to 1–50) |
| `api_list_announcements` | `/api/v1/announcements` |
| `api_get_shop_items` | `/api/v1/shop/items` (no key) |

## Make targets

```bash
make                # Build for Linux (./crescent-tui)
make windows        # Cross/native build for Windows (crescent-tui.exe)
make clean          # Remove Linux build files
make clean-windows  # Remove the Windows build
sudo make install   # Install to /usr/local/bin/crescent-tui
```

The build uses `-std=c11 -Wall -Wextra -O2 -Iinclude`. Linux link flags come from `pkg-config` for `ncurses` and `libcurl`, falling back to `-lncurses -lcurl`. `CC` and `CFLAGS` can be overridden from the environment.

`make install` honours `DESTDIR`, which is useful for packaging:

```bash
make install DESTDIR=/tmp/pkgroot
```

## Continuous integration

Every push and pull request runs the **Build** workflow (`.github/workflows/c-cpp.yml`) on Ubuntu. It installs `gcc make libncurses-dev libcurl4-openssl-dev`, runs `make`, and checks that `crescent-tui` exists and is executable. Only Linux is built in CI.

## Documentation site

The docs live in `docs/` and use [Astro Starlight](https://starlight.astro.build). Pages are Markdown files in `docs/src/content/docs/`.

```bash
cd docs
npm install
npm run dev      # http://localhost:4321/crescenttt
npm run build    # outputs to docs/dist
npm run preview
```

The **Deploy Docs** workflow builds the site with Node 22 and publishes it to GitHub Pages on every push to `main`. The site is configured with `base: '/crescenttt'`.