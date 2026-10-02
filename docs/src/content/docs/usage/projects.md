---
title: Projects
description: Browse your projects, their status, ships and reviewer feedback.
sidebar:
  order: 1
---

## Navigating the app

The screen has a header, a sidebar menu on the left, a content pane, and a footer showing key hints and status messages.

The header shows your handle, star count and verification status.

| Key | Action |
| :-- | :-- |
| `↑` / `↓` or `k` / `j` | Move selection (menu or list) |
| `Enter`, `→` or `l` | Open: move from the menu into the content, or open a project |
| `Esc`, `Backspace`, `←` or `h` | Go back: to the list, or to the menu |
| `r` | Refresh the current view |
| `R` | Cycle the price region (Shop view only) |
| `?` | Toggle the help screen |
| `q` | Quit |

The app starts on the **Account** view with the sidebar focused. Each view loads its data the first time you open it.

:::note
If the terminal is smaller than 12 rows or 40 columns, the app shows "Terminal too small" until you resize it.
:::

## Account view

Shows your handle, Slack ID, stars, verification status, join date and avatar URL.

## Project list

Open **Projects** from the menu. Each row shows:

| Column | Description |
| :-- | :-- |
| Status | Current project status |
| Title | Project title (truncated to 28 characters) |
| Card | Name of the card the project uses, or `-` |
| Time | Tracked time as `XhYYm` |

Select a project and press `Enter` to open its detail view.

## Project detail

The detail view shows:

- Title and description
- Status
- Card name, multiplier (for example `x1.50`), a `wildcard` marker, its requirement and guide URL
- Repository, demo and project page URLs
- Tracked time, created date and approved date
- **Reviewer feedback**, if any
- **Ships**: each submission with its status, stars awarded (or `-` if not yet awarded), submission time and change description

Press `r` to refresh the project, or `Esc` / `Backspace` / `h` to return to the list.

## Status colours

Statuses are colour-coded across all views:

| Colour | Statuses |
| :-- | :-- |
| Green | `approved`, `fulfilled`, `verified`, `order_done`, `ship_approved`, `order_grant_sent` |
| Yellow | `under_review`, `pending`, `on_hold`, `awaiting_verification` |
| Red | `rejected`, `cancelled`, `expired`, `ineligible`, `forbidden` |
| Magenta | `needs_changes`, `ship_needs_changes` |
| Cyan | `draft` and anything unrecognised |