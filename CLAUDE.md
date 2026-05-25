# CLAUDE.md

## What this is

A VirtualDJ 8 plugin that extends VDJ's built-in quickfilters with auto-refresh and metadata-driven filtering. Used during **live DJ performances** — a plugin crash takes down VirtualDJ mid-set, so changes need to favor stability over cleverness.

## Goals

1. **Auto-refresh on master deck track change.** When the master deck swaps or a new track is loaded into the master deck, the plugin should disengage and re-engage all active quickfilters so they re-evaluate against the new track.
2. **Metadata-driven filtering.** Parse hidden `#tags` from the master track's comment field and expose richer filter semantics than VDJ ships with. First target: **numeric adjacency** — a tag like `#energy03` carries the value 3, and the filter should also match adjacent values (e.g. 2 and 4), with the window width configurable per filter.

Current state is early: `MyPlugin8.cpp` declares the toggle buttons but the auto-refresh trigger and the metadata reader are not yet implemented.

## Platform

VirtualDJ runs on macOS and Windows only. This project targets **macOS arm64** (Apple Silicon). Builds produce a `.bundle`.

## Build workflow

Development happens on Linux; the actual build runs on a Mac over SSH. The Mac host is configured in `justfile`.

```bash
just deploy   # rsync to mac-host, build there, install bundle — the main loop
just sync     # rsync only, no build
```

On the Mac directly: `just build` / `just install`.

## Plugin shape

The VDJ SDK in `external/sdk/` is a COM-style C++ interface. `Main.cpp` exports `DllGetClassObject`, which returns a `CMyPlugin8` derived from `IVdjPlugin8` (see `vdjPlugin8.h` for the full interface and the host callbacks like `SendCommand`, `GetInfo`, `GetStringInfo`).

**Adding a button** — three places to touch:

1. `ID_BUTTON_N` entry in the enum in `MyPlugin8.h` — enum order is the UI display order. **Keep the `ID_BUTTON_N` naming pattern strictly sequential** (`ID_BUTTON_1`, `ID_BUTTON_2`, …); do not rename to semantic identifiers like `ID_ENERGY_BASE`.
2. `int m_*` state field on `CMyPlugin8`.
3. In `OnLoad`, init the field to `0` and call `DeclareParameterButton(&m_field, ID, "Long Name", "ShortName")`. Add the matching branch in `OnParameter` checking `id == ID && m_field == 1`.

**Mapping handles.** VDJ mappings reference plugin parameters two ways:
- By 1-based position in the enum: `plugin "QFPro" 2` → `ID_BUTTON_2`.
- By short name: `plugin "QFPro" "ShortName"`.

Because the numeric form is the common one users will type, the `ID_BUTTON_N` identifier number must match the 1-based mapping number — that's why the sequential naming convention is load-bearing, not cosmetic.

## Current button layout & behavior

102 buttons total: 2 control + 4 tag banks × 25. Declared in `MyPlugin8.cpp::OnLoad` in UI order = mapping number.

| Buttons | Group | Behavior |
|---|---|---|
| 1 | `RFR` "Refresh Quick Filters" | (TODO) disengage + re-engage active filter against the master deck. |
| 2 | `Kill` "Kill Quick Filter" | Sends `quick_filter off`, clears `m_ActiveFilter` cache, calls `Clear()` on every bank. |
| 3–27 | Energy bank (`E>=N.N`, `E=<N.N`, `E+N.N`, `E0`, `E-N.N`) | Range 3–14, Stack 15–27. |
| 28–52 | Happy bank (`H...`) | Range 28–39, Stack 40–52. |
| 53–77 | Dance bank (`D...`) | Range 53–64, Stack 65–77. |
| 78–102 | Pop bank (`P...`) | Range 78–89, Stack 90–102. |

Each bank targets one numeric tag in the master deck's comment (e.g. `#03Energy`), parsed by `GetNumericTag(tagName)`. Within a bank, range and stack are mutually exclusive; across banks, contributions are AND-joined into a single `quick_filter` expression.

### TagBank (struct in MyPlugin8.h)

