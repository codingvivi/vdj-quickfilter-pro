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

Buttons are declared in `MyPlugin8.cpp::OnLoad` in this order (= UI order = mapping number):

| # | Name (short = long) | Behavior |
|---|---|---|
| 1 | `RFR` (long: "Refresh Quick Filters") | (TODO) disengage + re-engage active filter against the master deck. |
| 2 | `Kill` (long: "Kill Quick Filter") | Sends `quick_filter off`, clears `m_ActiveFilter` cache, resets both range and stack state. |
| 3–15 | `E>=N.N`, `E=0`, `E=<N.N` | **Range mode.** See below. |
| 16–28 | `E+N.N`, `E0`, `E-N.N` | **Stack mode.** See below. |

Both modes target the master deck's `#NNEnergy` tag (parsed via `GetNumericTag("Energy")`) and produce a `quick_filter '<OR-joined clauses>'` expression. The two modes are mutually exclusive — pressing a button in one mode clears the other.

### Range mode (buttons 3–15)

Three independent pieces of state: `m_PosPeakHs` (positive ceiling in half-steps), `m_NegPeakHs` (negative ceiling), `m_ZeroEngaged` (the `=0` toggle).

- Press `>=N` → `m_PosPeakHs = N*2`, overwriting any prior positive peak. Filter includes half-steps `1..PosPeakHs` (i.e. master+0.5 .. master+N).
- Press the current positive peak again → `m_PosPeakHs = 0`, positive side cleared. Negative side and `=0` untouched.
- Same rules mirrored for `=<N` / `m_NegPeakHs`.
- `=0` toggles `m_ZeroEngaged`, fully independent of either side — does **not** get added or removed when range buttons are pressed.

### Stack mode (buttons 16–28)

Per-button latched state in `m_StackEngaged[ENERGY_BTN_COUNT]`. Each engaged button contributes only its own single half-step value (not a range). Press toggles its own state, leaves others alone.

### Mutual exclusion

`OnParameter` enforces it at dispatch:
- Range press while `AnyStackEngaged()` → `ClearStackState()` first, then handle the range press.
- Stack press while `AnyRangeEngaged()` → `ClearRangeState()` first, then handle the stack press.

`RebuildEnergyFilter` unions both contributions into a `std::set<int>` of half-step deltas, formats each as `#<value>Energy` via `formatTagValue` (preserving the master tag's zero-padding width for integer values, `%g` for fractional), wraps with `BuildOrExpression`, and dispatches via `SendFilterExpression`.

### Active-filter tracking

`m_ActiveFilter` caches the last `quick_filter '<expr>'` string sent. To swap filters, `SendFilterExpression` re-sends the cached string (VDJ toggles same-expression off), then sends the new one. Same-expression re-send disengages without replacing. `quick_filter off` alone did **not** reliably clear custom expressions in testing, hence the cache-based dance.

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
