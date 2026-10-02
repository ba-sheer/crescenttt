---
title: Notifications
description: Read your latest Crescent notifications in the terminal.
sidebar:
  order: 3
---

Open **Notifications** from the sidebar to see your most recent notifications. The app requests up to **50** at a time (the API accepts 1–50).

## List

Each row shows:

| Column | Description |
| :-- | :-- |
| `*` | Marks an **unread** notification |
| Kind | The notification type, colour-coded using the [status colours](../projects/#status-colours) |
| Title | Notification title (truncated to 30 characters) |
| Time | When it was created, as `YYYY-MM-DD HH:MM` |

## Detail

Move the selection with `↑` / `↓`. The body of the selected notification and its link (if it has one) appear below the list.

## Refreshing

Press `r` to fetch the latest notifications. If there are none, the view says "No notifications."

:::note
Notifications are read-only in the TUI. Viewing one here does not change its read state.
:::