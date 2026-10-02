---
title: Configuration
description: Where Crescent TUI reads and stores its settings.
sidebar:
  order: 2
---

Crescent TUI has two settings: your **API key** and your shop **region**.

## Config file

The config file is a plain `key=value` text file:

```text title="~/.config/crescent-tui/config"
api_key=YOUR_API_KEY
region=US
```

| Key | Default | Description |
| :-- | :-- | :-- |
| `api_key` | _(empty)_ | Your Crescent API key (max 127 characters) |
| `region` | `US` | Region used for shop prices |

### Location

| Condition | Path |
| :-- | :-- |
| `XDG_CONFIG_HOME` is set | `$XDG_CONFIG_HOME/crescent-tui/config` |
| Otherwise | `~/.config/crescent-tui/config` |

If `HOME` is not set, the path falls back to `./.config/crescent-tui/config` relative to the current directory.

When the app saves the file it creates the directory if needed and sets the file mode to `600` (owner read/write only).

Lines without an `=` are ignored. There is no comment syntax.

## Environment variable

```bash
export CRESCENT_API_KEY="YOUR_API_KEY"
crescent-tui
```

`CRESCENT_API_KEY` **overrides** the `api_key` value in the config file when both are set.

:::caution
If you use the environment variable and then change the shop region (`R` in the Shop view), the app saves the config file. That file will then contain the key currently in use, including one that came from the environment variable.
:::

## Region

The region controls which price column the [Shop](../../usage/orders/#shop) view displays. Press `R` in the Shop view to cycle through the regions provided by the shop feed. The choice is saved to the config file.

If the feed has not loaded yet, the app cycles through a built-in fallback list: `US`, `EU`, `UK`, `IN`, `CA`, `AU`, `XX`.

## API endpoint

The API base URL is fixed in the source as `https://crescent.hackclub.com` and is not configurable at runtime.