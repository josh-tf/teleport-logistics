# Multiplayer and dedicated servers

## Install the same version everywhere

The host and every client must run the **same mod version**. A mismatch is the single most common cause of odd behaviour, and it will not always announce itself clearly.

## Supported targets

| Target | State |
| --- | --- |
| Windows client, Steam | supported |
| Windows client, Epic | supported |
| Windows dedicated server | supported |
| Linux dedicated server | not currently certified |

The release ships a Windows client package and a Windows dedicated-server package. The combined archive on the mod page contains both; a server needs the server half.

Linux dedicated servers are untested rather than known-broken. If you try one, a report either way is genuinely useful.

## How it behaves in multiplayer

The host owns the network. Route assignments, channel membership and transport all happen on the host and replicate to clients, so what you see in a window is the host's state.

Actions taken from a client, such as naming a route, enabling an endpoint, taking buffered items or flushing a fluid buffer, are sent to the host to perform. A client cannot desynchronise the network by clicking faster than the host can answer.

Map markers are created by the host, so every player sees the same endpoints on the map.

## Mod settings are per player

The settings under **Mods** in the pause menu are **client-side only**. They cover interface preferences and nothing else:

- **Window refresh interval**: how often an open window re-reads the network.
- **Show route when looking at a building**: the channel and route line in the look-at panel.
- **Confirm before flushing fluid**: the confirmation step on a fluid flush.

Deliberately absent: anything that affects transport rates, power draw, the route ceiling or travel timing. Those are compile-time constants precisely because SML writes settings per client. Exposing a gameplay number would let one player's client disagree with the host about how the world works.

## Dedicated server setup

Install the server package into the server's mods directory alongside SML, exactly as for any other SML mod, then install the matching client package for everyone connecting.

Nothing in the mod needs server-side configuration. There are no server config files to edit.

## Before you commit a save

This is a pre-1.0 release and full retail multiplayer, progression and save-reload acceptance is still being completed. **Keep a backup of any save you care about.**
