# ASCII ARENA - Technical Design Report

## 1. Concept

ASCII ARENA is a top-down survival shooter in the vein of wave-based arena
games, but the entire presentation is text. A 100x34 console grid is the screen;
`@` is the player, letters are enemies, `*` are bullets, `#` blocks are cover and
the same grid holds the HUD. There is no image, font or audio asset: the
executable and its source are the whole game.

The visual target is white-on-black terminal art: bright `@` and shots, mid-grey
enemies, dim floor grit (`,` `.` `` ` ``), and a restrained 16-colour palette
used only for meaning (health, waves, boss, pickups).

## 2. Goals and constraints

| Goal | How it is met |
|---|---|
| Runs on a stock Windows machine | Win32 Console API only, no third-party libraries |
| Portable enough to test anywhere | POSIX branch in the platform layer (termios + ANSI) |
| Good OOP demonstration | abstract base classes, polymorphism, encapsulation, composition, templates |
| Real-time feel in a text terminal | fixed 60 Hz simulation, double-buffered frame output, non-blocking input |
| Deterministic and testable | own RNG with seed control, headless simulation mode |
| Readable codebase for a marker | layered folders, one responsibility per file, ~3.2k lines total |

## 3. Architecture

Three layers, each with a narrow interface:

```
             +-------------------------------------------------+
             |  main.cpp: config + CLI, creates Game            |
             +------------------------+------------------------+
                                      |
+-------------------------------------v--------------------------------------+
| GAME LAYER                                                                 |
|  Game (loop, layout, state switching)                                      |
|    |-- IGameState: Menu / Play / Pause / Upgrade / GameOver  (States)       |
|    |-- World (simulation)                                                   |
|    |     |-- Player                                                         |
|    |     |-- vector<unique_ptr<Enemy>>  -> Rusher, Spitter, Brute, Boss     |
|    |     |-- vector<unique_ptr<Projectile>>                                  |
|    |     |-- vector<unique_ptr<Pickup>>                                      |
|    |     |-- ParticleSystem  (ObjectPool<Particle>)                          |
|    |     |-- WaveDirector, obstacles, debris                                 |
|    |-- Hud (status bar, boss bar, banner, footer)                            |
|    |-- Scoreboard (highscores.txt)                                           |
+-------------------------------------+--------------------------------------+
                                      | Canvas cells
+-------------------------------------v--------------------------------------+
| CORE LAYER                                                                 |
|  Canvas (grid, clipping, text/ANSI export)                                  |
|  Platform/Terminal (console sizing, raw input, frame write)                  |
|  Config (key=value parser), Rng (xorshift), ObjectPool<T>, Math (Vec2)      |
+----------------------------------------------------------------------------+
```

Rules that keep the layers honest:

* **Core layer never includes game code.** `Canvas` and `Terminal` know nothing
  about enemies or waves.
* **Game logic never calls the OS directly.** All output goes through `Canvas`,
  all input arrives as an `InputState` value.
* **`World` is engine-agnostic.** It can run 600 steps with no terminal at all,
  which is exactly what `--demo` does.

### 3.1 Class hierarchy

```
Entity (abstract)
 |  virtual update(World&, float) = 0
 |  virtual draw(Canvas&, int, int) const
 |  pos, radius, glyph, colour, alive
 |
 +-- Player        (dash, auto-aim, XP, upgrades)
 +-- Enemy (abstract: update() handles health/contact, behave() = AI)
 |    +-- Rusher   chase + sinusoidal weave
 |    +-- Spitter  maintain distance, strafe, fire projectiles
 |    +-- Brute    slow, heavy, high HP
 |    +-- Boss     radial volley + telegraphed charge (isBoss() -> true)
 +-- Projectile    faction-tagged bullet, pierce counter
 +-- Pickup        heal / score, expires
```

`Entity` is the only class the containers know about; `Enemy` adds the pieces
every enemy shares (health, contact damage, hit flash, stuck detection) and
leaves the interesting part - `behave()` - to subclasses.

### 3.2 State machine

```
 Menu --start--> Playing <--resume-- Paused
   ^                |  \                 ^
   |                |   \--pause--------/
   |                | level up
   |                v
   |             Upgrade --chosen--> Playing
   |                |
   +---quit---------+--- death --> GameOver --retry--> Playing
```

`Game::changeState()` stores the new state in `pendingState_` and swaps it after
the current `update()` returns. That avoids deleting a state object while its own
`update()` is still on the stack.

## 4. Frame loop

```
while running:
    poll console input                      -> InputState
    accumulate real time, run 0..N fixed steps of 1/60 s
        WaveDirector -> Player -> enemies -> projectiles -> pickups
        collisions -> reap dead -> particles
    build Canvas: HUD, arena frame, world, overlays
    write the grid to the console in one pass
    sleep until the frame budget is used
```

* **Fixed timestep.** Gameplay always advances 1/60 s, so behaviour is
  frame-rate independent and reproducible.
* **Single write per frame.** On Windows the full `CHAR_INFO` buffer is handed to
  `WriteConsoleOutputW`, so the terminal never shows a half-drawn frame. On
  POSIX the frame is emitted with a cursor-home and per-run colour codes.
* **Input without echo.** Windows: `ReadConsoleInput` with `ENABLE_WINDOW_INPUT`
  gives real key-down/up events (so held keys work). POSIX: raw `termios` with
  `VMIN=0`, plus arrow-key escape parsing.

## 5. Simulation details

### 5.1 Coordinates

Entity positions are floats measured in character cells inside the arena
interior (98x30 by default). Rendering rounds to the nearest cell, which keeps
movement smooth at 60 Hz even though the display is discrete.

### 5.2 Collisions

All collisions are circle overlaps in cell space, evaluated centrally in
`World::collide()`:

1. player bullets vs enemies - on hit: damage, hit flash, particles, decrement
   `pierce`, destroy the bullet when pierce is exhausted;
2. enemy bullets vs player - respects dash invulnerability;
3. player vs pickups - applies heal/score and a short HUD flash.

Contact damage and enemy separation live in `Enemy::update()` so every enemy
type inherits it. Cover uses circle-vs-AABB tests; `World::moveEntity()` resolves
movement **per axis**, which makes entities slide along walls instead of sticking.

### 5.3 Enemies stuck on cover

Enemies have no pathfinding. If an enemy makes less than 35% of its expected
movement for 0.7 s it enters an "escape" state: it moves perpendicular to the
player for 0.85 s, which reliably walks it around a block. This is a deliberate
cheap alternative to A*: it keeps the code small and prevents the visible failure
mode of enemies grinding against a wall.

### 5.4 Waves

`WaveDirector` is the only place that knows about pacing:

| Wave rule | Value |
|---|---|
| enemies per wave | `(6 + 3 * wave) * difficulty`, clamped to 5..40 |
| spawn interval | `max(0.22, 0.9 - 0.05 * wave)` seconds |
| release order | shuffled queue, one spawn per interval |
| boss | every 5th wave (plus a third of the normal count) |
| wave end | arena empty **or** 34 s elapsed |
| rest between waves | 2.8 s |

The time limit is a robustness decision: without it a single spitter hiding
behind cover could freeze progression forever.

### 5.5 Difficulty table

| Enemy | Glyph | HP | Speed (cells/s) | Contact | XP | Score |
|---|---|---|---|---|---|---|
| Rusher | `r` | 16 x `(1+0.16(w-1))` | 4.9 + 0.22w (cap 11) | 8 | 2 | 10 |
| Spitter | `s` | 26 x `(1+0.17(w-1))` | 3.3 | 7 | 3 | 16 |
| Brute | `B` | 78 x `(1+0.18(w-1))` | 2.55 | 16 | 6 | 34 |
| Warden | `W` | 420 x `(1+0.12(w-1))` | 2.3 (+charge x4.2) | 22 | 25 | 320 |

All HP values are additionally multiplied by `difficulty_scale`. Player defaults:
100 HP, 16 cells/s, 10 damage, 0.42 s fire interval, 27 cells/s bullets.

### 5.6 Progression

Kills grant XP. Level `n` needs `6 + 4n` XP, and every level pushes one pending
upgrade. The upgrade pool is table-driven:

| Upgrade | Effect |
|---|---|
| Rapid Fire | fire interval x 0.85 (floor 0.12 s) |
| Heavy Rounds | +5 damage |
| Piercing Rounds | +1 pierce (max 4) |
| Swift Boots | speed x 1.12 |
| Vitality | +25 max HP, heals 25 |
| Twin Cannons | +1 projectile per shot, spread (max 3 extra) |
| Dash Coils | dash cooldown x 0.8 (floor 0.5 s) |
| Field Repair | heal 40 |

`rollUpgrades()` filters out options already at their cap and never offers the
same card twice in one choice.

### 5.7 Procedural layout

Each run seeds `Rng` and generates 5-7 cover blocks (4-7 x 2-3 cells) with
rejection sampling: no overlap with each other, a 2-cell margin, and the
player's spawn bowl kept clear. Dim floor grit is scattered afterwards, rejecting
any cell inside cover. Same seed, same arena.

## 6. OOP and C++ concepts by file

| Concept | Where |
|---|---|
| Abstraction / pure virtual functions | `Entity::update`, `Enemy::behave`, `IGameState::update/draw` |
| Runtime polymorphism via base pointers | `vector<unique_ptr<Enemy>>` iterated in `World`; `unique_ptr<IGameState> state_` |
| Inheritance and code reuse | `Rusher/Spitter/Brute/Boss` reuse health, contact damage, flash, stuck escape from `Enemy` |
| Encapsulation | `World` exposes accessors and keeps `enemies_`, `projectiles_`, `score_` private; `Canvas` clips writes internally |
| Composition | `Game` owns `World`, `Hud`, `Scoreboard`; `World` owns entities |
| Templates | `ObjectPool<T>` used as `ObjectPool<Particle>` (768 recycled slots) |
| RAII | `Terminal::~Terminal()` restores console modes/termios; `std::ofstream` for dumps |
| Smart pointers | `std::unique_ptr` for every heap entity; no manual `new`/`delete` except the saved termios state (deleted in `shutdown()`) |
| STL containers / algorithms | `std::vector`, `std::erase`-style `remove_if` in `World::reap`, `std::stable_sort` in `Scoreboard`, `std::shuffle`-style loop in `WaveDirector` |
| Lambdas | removal predicates, `ObjectPool::forEachAlive` visitors, upgrade `choose` lambda |
| Exception safety | `Config::load` catches `std::stoi/stof` errors and keeps defaults |
| File I/O | `Config` (parse + write defaults), `Scoreboard` (load/save), frame dumps |
| Design patterns | State (game screens), Factory (`makeEnemy`), Observer-free event calls (`notifyWaveStarted/Cleared`), Strategy-ish upgrade table |
| Fixed timestep / delta time | `Game::run`, `World::update(dt)` |
| Deterministic RNG | `Rng` xorshift32 with seed from config or clock |
| Bit flags / platform abstraction | `#ifdef _WIN32` in `Platform.cpp`, ANSI vs `CHAR_INFO` |
| Unicode in Win32 | `WriteConsoleOutputW` with `wchar_t` cells |

## 7. Verification performed

* **Builds warning-free** with `-Wall -Wextra -Wpedantic` (g++ 13, C++17).
* **AddressSanitizer + UndefinedBehaviorSanitizer** clean over a 120 s simulated
  run (hundreds of entities, thousands of particle churn).
* **Deterministic replay**: identical seeds produce identical summaries:
  `--demo 90 --seed 12345` and `--demo 116 --seed 4242` were used repeatedly
  during balancing.
* **Screen coverage**: `--screens` renders menu, gameplay, pause, upgrade and
  game-over frames; every screen was inspected for clipping and overlap
  (`docs/*.png` are those frames rendered to images).
* **Balance check** (scripted kiting bot, no upgrade choices): dies around
  110-120 s on wave 5 with ~1000 score, i.e. the boss wave is a real wall for a
  passive player, which is the intended difficulty curve.

## 8. Known limitations and future work

* No pathfinding: enemies use direct steering plus the stuck escape. A* or a flow
  field would let more complex arena layouts work.
* One player, keyboard only. A gamepad or mouse aim would need a different input
  abstraction, but `InputState` is the single seam to change.
* Terminal resizing during play is not handled live; the game reads the window
  size at start-up.
* The Windows build draws with `WriteConsoleOutputW` at ~60 Hz; on very slow
  terminals or remote sessions this can still tear slightly.
* Possible extensions: multiple weapons, elite modifiers, daily seed runs,
  replay files (the deterministic RNG makes this straightforward), or a small
  ASCII level editor that emits `config` obstacle layouts.
