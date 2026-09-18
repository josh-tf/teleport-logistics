# Publication kit: Teleport Logistics 0.5.10

Prepared for review, not uploaded. Read the [release audit](../../reports/0.5.10-release-audit.md) and complete [retail acceptance](../validation.md) before presenting this as a tested public release.

## Listing fields

| Field | Prepared value |
| --- | --- |
| Name | **Teleport Logistics** |
| Mod reference | **TeleportLogistics**, checked free on SMR. Must match the plugin and native module |
| Short description | Named teleport routes for items and fluids, plus powered personal destinations for your Pioneer. |
| Full description | [listing.md](listing.md), Markdown paste, after replacing the `{{IMG_*}}` placeholders below |
| Version notes | [release-notes.md](release-notes.md) |
| Icon | [media/icon-512.png](media/icon-512.png) |
| Showcase | [media/banner.png](media/banner.png), a studio-render composition, not an in-game screenshot |
| Suggested searchable terms | teleport, teleporter, logistics, routes, network, portal, items, fluids, transport |
| Upload archive | `dist/TeleportLogistics-0.5.10.zip`, combined Windows client/server, built and listed in `dist/SHA256SUMS` |
| Dependency | SML `^3.12.0`; game build `>=502094` in manifest |
| Author / support contact | Support via GitHub issues: `https://github.com/josh-tf/teleport-logistics/issues` |
| Source URL / code license | `https://github.com/josh-tf/teleport-logistics`, repository created and **not yet pushed**. Code license MIT |
| Wiki | `https://github.com/josh-tf/teleport-logistics/wiki`, linked from the description. Pages must exist before the listing goes live |

Do not upload the source archive or model viewer as the mod version. Do not mark Experimental, Linux dedicated-server or controller compatibility as tested without evidence. A manifest's minimum game build is not proof of compatibility with every newer build.

The title is separate from the immutable mod reference, and the official upload guide requires the reference to
match the plugin and native module. `TeleportLogistics` was verified free against the SMR API. Do not switch the
reference to `Teleporter`: it belongs to an established player-teleport mod (`Bk37KmQuPNpDvK`, 333k downloads,
last released 2024-06-30). SMR references cannot be changed after the page is created.

Naming decisions inside the package: the mod is **Teleport Logistics**, and the buildings keep their own nouns
because they are teleporters within that system. The milestones are **Teleport Logistics** (Tier 5) and
**Teleport Personnel Transport** (Tier 9).

Building names are short, because the build menu clamps a tile label to two lines and wraps on spaces, which
truncated "(Input)" and left the input and output tiles reading identically. A two-tier scheme was tried first and
does not work: `UFGBuildingDescriptor` overrides `GetItemNameInternal` to return `mBuildableClass`'s `mDisplayName`,
so a short label set on the descriptor is never read, and `mAbbreviatedDisplayName` is not what the tile uses
either. The name therefore lives on the buildable, where one string serves the tile, the look-at panel, the use
prompt, the hologram and the dismantle UI alike: `Item (In)`, `Item (Out)`, `Fluid (In)`, `Fluid (Out)`,
`Teleporter Hub`, `Personnel Teleporter`. The descriptive form survives in the configure window's heading. Do not
move these back onto the descriptors.

## Media

The banner and six transparent building renders are ready to upload as artwork. They are renders of the authored meshes, so material appearance in the offline viewer differs from retail rendering. Keep the banner's “Model preview” label when using it.

### Screenshot placeholders

`listing.md` carries one placeholder image per screenshot slot, rendered as a small click-to-enlarge
thumbnail:

```html
<a href="URL"><img alt="..." src="URL" width="480"></a>
```

Both `URL`s are the same, so each screenshot needs one SMR upload and one find-and-replace. SMR renders
descriptions with `marked` plus DOMPurify on its default configuration, which keeps `<img width>` and
`<a href>`, and the [markdown help page](https://ficsit.app/help) documents raw HTML as supported. Keep
`width="480"` for a thumbnail, raise it for a shot that needs detail, and leave the `alt` text alone.

Until a real capture exists, each slot points at a labelled `placehold.co` box stating its number, the
section it sits in and what to shoot. The slot name rides in an ignored `&slot=IMG_*` parameter so the URL
stays searchable; the visible caption has to stay under about 60 characters or the service truncates it.
Those URLs must not ship: grep the description for `placehold.co` before pasting, and delete any slot you
decide not to fill rather than leaving the placeholder in.

| Slot | Capture |
| --- | --- |
| `IMG_ROUTE_OVERVIEW` | Hero shot: an Item Input fed by a belt in one factory and its Output delivering in another, both buildings readable. |
| `IMG_BUILDINGS` | All six Teleport Logistics buildings placed side by side, front three-quarter view, powered, with directional indicators visible. |
| `IMG_MILESTONE_T5` | HUB terminal, Tier 5 **Teleport Logistics** milestone tile: normal progression costs and the 3D reward models. |
| `IMG_MILESTONE_T9` | HUB terminal, Tier 9 **Teleport Personnel Transport** tile with its costs and reward. |
| `IMG_ENDPOINT_UI` | Endpoint window on an item endpoint: label, channel, route, enable switch and the local item buffer populated. |
| `IMG_CHANNEL_SELECT` | Channel/route picker open with the search field in use, showing an existing route and the create-a-route option. |
| `IMG_FLUID_ROUTE` | Fluid Input and Output connected by pipes, with a fluid endpoint window open showing its buffer. |
| `IMG_HUB_UI` | Powered Teleporter Hub window: channel list, route detail and the paginated endpoint directory. |
| `IMG_PERSONNEL_UI` | Personnel destination directory with sign icons, at least one ready and one unpowered destination. |
| `IMG_MAP` | Map open with the separate Logistics and Personnel categories both populated. |

### Wiki pages the description links to

The description links out to the repository wiki for detail that does not belong on a store page.
Each page must exist before the listing is published, or the links will dead-end:
`First-Route`, `Buildings`, `Channels-and-Routes`, `Hubs`, `Personnel-Teleporters`,
`Throughput-and-Power`, `Multiplayer-and-Dedicated-Servers`, `FAQ`.
The changelog and issue links resolve only once the repository has been pushed.

Historical bug screenshots are not publication screenshots.

## Upload sequence

1. Finish the acceptance checklist, choose author/contact/source policy, and inspect the final copy/media.
   Push the repository, create the wiki pages listed above, and replace every `placehold.co` URL with an
   uploaded image link before the description is pasted.
2. Sign in to ficsit.app and create/edit the page using the fields above. `TeleportLogistics` was free when checked against the SMR API during this pass; confirm it is still free at creation time.
3. Upload icon and media through SMR, then insert the resulting hosted image links into the description. Local repository paths will not display on the public page.
4. Upload the combined archive with the supplied release notes. Check dependency, platform and compatibility fields against the evidence.
5. Review the rendered page and complete the native-code approval process.

For the first stable public release, the official guidance recommends 1.0.0. This preparation pass stays on 0.5.10 with beta status until acceptance is complete; a later version promotion must be rebuilt, not renamed as a ZIP.

Sources: [SMR upload guide](https://docs.ficsit.app/satisfactory-modding/latest/UploadToSMR.html), [release and platform guidance](https://docs.ficsit.app/satisfactory-modding/latest/Development/BeginnersGuide/ReleaseMod.html). Retrieved during this preparation pass. The SDK remains pinned to the version actually built and tested.
