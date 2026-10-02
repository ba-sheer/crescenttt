---
title: Crescent TUI
description: A terminal dashboard for Crescent, built in C with ncurses and libcurl.
template: splash
hero:
  tagline: Check your projects, orders, notifications and announcements without leaving the terminal.
  actions:
    - text: Install
      link: /crescenttt/getting-started/installation/
      icon: right-arrow
    - text: View on GitHub
      link: https://github.com/ba-sheer/crescenttt
      icon: external
      variant: minimal
---

**Crescent TUI** is a terminal version of [Crescent](https://crescent.hackclub.com), built on the public Crescent API. It is written in C11 and uses `ncurses` (`PDCurses` on Windows) for the interface and `libcurl` for requests.

## What you can do

| Section | What it shows |
| :-- | :-- |
| **Account** | Your handle, Slack ID, stars, verification status and join date |
| **Projects** | Every project with its status, card and tracked time, plus a detail view with ships and reviewer feedback |
| **Orders** | Shop orders, their status, price in stars and tracking info |
| **Notifications** | Your latest notifications (up to 50), with unread markers |
| **Announcements** | Posts from the Crescent team |
| **Shop** | The shop catalogue with per-region pricing and stock |

:::note
The dashboard is **read-only** and only shows data for the account that owns your API key. Crescent's API has no endpoint for viewing or managing other participants.
:::

## Where to go next

1. [Install Crescent TUI](getting-started/installation/)
2. [Set up your API key](getting-started/api-key/)
3. [Learn the controls and views](usage/projects/)

## Requirements at a glance

- **Linux:** `gcc` (C11), `make`, `ncurses`, `libcurl`, Git
- **Windows x64:** nothing for the prebuilt package (DLLs included)
- A Crescent API key

:::caution
The Windows build is marked **not tested** in the v0.1.0 release notes. The Linux build is the primary target.
:::
