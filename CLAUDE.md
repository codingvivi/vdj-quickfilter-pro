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
