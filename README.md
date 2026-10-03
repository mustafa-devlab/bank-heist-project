# Bank Heist: The Last Score

A large, multilayered text-based heist game written in plain C, using only first-semester fundamentals: variables, operators, `if`/`switch`, loops, functions, arrays, and `char` strings. No structs, no dynamic memory, no file I/O.

You owe the wrong people a lot of money. Pull off enough jobs to reach **$300,000 within 60 days** and disappear. Get arrested **three times** and it's over.

## Build and Run

You need a C compiler such as `gcc` (Linux, macOS, or MinGW/MSYS2 on Windows).

```bash
gcc -Wall -Wextra -std=c99 bank_heist.c -o bank_heist
./bank_heist
```

On Windows, run `bank_heist.exe` instead of `./bank_heist`.

## How the Game Works

### 1. The Hideout
Your base between jobs.

| Option | What it does |
|---|---|
| Plan a heist | Pick a target and start a job |
| Hire crew | Recruit specialists (one-time cost, they take a cut of every haul) |
| Black market | Buy gear |
| Lay low | Pass 1 to 5 days to lower heat and heal |
| Status & record | See your stats, crew, gear, and last 5 heists |

Rent of $300 is charged every day, and random daily events can help or hurt you.

### 2. The Targets

| Bank | Vault | Guards | Difficulty | Reputation needed |
|---|---|---|---|---|
| Corner Credit Union | $40,000 | 2 | 1 | 0 |
| City Trust Bank | $95,000 | 3 | 2 | 3 |
| Federal Reserve Branch | $210,000 | 4 | 3 | 8 |

Successful heists raise your reputation, which unlocks bigger banks.

### 3. The Crew

| Member | Role | Strength | Cut |
|---|---|---|---|
| Ghost | Infiltrator | Stealth | 8% |
| Byte | Hacker | Vault cracking | 10% |
| Tank | Muscle | Combat and carrying more bags | 7% |
| Wheels | Driver | Getaway chase | 9% |

### 4. A Heist, Step by Step

1. **Planning:** optionally pay $1,000 to case the bank for +1 stealth and a hint about the vault code.
2. **Entry:** choose your approach.
   - **Stealth:** move through a live map. Guards patrol, cameras blink on and off, and you need a keycard (or lockpicks) to open the locked door. Grab loose cash along the way if you dare.
   - **Loud assault:** storm the lobby in turn-based combat against the guards.
3. **The vault:** crack a 3-digit code (each digit 1 to 6) using Mastermind-style feedback, then decide how many bags to grab. Greed raises the chance of tripping the alarm.
4. **The getaway:** an 8-round chase. Get your lead to 10 to vanish, or lose it and get caught.
5. **Payday:** your crew takes their cut, you earn reputation, and the heat rises.

### Stealth Controls

| Key | Action |
|---|---|
| `w` `a` `s` `d` | Move |
| `x` | Wait a turn |
| `b` | Throw a smoke bomb (stuns all guards for a few turns) |
| `q` | Abort the heist |

Map legend: `P` you, `G` guard, `g` stunned guard, `*` camera on, `o` camera off, `x` camera jammed, `K` keycard, `D` locked door, `$` loose cash, `V` vault.

### Heat, Health, and Strikes

- **Heat** goes up when you are spotted, fail vault attempts, or grab loot. At 100%, the police arrive.
- **Health** drops from fights and gunfire. Armor reduces damage by 40%.
- **Getting arrested** costs you the loot, $5,000 bail, a few days, and one strike.

## Tips

- Early on, the Corner Credit Union is the safe way to build cash and reputation.
- The Signal Jammer and Stealth Boots make stealth runs much easier.
- Hiring a hacker or buying the laptop gives you more vault attempts.
- Don't be greedy. A bag of cash is worth nothing in a police cruiser.

## Programming Concepts Used

- Variables, data types, and operators
- `if` / `else if` / `else` and `switch` statements
- `for`, `while`, and `do-while` loops
- Functions with prototypes, parameters, and return values
- 1D and 2D arrays (including a char map and parallel data tables)
- Strings as `char` arrays
- `#define` constants and global variables
- `rand()` and `srand()` for randomness
- Input validation with `scanf`

## Project Structure

```
.
├── bank_heist.c   # entire game source
├── README.md
└── .gitignore     # keeps the compiled binary out of the repo
```

## Ideas for Future Versions

- Save and load progress using file I/O
- Convert the parallel arrays into structs
- Add more maps or randomly generated floor plans
- Add new crew members, banks, and gear
