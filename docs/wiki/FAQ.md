# FAQ and known limitations

## Cargo is not moving

Check, in this order:

1. **Do both ends share a channel and a route?** Same channel, identical route name. This is almost always the answer.
2. **Are both endpoints enabled?** A disabled endpoint neither accepts nor sends.
3. **Is there anything to send?** The input needs a supplying belt or pipe with cargo on it.
4. **Can the output deliver?** If its buffer is full, the receiving belt cannot keep up. That is a factory problem, not a route problem.
5. **Fluid route type.** A fluid route carries one fluid at a time; sending a second type does nothing until the route is emptied.

## Why do Item (In) and Item (Out) look the same in the build menu?

They are colour-coded and carry different glyphs: mint for input, orange for output. The build menu clamps a tile label to two lines, which is also why the names are short.

## Can I filter what an output receives?

No. An output receives whatever the inputs on its route send. If you need separation, use separate routes. Items keep their order and state through a route, so a single mixed route into a sorting area works well.

## Is there a shared warehouse or storage?

No. The buffers are 64 items or 50 m³ and exist to smooth belt timing. Nothing is stored between sessions beyond what happens to be buffered.

## Can I change what a fluid route carries?

Yes, but the route must be empty. Every endpoint on it must be disabled and drained, then select the route in a powered Teleporter Hub and use **Reset empty fluid route**. Without a hub, move the endpoints to a new route name, which carries no fluid type. Drain connected pipes too, or the old fluid re-pins the route as soon as an endpoint is re-enabled.

## Does flushing a fluid buffer drain my pipes?

No. Flush discards **only** the endpoint's local buffer. Your pipe network is untouched. It asks for confirmation, and needs the endpoint disabled first so nothing refills it mid-flush.

## What happens to items in a buffer when I reassign a route?

Take them first. Item endpoints let you move buffered items into your own inventory, so nothing is destroyed. Drain before reassigning, or the old route's cargo is delivered to the new one.

## Do the endpoints need power?

No. Item and fluid endpoints have no electrical connection, so a brownout cannot strand your supply lines. Only the hub (5 MW) and the Personnel Teleporters (50 MW each end) draw power.

## Do I need a hub?

No. The Default channel works with no hub anywhere. Build one when your network is large enough that searching a list beats walking to the building, or when you want to rename, pause or delete routes: a hub is the only building that can do those, so a network that churns route names will eventually want one.

## My hub lost power. Did my factory stop?

No. Losing hub power disables the management interface, not logistics. Routes keep carrying cargo.

## Can Personnel Teleporters move items or vehicles?

No, pioneers only. Both ends need 50 MW at the moment of travel, and a journey reserves both teleporters for 30 seconds, so neither end will accept another traveller until the cooldown elapses.

## Which way do I face when I arrive?

The way the placement arrow points. Aim it somewhere useful when you build.

## Can I change transport rates or power costs in the settings?

No, and this is deliberate. SML writes settings per client, so a gameplay number in there would let one player's client disagree with the host. The settings are interface preferences only. See [Multiplayer and dedicated servers](Multiplayer-and-Dedicated-Servers).

## The interface costs me frames on a big network

Raise the **window refresh interval** under Mods in the pause menu. It controls how often an open window re-reads the network and affects nothing about transport.

## Known limitations

- No item filtering, no priority between outputs, no shared storage.
- Fluid routes carry one fluid type at a time.
- 256 routes maximum.
- Linux dedicated servers and controller input are not currently certified.
- Pre-1.0: full retail multiplayer, progression and save-reload acceptance is still in progress. Keep save backups.

## Reporting a problem

[Open a GitHub issue](https://github.com/josh-tf/teleport-logistics/issues) with your Satisfactory build, SML version, whether you are on a dedicated server, what you expected, what happened, and `FactoryGame.log` if the game misbehaved.