State-only, no SDK calls. Each instance owns:
- `tagName` — e.g. "Energy".
- `rangeBtns[12]`, `stackBtns[13]` — SDK-owned momentary press state.
- `posPeakHs`, `negPeakHs` — current range peak per side, in half-steps (0..6).
- `stackEngaged[13]` — per-button latch for stack mode.

Methods: `Init`, `Clear`, `HandleRangePress(idx)`, `HandleStackPress(idx)`, `AnyRangeEngaged`, `AnyStackEngaged`, `AnyEngaged`, `ComputeDeltas()` (returns the union of half-step offsets relative to master).

### Range mode (12 buttons per bank, no `=0`)

- Press `>=N` → `posPeakHs = N*2`, overwriting any prior positive peak. Contribution: half-steps `1..posPeakHs` (= master+0.5 .. master+N). Master itself is NOT included.
- Press the current positive peak again → `posPeakHs = 0`, positive side cleared. Negative side untouched.
- Same rules mirrored for `=<N` / `negPeakHs`.
- There is no range `=0` button — the stack `0` button is the only way to engage just the master value.

### Stack mode (13 buttons per bank, includes `0`)

Per-button latch in `stackEngaged[i]`. Each engaged button contributes only its single half-step value (not a range). Press toggles. The `0` button contributes the master value itself.

### Mutual exclusion (within a bank)

`HandleRangePress` clears `stackEngaged[]` first if any stack is on. `HandleStackPress` clears `posPeakHs`/`negPeakHs` first if any range is on. The other banks are untouched.

### Combining across banks (AND)

`CMyPlugin8::RebuildFilter`:
1. Iterates `m_banks`. For each engaged bank, reads its master value via `GetNumericTag(bank.tagName)`, asks for its `ComputeDeltas()`, formats each delta into `Comment has tag #<value><tagName>` via `formatTagValue` + `commentTagClause`, joins with `or` via `buildOrExpression`.
2. AND-joins all per-bank expressions: `(bankA OR-clauses) and (bankB OR-clauses)`.
3. Dispatches via `SendFilterExpression`.

If no bank is engaged → re-sends the cached filter to clear and empties the cache.

### Active-filter tracking

`m_ActiveFilter` caches the last `quick_filter '<expr>'` string sent. To swap filters, `SendFilterExpression` re-sends the cached string (VDJ toggles same-expression off), then sends the new one. Same-expression re-send disengages without replacing. `quick_filter off` alone did **not** reliably clear custom expressions in testing, hence the cache-based dance.

### Adding more banks

1. Bump `CMyPlugin8::BANK_COUNT` in `MyPlugin8.h`.
2. Append the tag name to `BANK_TAG_NAMES` in `MyPlugin8.cpp` (first character becomes the label prefix).
3. Extend the `ID_Interface` enum with 25 more sequential `ID_BUTTON_N` entries (with `// range` and `// stack` section markers).

No changes to dispatch, mutual exclusion, or AND-combine — they scale on `BANK_COUNT`.

## Stability rules

- **No exceptions cross the SDK boundary.** The SDK is `HRESULT`-based; an exception unwinding into VDJ's call site aborts the process.
- **No allocation or blocking work on audio-thread callbacks** if/when `OnProcessSamples` is added — the audio thread is real-time.
- Test changes in a low-stakes setting before relying on them live.

## Names

Built binary: `QFPro` (matches `meson.build` and `justfile`). Long display name: "QuickFilter Pro".

## References

- [VDJScript verb list](https://virtualdj.com/manuals/virtualdj/appendix/vdjscriptverbs.html) — authoritative list of all script verbs (e.g. `quick_filter`, `masterdeck`, `get_activedeck`, `get_comment`). First place to check before guessing syntax for `GetInfo`/`GetStringInfo`/`SendCommand` arguments.
- [VDJScript overview](https://virtualdj.com/wiki/vdjscript.html) — language reference: expression syntax, ternaries, deck scoping (`deck N <verb>`), boolean vs numeric verbs.
- [Developers wiki](https://virtualdj.com/wiki/Developers) — entry point for SDK docs (plugin types, interface, build notes).

VDJScript dual-use note: many verbs both *query* (when used in `GetInfo`/conditionals) and *act* (when sent via `SendCommand`). E.g. `masterdeck` returns true on the master deck when read, but pressing it makes that deck master.
