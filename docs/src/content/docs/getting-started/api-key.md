---
title: API Key
description: How to get a Crescent API key and give it to Crescent TUI.
sidebar:
  order: 3
---

Crescent TUI needs a **Crescent API key** to read your account data.

## Get a key

On the Crescent website, open the account menu and choose **API keys**, then create a key.

:::tip
Treat the key like a password. Anyone with it can read your account's data through the API.
:::

## Give the key to the app

Crescent TUI looks for the key in this order:

1. The `CRESCENT_API_KEY` environment variable (highest priority)
2. The `api_key` entry in `~/.config/crescent-tui/config`
3. An interactive prompt on startup

### Interactive prompt

If no key is found, a dialog appears asking you to paste one. Your input is masked with `*`.

1. Paste the key and press **Enter**.
2. The app asks whether to save it to `~/.config/crescent-tui/config` (mode 600). Press `y` to save or `n` to use it for this session only.

Pressing **Esc** cancels the prompt. The app still opens, but requests will fail with an *unauthorized* error until a key is provided.

## Limits and errors

The API allows **60 requests per minute per key**. The app shows the following messages in the footer bar:

| HTTP status | Message | Meaning |
| :-- | :-- | :-- |
| 401 | unauthorized | No key, or the key is wrong or revoked |
| 403 | forbidden | The account is banned |
| 404 | not found | The requested item does not exist |
| 429 | rate limited | Over 60 requests/min; the message shows how long to wait |
| 503 | API switched off | Crescent's API is temporarily disabled |
| – | network error | Could not reach the server (15 s timeout) |

Requests are sent with an `Authorization: Bearer <key>` header. The shop catalogue is the only endpoint that does not need a key.

## Replacing a key

Edit `~/.config/crescent-tui/config`, or unset `CRESCENT_API_KEY`, then restart. See [Configuration](../configuration/) for details.