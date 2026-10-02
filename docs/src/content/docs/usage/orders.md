---
title: Orders
description: View your shop orders and browse the shop catalogue.
sidebar:
  order: 2
---

## Orders

Open **Orders** from the sidebar to see every order on your account. Each row shows:

| Column | Description |
| :-- | :-- |
| Status | Order status (colour-coded) |
| Item | Item name |
| Quantity | Shown as `xN` |
| Price | Total price in stars |
| Label | Human-readable status label from the API |

Select an order to see a detail line underneath the list:

- Order number
- Unit price in stars
- Placed date and fulfilled date (`-` if not fulfilled yet)
- **Tracking** carrier and URL, when shipped

Press `r` to refresh. If you have no orders, the view says "No orders yet."

:::note
The TUI cannot place or cancel orders. It only displays them.
:::

## Shop

Open **Shop** from the sidebar to browse the catalogue. This is the only view backed by an endpoint that does not require an API key.

The top line shows the active **region** and the currency conversion (for example `1 = $0.50`). Each row shows:

| Column | Description |
| :-- | :-- |
| Name | Item name |
| Type | Item type |
| Price | Price in stars for the active region, with an approximate dollar value, or `n/a` |
| Stock | `in stock` or `out of stock` (greyed out) |
| `(1/user)` | Appears when the item is limited to one per user |

Select an item to see its description and URL underneath. Items with grant rules attached show a note.

Press `R` to cycle regions. The choice is saved to your [config file](../../getting-started/configuration/#region).