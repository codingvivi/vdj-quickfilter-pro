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

107 buttons total: 3 control + 4 tag banks × 26. Declared in `MyPlugin8.cpp::OnLoad` in UI order = mapping number.

| Buttons | Group | Behavior |
|---|---|---|
| 1 | `Chk` "Refresh If Master Changed" | Auto-refresh callback for the VDJ-side `repeat_start_instant ... & load_pulse ? plugin "QFPro" 1` watcher. Compares master deck's filepath against `m_LastSeenFilePath`; calls `RebuildFilter()` only if it changed. Safe to fire on every load tick. |
| 2 | `RFR` "Refresh Quick Filters" | Unconditional refresh — calls `RebuildFilter()`, which re-reads master tags and goes through the disengage/re-engage dance via `SendFilterExpression`. |
| 3 | `Kill` "Kill Quick Filter" | Sends `quick_filter off`, clears `m_ActiveFilter` cache, calls `Clear()` on every bank. |
| 4–29 | Energy bank (`E>=N.N`, `E0`, `E=<N.N`, `E+N.N`, `E0`, `E-N.N`) | Range 4–16, Stack 17–29. |
| 30–55 | Happy bank (`H...`) | Range 30–42, Stack 43–55. |
| 56–81 | Dance bank (`D...`) | Range 56–68, Stack 69–81. |
| 82–107 | Pop bank (`P...`) | Range 82–94, Stack 95–107. |

`m_LastSeenFilePath` is updated at the top of every `RebuildFilter()` call, so the Check cache stays current regardless of which path triggered the rebuild (bank press, manual Refresh, or Check itself).

**Recommended ONINIT mapping snippet** (lives in the controller mapper, not the plugin):
```
repeat_start_instant "qfpro_watch" 100ms & load_pulse ? plugin "QFPro" 1 : nothing
```
Catches all load events (drag, controller, autoload). Does not catch master-deck *swaps* — for that, press button 2 (Refresh) manually after switching master.

Each bank targets one numeric tag in the master deck's comment (e.g. `#03Energy`), parsed by `GetNumericTag(tagName)`. Within a bank, range and stack are mutually exclusive; across banks, contributions are AND-joined into a single `quick_filter` expression.

### TagBank (struct in MyPlugin8.h)

State-only, no SDK calls. Each instance owns:
- `tagName` — e.g. "Energy".
- `rangeBtns[13]`, `stackBtns[13]` — SDK-owned momentary press state.
- `posPeakHs`, `negPeakHs` — current range peak per side, in half-steps (0..6).
- `zeroRangeEngaged` — standalone-zero latch for range mode (true when 0 is engaged with no pos/neg side wider). Master (delta 0) is included whenever `AnyRangeEngaged()` is true — i.e. any of pos/neg/zero is on.
- `stackEngaged[13]` — per-button latch for stack mode.

Methods: `Init`, `Clear`, `HandleRangePress(idx)`, `HandleStackPress(idx)`, `AnyRangeEngaged`, `AnyStackEngaged`, `AnyEngaged`, `ComputeDeltas()` (returns the union of half-step offsets relative to master).

### Range mode (13 buttons per bank, includes `0` at index 6)

- Press `>=N` → `posPeakHs = N*2`, overwriting any prior positive peak. Contribution: half-steps `1..posPeakHs` (= master+0.5 .. master+N).
- Press the current positive peak again → `posPeakHs = 0`, positive side cleared. Negative side untouched.
- Same rules mirrored for `=<N` / `negPeakHs`.
- Press `0`: if either side is wider than 0, shrink both sides to 0 (`posPeakHs = negPeakHs = 0`, `zeroRangeEngaged = true` → bank contributes just master). If only `0` was engaged, press turns the bank off. Otherwise (nothing in range mode active) engages just `0`.
- **Master (delta 0) is included whenever any range side is active** — i.e. `0` is the implicit start of both sides; pressing `>=N` or `=<N` automatically engages 0 as well.

### Stack mode (13 buttons per bank, includes `0`)

Per-button latch in `stackEngaged[i]`. Each engaged button contributes only its single half-step value (not a range). Press toggles. The stack `0` button contributes the master value itself — use it to include master in an additive selection (e.g. `{master, +1.0, -2.0}`). Stack `0` and range `0` are independent buttons: stack `0` adds master to a stack selection; range `0` is part of the range-mode state machine and turns off the bank when toggled off alone.

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
2. Append the tag name to `BANK_TAG_NAMES` in `MyPlugin8.cpp` (first character becomes the label prefix, lowercased for VDJ var names).
3. Extend the `ID_Interface` enum with 25 more sequential `ID_BUTTON_N` entries (with `// range` and `// stack` section markers).

No changes to dispatch, mutual exclusion, AND-combine, or var-sync — they scale on `BANK_COUNT`.

## VDJ var mirroring (LED feedback)

The unnamed `quick_filter '<expr>'` VDJ uses isn't queryable from a controller mapping, so the plugin **mirrors** its in-memory bank state into global non-persistent VDJScript vars (`$` prefix, no `@`). The plugin stays the source of truth; vars exist purely so mappings can light LEDs.

Per bank, 15 vars (4 banks × 15 = 60 total):

| Var | Type | Meaning |
|---|---|---|
| `$qfpro_<x>_pos` | float | signed positive-side peak, `0..3.0` (`0` = side off) |
| `$qfpro_<x>_neg` | float | signed negative-side peak, `-3.0..0` (`0` = side off) |
| `$qfpro_<x>_r_0` | 0/1 | range `0` LED — lit whenever `AnyRangeEngaged()` (standalone-zero or either side > 0) |
| `$qfpro_<x>_s_<suffix>` | 0/1 | stack-button latch, one per stack button |

`<x>` is the lowercased first char of `tagName` (`e`, `h`, `d`, `p`). Stack suffixes are var-name-safe versions of `STACK_SUFFIX`: `p30 p25 p20 p15 p10 p05 0 n05 n10 n15 n20 n25 n30` (kept in `STACK_VAR_SUFFIX`).

**Sync points** — `SyncBankVars(b)` is called from:
- `OnLoad`, once per bank after `Init` (initializes all 60 vars to 0).
- `OnParameter` after every bank press (only the affected bank — handlers never touch other banks).
- `OnParameter` Kill branch, for all banks after `Clear()`.

**LED mapping recipes** (use these in the controller mapper, not in the plugin):
- Stack `E+0.5`: `var '$qfpro_e_s_p05' ? on : off`
- Stack `E0`: `var '$qfpro_e_s_0' ? on : off`
- Range `E0`: `var '$qfpro_e_r_0' ? on : off`
- Range positive `E>=1.5`: `var_greater_or_equal '$qfpro_e_pos' 1.5 ? on : off`
- Range negative `E=<2.0`: `var_smaller_or_equal '$qfpro_e_neg' -2.0 ? on : off`

VDJ verbs in use: `set '$name' <num>` to write, `var`/`var_equal`/`var_greater_or_equal`/`var_smaller_or_equal`/`get_var` to read. No `set_var` verb exists — `set` is the only writer.

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
