# Vaeneth Music — Adaptive Score for Unreal Engine 5

A state-driven adaptive music system for Unreal Engine 5.8.

**Horizontal re-sequencing** — the track changes with the game state.
**Vertical remixing** — stems fade in and out with a single intensity value.
**Priority arbitration** — zones and threats propose, the subsystem decides.

Every system is a Blueprint you can open and change. A small C++ facade sits
underneath so that no node in the API ever asks you for a world context.

---

## Contents

1. [Install](#1-install)
2. [The demo map](#2-the-demo-map)
3. [Your own level in five minutes](#3-your-own-level-in-five-minutes)
4. [Placeable actors and the threat component](#4-placeable-actors-and-the-threat-component)
5. [The Settings asset](#5-the-settings-asset)
6. [Authoring a MusicSet](#6-authoring-a-musicset)
7. [Core concepts](#7-core-concepts)
8. [Blueprint API](#8-blueprint-api)
9. [Saving and restoring](#9-saving-and-restoring)
10. [Audio routing](#10-audio-routing)
11. [Troubleshooting](#11-troubleshooting)
12. [Known limitations](#12-known-limitations)
13. [Under the hood](#13-under-the-hood)
14. [Requirements, licensing and support](#14-requirements-licensing-and-support)

---

## 1. Install

This product is delivered as an Unreal Engine project. You can open
`VaenethMusic.uproject` directly to explore the demo, or take the plugin into
your own game:

1. Copy the `Vaeneth` folder from `Plugins/` into your own project's `Plugins/`
   directory. Create that directory if it does not exist yet.
2. Restart the editor. Unreal will offer to build the module — accept.
3. Check **Edit → Plugins → Audio** and confirm *Vaeneth Music* is enabled.

All content ships inside the plugin and appears in the Content Browser under
**Vaeneth → Music**. If you do not see it, enable *Show Plugin Content* in the
Content Browser's settings dropdown.

> **The folder must stay named `Vaeneth`.** Renaming it changes the content
> mount point, and the music subsystem can no longer be found. If you have
> already renamed it, rename it back and restart.

Nothing else is required. There is no manager to place, no actor to spawn: the
subsystem creates itself on first use.

---

## 2. The demo map

Open `Vaeneth/Music/Demo/L_VN_MusicDemo` and press Play. Walk towards **+X**
through the seven stations.

| Station | What it shows |
|---|---|
| 1 — Exploration zone | A zone proposes a state; the subsystem arbitrates |
| 2 — Second exploration zone | Overlapping zones resolve by priority |
| 3 — Combat zone + 3 enemies | Vertical remix: 1 / 2 / 3 threats → 0.2 / 0.4 / 0.6 |
| 4 — Dialogue zone | The score ducks on enter and returns on exit |
| 5 — Ambient source | A diegetic radio ducks the score inside its radius |
| 6 — Narrative triggers | A scripted state, and a second trigger clearing it |
| 7 — Portal | Music survives a level change |

**Debug overlay** — top left. Shows the current track, intensity, threat count,
active zone count and duck multiplier. Toggle it with `ShowDebugOverlay`, or
tick *Show Debug Overlay* on a Settings asset.

**Debug keys** — `Page Up` and `Page Down` step the intensity by 0.1, so you can
hear the vertical remix without spawning enemies. Stand inside the combat zone
but out of enemy range (around `-700, -900`); the threat system overrides the
keys the moment an enemy engages. These are Debug Key events: they fire in the
editor and in development builds only, and are stripped from shipping builds.

The demo is driven by `BP_VN_DemoGameMode`, which calls `ApplySettings` on
`DA_VN_DemoSettings` at Begin Play. That is the whole setup — the next section
does the same thing in your own project.

---

## 3. Your own level in five minutes

### Step 1 — Make a MusicSet

Duplicate `Vaeneth/Music/Demo/DA_VN_DemoMusicSet` into your own content folder,
or create one from scratch: **right-click → Miscellaneous → Data Asset →
`DAB_VN_MusicSet`**.

Open it and fill the **States** array. One entry per state you want:

- **State** — pick from `E_VN_MusicState` (Exploration, Combat, Boss…).
- **Tracks** — at least one entry, with a **Base Track** sound assigned.

A single looping track per state is the most common setup and needs nothing
else. That is enough to hear music.

### Step 2 — Make a Settings asset

**Right-click → Miscellaneous → Data Asset → `DAB_VN_Settings`**.

Set **Default Music Set** to the asset from step 1. Leave the rest on its
defaults — Master Volume is 1.0, ducking is 0.3 with a 0.25 s attack and a
0.8 s release.

### Step 3 — Apply it once, at Begin Play

Call **ApplySettings** once when the game starts, from wherever suits you: your
GameMode, your Player Controller, or the Level Blueprint.

```
Event BeginPlay  -->  ApplySettings (Settings Asset = your DAB_VN_Settings)
```

This sets the master volume and the duck curve, and loads the MusicSet.

### Step 4 — Make something ask for a state

**This is the step people miss.** Loading a MusicSet caches it — it does not
start playback. Something has to ask for a state. Pick one:

- **Drop a `BP_VN_MusicZone`** into the level, scale its box over the playable
  area, set *State on Enter* to `Exploration` and *Priority* to `5`. Music
  starts when the player walks in. This is the recommended baseline.
- **Drop a `BP_VN_MusicTrigger`** and tick *Trigger on Begin Play* with
  *Target State* set. Music starts the moment the level loads.
- **Call `SetMusicState`** directly from your own Blueprint.

Press Play. You should hear music.

### Step 5 — Layer on top

From here, everything else is additive. Drop a combat zone at a higher
priority, put a `BPC_VN_IntensitySource` on your enemies, place a dialogue zone
where you need the score to step aside. None of it requires touching what you
built in steps 1 to 4.

---

## 4. Placeable actors and the threat component

Five assets do all the level-side work. Drag them from the Content Browser into
your level; the component goes on an actor instead.

### `BP_VN_MusicZone` — a volume that proposes a state

The workhorse. Scale its box to cover an area; while the player is inside, the
zone proposes its state to the arbitration.

| Property | What it does |
|---|---|
| **State on Enter** | The state this zone proposes |
| **Priority** | Higher wins against other active zones |
| **Intensity on Enter** | Intensity to apply, if the next box is ticked |
| **Apply Intensity on Enter** | Whether the zone drives intensity at all |
| **Music Set** | Optionally swap the whole MusicSet on entry |
| **Apply Music Set on Enter** | Whether the swap above happens |
| **Only Player Pawn** | Ignore anything that is not the player |

Zones can overlap freely — that is the point. Give the whole playable area a
baseline zone at priority 5 and let everything else outrank it.

### `BP_VN_MusicTrigger` — a one-shot scripted event

For narrative moments rather than places. Fires on overlap, on Begin Play, or
both.

| Property | What it does |
|---|---|
| **Target State** / **Apply State** | The state to force, and whether to |
| **State Priority** | Where the forced state sits in the arbitration |
| **State Persists** | Keep the override after the trigger ends |
| **Clears State Override** | Use this trigger to release a previous one |
| **Target Intensity** / **Apply Intensity** | Push an intensity value |
| **Music Set** / **Apply Music Set** | Swap the MusicSet |
| **Play Stinger** / **Stinger Volume** | Fire a one-shot over the score |
| **Trigger on Overlap** / **Trigger on Begin Play** | When it fires |
| **Trigger Once** | Never fire again after the first time |
| **Only Player Pawn** | Ignore anything that is not the player |

### `BP_VN_DialogueZone` — duck the score while the player is inside

| Property | What it does |
|---|---|
| **Duck Amount** | Multiplier applied to the score, 0–1 |
| **Fade Down Time** | Attack, in seconds |
| **Fade Up Time** | Release, in seconds |
| **Only Player Pawn** | Ignore anything that is not the player |

Requests stack. A cutscene starting inside a dialogue does not fight it: the
score only returns when the last request is released.

### `BP_VN_AmbientMusicSource` — a diegetic source in the world

A radio, a tavern band, a distant parade. Plays a spatialised sound and ducks
the score inside its radius, so the world's music and the score never compete.

| Property | What it does |
|---|---|
| **Sound** | The diegetic sound to play |
| **Volume** | Its level |
| **Fade in Time** | Fade applied when it starts |
| **Duck Main Music Inside** | Duck the score while the player is in range |
| **Play on Begin Play** | Start automatically |
| **Only Player Pawn** | Ignore anything that is not the player |

Spatialisation uses `ATT_VN_AmbientMusic`. Swap it on the audio component for
your own attenuation curve.

### `BPC_VN_IntensitySource` — drop it on an enemy

Add this component to any actor that should raise the tension. It registers
itself as a threat and the subsystem converts the live threat count into
intensity.

| Property | What it does |
|---|---|
| **Auto Detect** | Watch for the player automatically |
| **Detection Radius** | How close the player must be to count |
| **Require Line Of Sight** | Only count while the enemy can see the player |
| **Check Interval** | Seconds between checks — no Tick is used |
| **Auto Register on Begin Play** | Register without any wiring |

For full manual control, untick *Auto Detect* and call `ModifyThreatCount`
yourself: `+1` when the enemy engages, `-1` when it disengages or dies. The
component guards against double counting either way.

---

## 5. The Settings asset

A `DAB_VN_Settings` Data Asset holds the global configuration. `ApplySettings`
pushes all of it to the subsystem in one call.

| Field | Default | What it does |
|---|---|---|
| **Master Volume** | `1.0` | Global level for the score |
| **Default Music Set** | none | Loaded automatically by `ApplySettings` |
| **Duck → Duck Volume Multiplier** | `0.3` | Level the score ducks to |
| **Duck → Attack Time** | `0.25` | Seconds to duck down |
| **Duck → Release Time** | `0.8` | Seconds to come back |
| **Show Debug Overlay** | off | Show the diagnostics overlay on apply |
| **Music Sound Class** | `SC_VN_Music` | Routing for the score |
| **Stinger Sound Class** | `SC_VN_Stinger` | Routing for one-shots |
| **Use Quartz Quantization** | off | Reserved — see *Known limitations* |

A **Duck Sound Mix** field is also present. Leave it empty unless you drive
ducking through a Sound Mix of your own.

You can keep several Settings assets and swap between them at runtime — one for
gameplay, one for a menu, one for a cinematic pass — by calling `ApplySettings`
again.

---

## 6. Authoring a MusicSet

A `DAB_VN_MusicSet` holds **States**, **Transition Rules**, and a
**Default Fade Time** (`2.0` seconds on a new asset).

### States

One entry per music state. Each holds:

| Field | What it does |
|---|---|
| **State** | An `E_VN_MusicState` value |
| **Tracks** | One or more tracks; several means the state cycles |
| **Layer Mode** | `IntensityThreshold` |
| **Shuffle** | Pick the next track at random |
| **Avoid Immediate Repeat** | Never play the same track twice in a row |
| **Silence Between Tracks** | Gap in seconds when a state holds several tracks |

### Tracks

| Field | What it does |
|---|---|
| **Track Name** | Label, shown in the debug overlay |
| **Base Track** | The sound that always plays in this state |
| **Layers** | Stems that fade in and out with intensity |
| **BPM**, **Beats Per Bar**, **Bars Per Section** | Metadata, reserved for quantisation |
| **Volume** | Track level |
| **Fade In Time** / **Fade Out Time** | Used when no transition rule matches |
| **b Looping** | Loop the base track |

Track duration is read from the asset — there is no field to fill in. It only
matters for states holding several tracks, where the subsystem schedules the
move to the next one. A state built on a single looping track plays until the
state changes, which is the intended behaviour and the most common setup.

### Layers

| Field | What it does |
|---|---|
| **Layer Name** | Label |
| **Layer Sound** | The stem |
| **Intensity Min** / **Intensity Max** | The window in which this stem is audible |
| **Volume** | Level inside the window |
| **Fade Time** | Seconds to fade in and out |
| **b Start Muted** | Start silent and fade in when the window opens |

All stems must be the same length as the base track and start at the same
point — they are started together and never resynchronised.

### Transition rules

| Field | What it does |
|---|---|
| **From State** / **To State** | Which change this rule covers |
| **Mode** | `Immediate`, `Crossfade` or `LayersOnly` |
| **Fade Out Time** / **Fade In Time** | Timing for the crossfade |
| **b Use Stinger** / **Stinger** | Play a one-shot across the transition |
| **Priority** | Which rule wins when several match |

`LayersOnly` is worth knowing: it keeps the current base track playing and only
re-evaluates the stems. Use it between two states built on the same track —
Combat to Boss, for example — so the music intensifies without restarting.

With no matching rule, the MusicSet's **Default Fade Time** is used.

---

## 7. Core concepts

### States and arbitration

Zones do not set the music state. They **register** with the subsystem,
proposing a state and a priority, and `ResolveActiveZones` picks the winner.
The scale used in the demo:

| Source | Priority |
|---|---|
| Baseline exploration zone | 5 |
| Local zones | 10 |
| Combat zone | 50 |
| Active threats | 100 |

The numbers are yours to choose — only their order matters. A baseline zone
covering the whole playable area at a low priority gives you a fallback that
every other zone can outrank.

Because arbitration re-runs on every change, leaving a zone or killing the last
enemy makes the music fall back on its own. **You never script the way back.**

### Intensity and the vertical remix

Intensity is a 0–1 value driving layer volumes. Each layer defines an
`[Intensity Min, Intensity Max]` window: inside it the layer fades to its
volume, outside it fades to silence, using the layer's own **Fade Time**. No
Tick is involved.

Threats feed intensity through `ThreatsForFullIntensity` (default 5), so three
engaged enemies give 0.6. Set intensity yourself with `SetIntensity` at any
time — a low-health state, a timer running out, a chase.

### Ducking

`BeginDialogue` and `EndDialogue` stack. If a cutscene starts during a dialogue,
the music only returns when the last request is released. The duck multiplier
combines with master volume and layer volumes rather than overwriting them, so
everything returns to its exact previous level.

### Music states

`E_VN_MusicState` ships with twelve entries: `Silence`, `Exploration`,
`Tension`, `Combat`, `Boss`, `Stealth`, `Cutscene`, `Victory`, `Defeat`, and
`Custom1` to `Custom3`. Rename the custom slots to suit your game — the enum is
a normal asset and yours to edit. Adding entries beyond those twelve is
supported; existing MusicSets are unaffected.

---

## 8. Blueprint API

All nodes live under **Vaeneth | Music** and fill in the world context
themselves, so none of them shows a `WorldContextObject` pin.

| Node | Purpose |
|---|---|
| `SetMusicSet` | Load a MusicSet and cache its states and rules |
| `SetMusicState` | Switch state, honouring transition rules |
| `SetIntensity` | Drive the vertical remix directly |
| `ModifyThreatCount` | `+1` when an enemy engages, `-1` when it disengages |
| `EnterZone` / `LeaveZone` | Register or unregister a zone |
| `SetStateOverride` / `ClearStateOverride` | A scripted state inside arbitration |
| `BeginDialogue` / `EndDialogue` | Stackable ducking |
| `SetDucked` | Duck or unduck immediately, without the stack |
| `PlayStinger` | One-shot over the music |
| `SetMasterVolume` | Global level |
| `StopMusic` | Fade out and go silent |
| `NotifyLevelTravel` | **Call immediately before `Open Level`** |
| `RegisterMusicListener` / `UnregisterMusicListener` | Subscribe an actor |
| `ShowDebugOverlay` | Toggle the diagnostics overlay |
| `ApplySettings` | Apply a Settings Data Asset |

### Listening to the music

Implement `BPI_VN_MusicListener` on any actor and register it with
`RegisterMusicListener`. Only the events you implement are called:

- `OnMusicStateChanged(NewState)`
- `OnMusicIntensityChanged(NewIntensity)`
- `OnMusicTrackStarted(Track)`

Useful for driving VFX, UI or a camera in step with the tension. Unregister in
`End Play` if the actor can be destroyed while the music keeps running.

---

## 9. Saving and restoring

`GetSnapshot` and `ApplySnapshot` live on the subsystem rather than in the
**Vaeneth | Music** node list. Reach them with `GetVaenethSubsystem`
(class = `BP_VN_MusicSubsystem`), cast the result, and call the function on the
cast output. They will be promoted to the node library in the next update.

`GetSnapshot` returns an `S_VN_Snapshot`: MusicSet path, state, intensity,
track index, playback position and master volume. Store it in your own SaveGame
and pass it back to `ApplySnapshot`.

The MusicSet is recorded by asset path rather than by pointer, because a hard
reference does not survive serialisation.

Playback position is restored: `ApplySnapshot` feeds it to the `StartTime` pin
of the incoming fade, so the track resumes where it stopped.

---

## 10. Audio routing

The plugin ships its own routing so it drops into an existing mix without
touching anything:

| Asset | Role |
|---|---|
| `SC_VN_Music` | Sound Class provided for the score and its stems |
| `SC_VN_Stinger` | Sound Class provided for one-shot stingers |
| `SubMix_VN_Music` | Submix both classes route into |
| `ATT_VN_AmbientMusic` | Attenuation used by the diegetic source |

To fold the score into your own bus, set the parent of `SubMix_VN_Music` to
your music submix. To keep the plugin's assets untouched, point **Music Sound
Class** and **Stinger Sound Class** on your Settings asset at your own classes
instead.

The demo music ships with **Virtualization Mode** set to *Play When Silent*.
Use that setting on your own stems too: it is what lets a muted layer hold its
playback position and come back in sync.

---

## 11. Troubleshooting

**I hear nothing at all.**
Loading a MusicSet does not start playback — something has to ask for a state.
Enter a `BP_VN_MusicZone`, use a `BP_VN_MusicTrigger` with *Trigger on Begin
Play*, or call `SetMusicState`. See section 3, step 4.

**Still nothing, and a state is definitely set.**
Open the debug overlay. If **Track** is empty, the state you asked for has no
matching entry in the MusicSet's *States* array, or its first track has no
**Base Track** sound assigned.

**Everything went silent after `ApplySettings`.**
**Master Volume** on your Settings asset is `0`. A new asset ships at `1.0`, so
this means it was changed.

**The music never comes back after a dialogue.**
**Duck Amount** on the dialogue zone, or **Duck Volume Multiplier** on the
Settings asset, is `0`. Use a small non-zero value such as `0.05`.

**The music stops when I change level.**
`NotifyLevelTravel` was not called immediately before `Open Level`. Without it,
stale-world detection treats the change as a session restart.

**The Vaeneth nodes do not appear in the Blueprint palette.**
The plugin folder was renamed. It must stay named `Vaeneth`.

**The debug overlay does not show.**
`ShowDebugOverlay` needs a world context: call it from an actor or a Level
Blueprint, and not before any other Vaeneth call has run.

**A node stopped working after I edited the subsystem.**
See the contract in section 13.

---

## 12. Known limitations

- **Playback position follows world time, not the audio clock.** Pausing the
  game or changing `Global Time Dilation` makes the reported position drift.
  Harmless for a save file; worth knowing if you rely on it for anything tighter.
- **A saved position past the end of a track is undefined.** This can only
  happen if you swap the MusicSet between two saves.
- **Transitions are not beat-synchronised.** Crossfades start when you ask for
  them. Quartz-based quantisation is planned for a later version; until then the
  **Use Quartz Quantization** field on the Settings asset, and the `OnBeat` and
  `OnBar` dispatchers on the subsystem, are placeholders that do nothing. Do not
  bind to them yet.
- **The demo portal loads its second map by name.** Both demo maps carry
  `BP_VN_DemoGameMode` as their GameMode Override, so the walkthrough works in
  any project. If you package the demo, add `L_VN_MusicDemo2` to the maps to
  cook — a map referenced only by name is not gathered automatically.
- **Stems are started together and never resynchronised.** Layers must be the
  same length as their base track and share its start point.

---

## 13. Under the hood

The subsystem is a Blueprint `GameInstanceSubsystem`: it survives level changes
by design, which is why music continues across `Open Level` without any work on
your side. The trade-off is that such a subsystem has no world of its own, so
the C++ facade hands it the caller's world on every call. That is the whole
reason the `VaenethCore` module exists, and why no node in the API shows a
`WorldContextObject` pin.

The module supplies four functions Blueprint cannot reach on its own:
`GetSoundDuration`, `IsSoundLooping`, `GetVaenethSubsystem` and
`GetImplicitWorldContext`.

### One contract to respect if you edit the subsystem

The facade calls the Blueprint subsystem by name, passing parameters
positionally. Every system in this plugin is yours to open and change — but if
you rename one of the functions below, or reorder or retype its parameters, the
matching node in **Vaeneth | Music** stops working **silently**, with no compile
error. Add parameters at the end, or update
`Source/VaenethCore/Private/VaenethMusicLibrary.cpp` to match.

Functions under contract: `SetMusicState`, `SetMusicSet`, `StopMusic`,
`PlayStinger`, `SetIntensity`, `ModifyThreatCount`, `SetMasterVolume`,
`PushDuck`, `PopDuck`, `SetDucked`, `RegisterZone`, `UnregisterZone`,
`ResolveActiveZones`, `SetStateOverride`, `ClearStateOverride`,
`ApplySettings`, `NotifyLevelTravel`, `RegisterMusicListener`,
`UnregisterMusicListener`, `ShowDebugOverlay`, `CacheWorldContext`.

Everything else — the zones, the triggers, the threat component, the debug
overlay, the arbitration logic itself — can be rewritten freely.

---

## 14. Requirements, licensing and support

**Unreal Engine 5.8.** No third-party plugin or asset dependency.

The demo audio is included and released under CC0 1.0 Universal. Per-track
authors and sources are listed in `Documentation/CREDITS.md`.

Questions, bug reports and feature requests:
https://github.com/alexandre5674/Vaeneth-Core/issues

Documentation: https://github.com/alexandre5674/Vaeneth-Core