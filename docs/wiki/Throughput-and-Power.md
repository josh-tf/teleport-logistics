# Throughput and power

## The numbers

| | Ceiling | Local buffer | Power |
| --- | --- | --- | --- |
| Item endpoint | 1,200 items/min | 64 items | none |
| Fluid endpoint | 600 m³/min | 50 m³ | none |
| Teleporter Hub | not applicable | none | 5 MW |
| Personnel Teleporter | not applicable | none | 50 MW at each end |

These are **configured ceilings per endpoint**, not measured rates and not promises. What you actually get is whichever of these is smallest:

- the ceiling above,
- what your connected belts or pipes can carry,
- what the supply side can produce,
- what the receiving side can take away.

A Mk.6 belt delivering 1,200 items/min into an input can saturate it. A Mk.1 belt cannot, and no setting will change that.

## Why endpoints need no power

Item and fluid endpoints have no electrical connection at all. This is a deliberate design choice: your logistics should not fail because of a brownout somewhere else. Only the optional hub and the Personnel Teleporters draw power.

## What the buffers are for

Each endpoint holds a small local buffer of 64 items or 50 m³. They exist to smooth the timing mismatch between a belt arriving in bursts and a route sending continuously.

They are **not storage**. A buffer that sits full tells you something: at an output, the receiving belt cannot keep up; at an input, the route has nowhere to put the cargo.

## Merging and sharing

Several inputs on one route merge their cargo into it. Several outputs on one route share whatever is available, and an output that is backed up is skipped so it cannot stall the others.

There is no priority system and no filtering. If you need one output to win, give it its own route.

## Scaling up

The network supports up to **256 routes**. Past that, consolidate: one route carrying mixed cargo to a sorting area is usually better than many thin routes, because items keep their order and state through a route.

For a large network, a [hub](Hubs) pays for its 5 MW in search time alone.

## Interface cost

If a very large network makes the endpoint or hub window cost you frames, raise the **window refresh interval** in the mod settings. See [FAQ](FAQ). It changes how often an open window re-reads the network, and nothing about transport.
