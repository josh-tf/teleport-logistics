# Release acceptance: TeleportLogistics 0.5.12

Automated results are recorded in the release audit, which ships in the source and publication kit archives rather than in this tree. Historical reports apply only to their stated builds. Startup was confirmed after the 0.5.5 fix; reward and UI screenshots ship with the source archive.

Record pass/fail, game build, mod version, test mode, save name and evidence for each row. These retail checks are **pending**, not silently inferred from SDK tests.

| Priority | Test | Expected result |
| --- | --- | --- |
| Required | Launch fresh normal-progression map and previous test save | No startup, shader, missing-import or GC crash; TeleportLogistics appears in mod list. |
| Required | Tier 5 unlock paid normally, save/reload | Correct resource identities/counts; five 3D building rewards; custom milestone tile; recipes stay unlocked. |
| Required | Tier 9 unlock paid normally, save/reload | Correct 200/100/50 costs; Personnel reward and recipe; custom tile. |
| Required | Place/use/rotate/dismantle all six buildings | E opens correct panel; connectors and placement arrow align; removed nodes disappear. Dismantle is refused while a transfer is in flight, and for a fluid endpoint while its buffer or attached pipe still holds fluid. |
| Required | Item 2-to-3 route with mixed cargo, blocked output and recovery | Conservation, FIFO/state retention, fair available output shares; no loss/duplication. |
| Required | Water route, second fluid/gas, full pipes, flush/cancel | Fluid separation, conservation/backpressure, explicit local-only flush. |
| Required | Save/reload with cargo, routes and custom names/icons | State persists; no duplicate IDs/markers; old names remain valid. |
| Required | Hub powered/unpowered/repaint | Correct lights, management lock; logistics continue without hub power. |
| Required | Personnel ready/busy/unpowered/blocked/deleted | Correct rejection, 30-second reservation, directory cleanup. |
| Required | Personnel travel near/far, rotated destination, repeat after cooldown | Native transition completes, safe landing, intended facing, no stuck controls. |
| Required | Remote client plus listen and Windows dedicated host | Both directions of travel, late join, directory, concurrent edits, distant endpoints and save/reload. |
| Required | 1080p/1440p, high UI scale, Escape while editing | No overlap/clipping, readable title-case labels, close affects only its own panel. |
| Required | Sign picker and map | Icons preserve aspect ratio; selection persists; separate Logistics/Personnel sections; no white map squares. |
| Visual | Model front/rear/oblique, paint and LOD distance | Connector backs closed, no floating details, accents remain restrained, no shader crash. |
| Performance | Representative 100/1,000 endpoint factory | Record scheduler time and UI traffic; do not claim benchmarked scalability without measurements. |
| Optional platform | Linux dedicated server, controller, Experimental | Separate build/test evidence needed; currently not certified. |

Use a standard progression save for unlock costs; Creative Mode can hide them. Keep a test-save backup. Supply both client/server logs for multiplayer defects. Retest the affected rows after any fix; release automation passing is not a substitute for this matrix.
