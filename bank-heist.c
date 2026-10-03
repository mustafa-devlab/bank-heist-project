/* =====================================================================
   BANK HEIST: THE LAST SCORE
   A text-based campaign game written in plain C.

   Concepts used (all first-semester fundamentals):
     variables, data types, operators, if / else, switch, for / while /
     do-while loops, functions, 1D and 2D arrays, char arrays (strings),
     #define constants, global variables, rand() and srand().

   Compile:  gcc -Wall -Wextra -std=c99 bank_heist.c -o bank_heist
   Run:      ./bank_heist

   GAME LAYERS
     1. Campaign   : 60 days to reach the cash goal. 3 strikes = prison.
     2. Hideout    : hire crew, buy gear, lay low, random daily events.
     3. Planning   : pick a bank (unlocked by reputation), case the joint.
     4. Entry      : stealth through a live map (guards, cameras, keycard,
                     locked door) OR a loud assault on the lobby.
     5. Vault      : Mastermind-style code cracking, then greedy bag looting.
     6. Escape     : chase sequence against the police.
     7. Economy    : crew cuts, bail, rent, heat, health, reputation.
   ===================================================================== */

#include <stdio.h>
#include <stdlib.h>
#include <time.h>

/* ---------------------------- CONSTANTS ---------------------------- */
#define MAP_ROWS     8
#define MAP_COLS     20
#define NUM_CREW     4
#define NUM_ITEMS    7
#define NUM_BANKS    3
#define MAX_GUARDS   4
#define MAX_CAMS     3
#define MAX_HISTORY  5
#define GOAL_CASH    300000
#define MAX_DAYS     60
#define MAX_STRIKES  3
#define DAILY_RENT   300
#define BAIL_COST    5000

/* stat indexes for crewStat[][] */
#define HACK    0
#define MUSCLE  1
#define STEALTH 2
#define DRIVING 3

/* item indexes */
#define LOCKPICK 0
#define JAMMER   1
#define SMOKE    2
#define ARMOR    3
#define LAPTOP   4
#define ENGINE   5
#define BOOTS    6

/* ------------------------- DATA TABLES ----------------------------- */
char bankName[NUM_BANKS][32] = {
    "Corner Credit Union", "City Trust Bank", "Federal Reserve Branch"
};
int bankDifficulty[NUM_BANKS] = {1, 2, 3};
int bankVault[NUM_BANKS]      = {40000, 95000, 210000};
int bankGuards[NUM_BANKS]     = {2, 3, 4};
int bankRepReq[NUM_BANKS]     = {0, 3, 8};

char crewName[NUM_CREW][12] = {"Ghost", "Byte", "Tank", "Wheels"};
char crewRole[NUM_CREW][14] = {"Infiltrator", "Hacker", "Muscle", "Driver"};
int crewCost[NUM_CREW]      = {4000, 5000, 3500, 4500};
int crewCut[NUM_CREW]       = {8, 10, 7, 9};            /* % of the loot */
/*                              HACK MUSCLE STEALTH DRIVING */
int crewStat[NUM_CREW][4]   = { {1, 0, 3, 0},   /* Ghost  */
                                {3, 0, 1, 0},   /* Byte   */
                                {0, 3, 0, 1},   /* Tank   */
                                {0, 1, 0, 3} }; /* Wheels */
int crewHired[NUM_CREW]     = {0, 0, 0, 0};

char itemName[NUM_ITEMS][20] = {
    "Lockpick Set", "Signal Jammer", "Smoke Bomb", "Body Armor",
    "Hacking Laptop", "Tuned Engine", "Stealth Boots"
};
char itemDesc[NUM_ITEMS][58] = {
    "Pick the locked door without a keycard",
    "Switches off all security cameras",
    "Freezes guards / blocks police (single use, max 5)",
    "Reduces all damage you take by 40%",
    "+2 vault code attempts",
    "+2 head start in the getaway chase",
    "+1 stealth when slipping past guards"
};
int itemCost[NUM_ITEMS]  = {1500, 3000, 800, 2500, 4000, 5000, 2000};
int itemOwned[NUM_ITEMS] = {0, 0, 0, 0, 0, 0, 0};

/* The bank floor plan. # wall, . floor, E entry, K keycard,
   D locked door, $ loose cash, V vault door                           */
char mapTemplate[MAP_ROWS][MAP_COLS + 1] = {
    "####################",
    "#E.......#........$#",
    "#.....K..#.........#",
    "#........D.........#",
    "#...##...#.........#",
    "#...##...#.....$...V",
    "#........#.........#",
    "####################"
};

/* --------------------------- PLAYER STATE -------------------------- */
int cash = 8000;
int heat = 0;
int health = 100;
int rep = 0;
int day = 1;
int strikes = 0;
int heistsWon = 0;
int totalStolen = 0;
int histBank[MAX_HISTORY];
int histResult[MAX_HISTORY];   /* net money, or -1 if the heist failed */
int histCount = 0;

/* --------------------------- HEIST STATE --------------------------- */
char map[MAP_ROWS][MAP_COLS + 1];
int pr, pc;                    /* player row / column  */
int hasKey = 0;
int heistTurns = 0;
int pickupCash = 0;
int reconDone = 0;
int reconPos = 0;
int vaultCode[3];

int gRow[MAX_GUARDS]      = {2, 6, 3, 5};
int gStartCol[MAX_GUARDS] = {5, 3, 12, 15};
int gStartDir[MAX_GUARDS] = {1, -1, 1, -1};
int gMin[MAX_GUARDS]      = {2, 2, 11, 11};
int gMax[MAX_GUARDS]      = {8, 8, 17, 17};
int gCol[MAX_GUARDS];
int gDir[MAX_GUARDS];
int gFrozen[MAX_GUARDS];
int camCol[MAX_CAMS]      = {7, 14, 3};

/* ------------------------- FUNCTION PROTOTYPES --------------------- */
void flushLine(void);
int  readInt(int lo, int hi);
char readChar(void);
void pressEnter(void);
int  randRange(int lo, int hi);
void printLine(void);
void printHeader(char title[]);
void addHeat(int amount);
void hurt(int amount);
int  teamStat(int stat);
int  crewCutTotal(void);
int  crewCount(void);

void showTitle(void);
void showStatus(void);
void addHistory(int bank, int result);
void advanceDay(int days);
void dailyEvent(void);

void hireCrew(void);
void shopMenu(void);
void layLow(void);

void loadMap(void);
void drawMap(int bank);
int  isCamera(int col, int bank);
int  guardAt(int r, int c, int numGuards);
int  cameraActive(void);
void moveGuards(int numGuards);
int  checkGuards(int bank);
void checkCameras(int bank);
int  infiltrate(int bank);
int  loudAssault(int bank);
int  crackVault(int bank);
int  escapeRun(int bank, int loot);
void handleArrest(int bank);
void runHeist(void);
int  checkEnding(void);
void finalScreen(int ending);

/* ============================ UTILITIES ============================ */

/* throw away the rest of the current input line */
void flushLine(void) {
    int c;
    do {
        c = getchar();
    } while (c != '\n' && c != EOF);
}

/* read an integer in [lo, hi], keep asking until valid */
int readInt(int lo, int hi) {
    int value, ok;
    while (1) {
        printf("> ");
        ok = scanf("%d", &value);
        if (ok == EOF) {
            printf("\nInput closed. Goodbye.\n");
            exit(0);
        }
        flushLine();
        if (ok == 1 && value >= lo && value <= hi) {
            return value;
        }
        printf("   Please enter a number from %d to %d.\n", lo, hi);
    }
}

/* read one lower-case character command */
char readChar(void) {
    char c;
    int ok;
    ok = scanf(" %c", &c);
    if (ok == EOF || ok == 0) {
        printf("\nInput closed. Goodbye.\n");
        exit(0);
    }
    flushLine();
    if (c >= 'A' && c <= 'Z') {
        c = c + 32;
    }
    return c;
}

void pressEnter(void) {
    printf("\n   [Press Enter to continue]");
    flushLine();
}

int randRange(int lo, int hi) {
    return lo + rand() % (hi - lo + 1);
}

void printLine(void) {
    int i;
    for (i = 0; i < 62; i++) {
        putchar('=');
    }
    putchar('\n');
}

void printHeader(char title[]) {
    printf("\n");
    printLine();
    printf("  %s\n", title);
    printLine();
}

void addHeat(int amount) {
    heat = heat + amount;
    if (heat > 100) heat = 100;
    if (heat < 0)   heat = 0;
}

/* damage is reduced when wearing armor */
void hurt(int amount) {
    if (itemOwned[ARMOR] > 0) {
        amount = amount * 60 / 100;
    }
    health = health - amount;
    if (health < 0) health = 0;
    printf("   >> You take %d damage! (Health: %d)\n", amount, health);
}

/* total of one stat across the hired crew */
int teamStat(int stat) {
    int i, total = 0;
    for (i = 0; i < NUM_CREW; i++) {
        if (crewHired[i]) {
            total += crewStat[i][stat];
        }
    }
    return total;
}

int crewCutTotal(void) {
    int i, total = 0;
    for (i = 0; i < NUM_CREW; i++) {
        if (crewHired[i]) {
            total += crewCut[i];
        }
    }
    return total;
}

int crewCount(void) {
    int i, n = 0;
    for (i = 0; i < NUM_CREW; i++) {
        n += crewHired[i];
    }
    return n;
}

/* ============================ UI SCREENS =========================== */

void showTitle(void) {
    printf("\n");
    printf("   ____   _    _   _ _  __  _   _ _____ ___ ____ _____ \n");
    printf("  | __ ) / \\  | \\ | | |/ / | | | | ____|_ _/ ___|_   _|\n");
    printf("  |  _ \\/ _ \\ |  \\| | ' /  | |_| |  _|  | |\\___ \\ | |  \n");
    printf("  | |_) / ___ \\| |\\  | . \\  |  _  | |___ | | ___) || |  \n");
    printf("  |____/_/   \\_\\_| \\_|_|\\_\\ |_| |_|_____|___|____/ |_|  \n");
    printf("\n              T H E   L A S T   S C O R E\n\n");
    printLine();
    printf("  You owe the wrong people a lot of money. Pull off enough\n");
    printf("  jobs to reach $%d within %d days and disappear.\n", GOAL_CASH, MAX_DAYS);
    printf("  Three arrests and it's over. Stay smart. Stay quiet.\n");
    printLine();
}

void showStatus(void) {
    int i;
    printHeader("STATUS & CRIMINAL RECORD");
    printf("  Day %d / %d       Cash: $%d / $%d\n", day, MAX_DAYS, cash, GOAL_CASH);
    printf("  Health: %d        Reputation: %d        Strikes: %d / %d\n",
           health, rep, strikes, MAX_STRIKES);
    printf("  Heat  : [");
    for (i = 0; i < 20; i++) {
        putchar(i < heat / 5 ? '#' : '-');
    }
    printf("] %d%%\n", heat);
    printf("  Heists won: %d     Total stolen: $%d\n", heistsWon, totalStolen);

    printf("\n  CREW (cut taken: %d%% of loot)\n", crewCutTotal());
    if (crewCount() == 0) {
        printf("    (nobody - you work alone)\n");
    }
    for (i = 0; i < NUM_CREW; i++) {
        if (crewHired[i]) {
            printf("    %-7s %-12s HAK:%d MUS:%d STL:%d DRV:%d\n",
                   crewName[i], crewRole[i], crewStat[i][HACK],
                   crewStat[i][MUSCLE], crewStat[i][STEALTH], crewStat[i][DRIVING]);
        }
    }
    printf("\n  GEAR\n");
    for (i = 0; i < NUM_ITEMS; i++) {
        if (itemOwned[i] > 0) {
            printf("    %s x%d\n", itemName[i], itemOwned[i]);
        }
    }

    printf("\n  LAST HEISTS\n");
    if (histCount == 0) {
        printf("    (none yet)\n");
    }
    for (i = 0; i < histCount; i++) {
        if (histResult[i] >= 0) {
            printf("    %-24s SUCCESS  net $%d\n", bankName[histBank[i]], histResult[i]);
        } else {
            printf("    %-24s FAILED\n", bankName[histBank[i]]);
        }
    }
}

/* keep only the most recent MAX_HISTORY heists */
void addHistory(int bank, int result) {
    int i;
    if (histCount == MAX_HISTORY) {
        for (i = 0; i < MAX_HISTORY - 1; i++) {
            histBank[i] = histBank[i + 1];
            histResult[i] = histResult[i + 1];
        }
        histCount--;
    }
    histBank[histCount] = bank;
    histResult[histCount] = result;
    histCount++;
}

/* ============================ HIDEOUT ============================== */

void dailyEvent(void) {
    int roll = randRange(1, 10);
    switch (roll) {
        case 1:
            printf("   [Event] An informant sells you a tip. -$500, heat -5.\n");
            if (cash >= 500) cash -= 500;
            addHeat(-5);
            break;
        case 2:
            printf("   [Event] Police sweep your neighborhood. Heat +15!\n");
            addHeat(15);
            break;
        case 3:
            printf("   [Event] You win big at an underground card game. +$1500.\n");
            cash += 1500;
            break;
        case 4:
            printf("   [Event] A crew member picks a fight. Medical bills -$600.\n");
            if (cash >= 600) cash -= 600; else cash = 0;
            break;
        case 5:
            printf("   [Event] An old friend vouches for you. Reputation +1.\n");
            rep++;
            break;
        case 6:
            printf("   [Event] A snitch is spotted and handled. Heat -10.\n");
            addHeat(-10);
            break;
        default:
            printf("   [Event] A quiet day.\n");
            break;
    }
}

/* move time forward; rent, healing, heat decay and random events */
void advanceDay(int days) {
    int i;
    for (i = 0; i < days; i++) {
        day++;
        if (cash >= DAILY_RENT) cash -= DAILY_RENT; else cash = 0;
        addHeat(-5);
        health += 10;
        if (health > 100) health = 100;
        dailyEvent();
    }
}

void hireCrew(void) {
    int i, choice;
    while (1) {
        printHeader("RECRUITMENT");
        printf("  Cash: $%d\n\n", cash);
        for (i = 0; i < NUM_CREW; i++) {
            printf("  %d) %-7s %-12s HAK:%d MUS:%d STL:%d DRV:%d  $%d  cut %d%%",
                   i + 1, crewName[i], crewRole[i], crewStat[i][HACK],
                   crewStat[i][MUSCLE], crewStat[i][STEALTH], crewStat[i][DRIVING],
                   crewCost[i], crewCut[i]);
            if (crewHired[i]) printf("  [HIRED]");
            printf("\n");
        }
        printf("  0) Back\n");
        choice = readInt(0, NUM_CREW);
        if (choice == 0) return;
        i = choice - 1;
        if (crewHired[i]) {
            printf("   %s is already on your team.\n", crewName[i]);
        } else if (cash < crewCost[i]) {
            printf("   You can't afford %s.\n", crewName[i]);
        } else {
            cash -= crewCost[i];
            crewHired[i] = 1;
            printf("   %s the %s joins your crew!\n", crewName[i], crewRole[i]);
        }
    }
}

void shopMenu(void) {
    int i, choice;
    while (1) {
        printHeader("BLACK MARKET");
        printf("  Cash: $%d\n\n", cash);
        for (i = 0; i < NUM_ITEMS; i++) {
            printf("  %d) %-15s $%-5d %s", i + 1, itemName[i], itemCost[i], itemDesc[i]);
            if (itemOwned[i] > 0) printf("  (own %d)", itemOwned[i]);
            printf("\n");
        }
        printf("  0) Back\n");
        choice = readInt(0, NUM_ITEMS);
        if (choice == 0) return;
        i = choice - 1;
        if (cash < itemCost[i]) {
            printf("   Not enough cash.\n");
        } else if (i == SMOKE && itemOwned[SMOKE] >= 5) {
            printf("   You can't carry more than 5 smoke bombs.\n");
        } else if (i != SMOKE && itemOwned[i] > 0) {
            printf("   You already own that.\n");
        } else {
            cash -= itemCost[i];
            itemOwned[i]++;
            printf("   Purchased: %s\n", itemName[i]);
        }
    }
}

void layLow(void) {
    int days, i;
    printf("\n  How many days do you lay low? (1-5, extra heat drop each day)\n");
    days = readInt(1, 5);
    for (i = 0; i < days; i++) {
        addHeat(-15);
    }
    advanceDay(days);
    printf("   You rest up. Heat: %d  Health: %d  Cash: $%d\n", heat, health, cash);
}

/* ============================ THE MAP ============================== */

void loadMap(void) {
    int r, c, i;
    for (r = 0; r < MAP_ROWS; r++) {
        for (c = 0; c <= MAP_COLS; c++) {
            map[r][c] = mapTemplate[r][c];
            if (map[r][c] == 'E') {
                pr = r;
                pc = c;
                map[r][c] = '.';
            }
        }
    }
    for (i = 0; i < MAX_GUARDS; i++) {
        gCol[i] = gStartCol[i];
        gDir[i] = gStartDir[i];
        gFrozen[i] = 0;
    }
    hasKey = 0;
    heistTurns = 0;
    pickupCash = 0;
}

int isCamera(int col, int bank) {
    int i;
    for (i = 0; i < bankDifficulty[bank]; i++) {
        if (camCol[i] == col) return 1;
    }
    return 0;
}

int guardAt(int r, int c, int numGuards) {
    int i;
    for (i = 0; i < numGuards; i++) {
        if (gRow[i] == r && gCol[i] == c) return i;
    }
    return -1;
}

/* cameras sweep: they are active 2 turns out of every 3 */
int cameraActive(void) {
    return (heistTurns % 3 != 0);
}

void drawMap(int bank) {
    int r, c, g;
    char ch;
    int numGuards = bankGuards[bank];
    printf("\n");
    for (r = 0; r < MAP_ROWS; r++) {
        printf("   ");
        for (c = 0; c < MAP_COLS; c++) {
            ch = map[r][c];
            g = guardAt(r, c, numGuards);
            if (r == pr && c == pc) {
                ch = 'P';
            } else if (g >= 0) {
                ch = (gFrozen[g] > 0) ? 'g' : 'G';
            } else if (r == 0 && isCamera(c, bank)) {
                if (itemOwned[JAMMER] > 0)  ch = 'x';
                else if (cameraActive())    ch = '*';
                else                        ch = 'o';
            }
            printf("%c ", ch);
        }
        printf("\n");
    }
    printf("\n   P you  G guard  g stunned  * camera on  o camera off  x jammed\n");
    printf("   K keycard  D locked door  $ cash  V vault\n");
}

void moveGuards(int numGuards) {
    int i, next;
    for (i = 0; i < numGuards; i++) {
        if (gFrozen[i] > 0) {
            gFrozen[i]--;
            continue;
        }
        next = gCol[i] + gDir[i];
        if (next > gMax[i] || next < gMin[i]) {
            gDir[i] = -gDir[i];
        }
        gCol[i] += gDir[i];
    }
}

/* returns 1 if a guard noticed (and hurt) the player */
int checkGuards(int bank) {
    int i, dr, dc, stealth, chance;
    int caught = 0;
    for (i = 0; i < bankGuards[bank]; i++) {
        if (gFrozen[i] > 0) continue;
        dr = abs(gRow[i] - pr);
        dc = abs(gCol[i] - pc);
        if (dr <= 1 && dc <= 1) {
            stealth = teamStat(STEALTH) + itemOwned[BOOTS] + reconDone;
            chance = 45 + stealth * 9 - bankDifficulty[bank] * 6;
            if (randRange(1, 100) <= chance) {
                printf("   You freeze in the shadows as a guard passes. Too close!\n");
                gFrozen[i] = 1;
            } else {
                printf("   A guard spots you! A scuffle breaks out!\n");
                hurt(randRange(10, 20));
                addHeat(20);
                gFrozen[i] = 2;
                caught = 1;
            }
        }
    }
    return caught;
}

void checkCameras(int bank) {
    int i, r, blocked;
    if (itemOwned[JAMMER] > 0 || !cameraActive()) return;
    for (i = 0; i < bankDifficulty[bank]; i++) {
        if (camCol[i] == pc) {
            blocked = 0;
            for (r = 1; r < pr; r++) {
                if (map[r][pc] == '#') blocked = 1;
            }
            if (!blocked) {
                printf("   A camera sweeps over you! Heat +15.\n");
                addHeat(15);
            }
        }
    }
}

/* ========================== HEIST: ENTRY =========================== */

/* returns 0 = aborted, 1 = reached the vault, 2 = arrested/downed */
int infiltrate(int bank) {
    char cmd;
    int nr, nc, acted, target, i;
    int numGuards = bankGuards[bank];

    loadMap();
    printHeader("STEALTH ENTRY");
    printf("  Find the keycard (K), open the door (D), reach the vault (V).\n");
    printf("  Grab loose cash ($) if you dare. Guards stop to look around;\n");
    printf("  cameras blink on and off.\n");

    while (1) {
        drawMap(bank);
        printf("\n   Turn %d | Heat %d | Health %d | Keycard: %s | Smoke: %d\n",
               heistTurns, heat, health, hasKey ? "YES" : "no", itemOwned[SMOKE]);
        printf("   Move w/a/s/d, wait x, smoke bomb b, abort q: ");
        cmd = readChar();

        acted = 1;
        nr = pr;
        nc = pc;
        switch (cmd) {
            case 'w': nr--; break;
            case 's': nr++; break;
            case 'a': nc--; break;
            case 'd': nc++; break;
            case 'x': printf("   You hold still and listen...\n"); break;
            case 'b':
                if (itemOwned[SMOKE] > 0) {
                    itemOwned[SMOKE]--;
                    for (i = 0; i < numGuards; i++) gFrozen[i] = 3;
                    printf("   Smoke fills the room. Guards stumble around blindly!\n");
                } else {
                    printf("   You have no smoke bombs.\n");
                    acted = 0;
                }
                break;
            case 'q':
                printf("   You abort the job and slip out the way you came.\n");
                return 0;
            default:
                printf("   Unknown command.\n");
                acted = 0;
                break;
        }

        if (acted && (nr != pr || nc != pc)) {
            target = map[nr][nc];
            if (target == '#') {
                printf("   A solid wall.\n");
                acted = 0;
            } else if (target == 'D') {
                if (hasKey) {
                    printf("   The keycard clicks. The door swings open.\n");
                    map[nr][nc] = '.';
                } else if (itemOwned[LOCKPICK] > 0) {
                    if (randRange(1, 100) <= 50 + teamStat(STEALTH) * 8) {
                        printf("   Your lockpicks do the job. The door is open.\n");
                        map[nr][nc] = '.';
                    } else {
                        printf("   The pick slips. A loud clack echoes! Heat +10.\n");
                        addHeat(10);
                    }
                } else {
                    printf("   Locked. You need a keycard (or a lockpick set).\n");
                    acted = 0;
                }
            } else if (target == 'V') {
                printf("\n   You reach the vault door!\n");
                return 1;
            } else {
                pr = nr;
                pc = nc;
                if (target == 'K') {
                    hasKey = 1;
                    map[nr][nc] = '.';
                    printf("   You lift the security keycard!\n");
                } else if (target == '$') {
                    i = randRange(500, 1500) * bankDifficulty[bank];
                    pickupCash += i;
                    map[nr][nc] = '.';
                    printf("   You pocket $%d from an unattended drawer.\n", i);
                }
            }
        }

        if (acted) {
            heistTurns++;
            if (heistTurns > 40) {
                printf("   Sirens in the distance... Heat +3.\n");
                addHeat(3);
            } else if (heistTurns % 8 == 0) {
                printf("   Police radio chatter crackles. Heat +5.\n");
                addHeat(5);
            }
            moveGuards(numGuards);
            checkGuards(bank);
            checkCameras(bank);
            if (heat >= 100 || health <= 0) {
                return 2;
            }
        }
    }
}

/* the loud alternative: storm the lobby. returns 1 or 2 like infiltrate */
int loudAssault(int bank) {
    int guardsLeft = bankGuards[bank];
    int choice, chance, round = 0;

    printHeader("LOUD ASSAULT");
    printf("  You kick in the doors. Customers scream. %d guards respond!\n", guardsLeft);
    addHeat(25);

    while (guardsLeft > 0) {
        round++;
        printf("\n  Round %d | Guards left: %d | Health: %d | Heat: %d\n",
               round, guardsLeft, health, heat);
        printf("  1) Open fire      (muscle check)\n");
        printf("  2) Threaten the hostages (guards may surrender)\n");
        printf("  3) Throw a smoke bomb   (skips return fire)\n");
        choice = readInt(1, 3);

        if (choice == 1) {
            chance = 45 + teamStat(MUSCLE) * 10 - bankDifficulty[bank] * 5;
            if (randRange(1, 100) <= chance) {
                guardsLeft--;
                printf("   A guard goes down.\n");
            } else {
                printf("   Your shots go wide.\n");
            }
            addHeat(5);
        } else if (choice == 2) {
            chance = 35 + teamStat(MUSCLE) * 8;
            if (randRange(1, 100) <= chance) {
                printf("   The remaining guards drop their weapons!\n");
                guardsLeft = 0;
                addHeat(10);
                break;
            }
            printf("   The guards call your bluff!\n");
        } else {
            if (itemOwned[SMOKE] > 0) {
                itemOwned[SMOKE]--;
                printf("   Smoke chokes the lobby. You use the cover to take one down.\n");
                guardsLeft--;
                continue;
            }
            printf("   You fumble for a smoke bomb you don't have!\n");
        }

        if (guardsLeft > 0) {
            printf("   The guards return fire!\n");
            hurt(randRange(0, 7) * guardsLeft);
        }
        if (health <= 0 || heat >= 100) {
            return 2;
        }
    }
    printf("\n   The lobby is yours. On to the vault!\n");
    if (health <= 0 || heat >= 100) return 2;
    return 1;
}

/* ========================== HEIST: VAULT =========================== */

/* returns loot gained (>= 0), or -1 if you were caught inside */
int crackVault(int bank) {
    int i, d, attempts, used = 0, solved = 0;
    int guess, digit[3], exact, common, misplaced;
    int codeCount[7], guessCount[7];
    int loot = 0, maxBags, bags = 0, bagValue, bagGot, choice;

    for (i = 0; i < 3; i++) vaultCode[i] = randRange(1, 6);

    attempts = 9 + teamStat(HACK) + 2 * itemOwned[LAPTOP] - bankDifficulty[bank];
    if (attempts < 4) attempts = 4;

    printHeader("THE VAULT");
    printf("  The keypad wants a 3-digit code. Each digit is 1 to 6.\n");
    printf("  After each guess you learn:\n");
    printf("    EXACT     = right digit in the right place\n");
    printf("    MISPLACED = right digit in the wrong place\n");
    printf("  You have %d attempts. Every failure adds heat.\n", attempts);
    if (reconDone) {
        printf("  Your casing notes say: digit #%d is %d.\n",
               reconPos + 1, vaultCode[reconPos]);
    }

    while (!solved && used < attempts) {
        printf("\n  Attempts left: %d   Enter your guess (e.g. 316)\n", attempts - used);
        guess = readInt(111, 666);
        digit[0] = guess / 100;
        digit[1] = (guess / 10) % 10;
        digit[2] = guess % 10;
        if (digit[0] < 1 || digit[0] > 6 || digit[1] < 1 || digit[1] > 6 ||
            digit[2] < 1 || digit[2] > 6) {
            printf("   Every digit must be between 1 and 6.\n");
            continue;
        }
        used++;

        for (d = 0; d < 7; d++) {
            codeCount[d] = 0;
            guessCount[d] = 0;
        }
        exact = 0;
        for (i = 0; i < 3; i++) {
            if (digit[i] == vaultCode[i]) exact++;
            codeCount[vaultCode[i]]++;
            guessCount[digit[i]]++;
        }
        common = 0;
        for (d = 1; d <= 6; d++) {
            common += (codeCount[d] < guessCount[d]) ? codeCount[d] : guessCount[d];
        }
        misplaced = common - exact;

        printf("   EXACT: %d   MISPLACED: %d\n", exact, misplaced);
        if (exact == 3) {
            solved = 1;
        } else {
            addHeat(4);
            if (heat >= 100) {
                printf("   The silent alarm trips! Police are on the doorstep!\n");
                return -1;
            }
        }
    }

    if (!solved) {
        printf("\n   The keypad locks out. The vault stays shut!\n");
        printf("   The alarm blares. Heat +25.\n");
        addHeat(25);
        if (heat >= 100) return -1;
        return 0;
    }

    printf("\n   *CLUNK* The vault door swings open!\n");
    maxBags = 3 + teamStat(MUSCLE);
    if (maxBags > 6) maxBags = 6;
    bagValue = bankVault[bank] / 6;
    printf("  You can carry up to %d bags (about $%d each).\n", maxBags, bagValue);
    printf("  Every bag adds heat and risks tripping the proximity alarm.\n");

    for (i = 0; i < maxBags; i++) {
        printf("\n  Loot so far: $%d  Heat: %d  -> 1) Grab bag #%d  2) Leave now\n",
               loot, heat, i + 1);
        choice = readInt(1, 2);
        if (choice == 2) break;
        bagGot = bagValue * randRange(80, 120) / 100;
        loot += bagGot;
        bags++;
        addHeat(6);
        printf("   You stuff $%d into the duffel.\n", bagGot);
        if (randRange(1, 100) <= bags * 6 + bankDifficulty[bank] * 3) {
            printf("   The proximity alarm shrieks! Time to run! Heat +20.\n");
            addHeat(20);
            break;
        }
    }
    if (heat >= 100) return -1;
    return loot;
}

/* ========================== HEIST: ESCAPE ========================== */

/* returns 1 if you got away, 0 if the police caught you */
int escapeRun(int bank, int loot) {
    int distance, round = 0, choice, delta, i, valid;

    distance = 5 + teamStat(DRIVING) + 2 * itemOwned[ENGINE] - heat / 20;
    if (distance < 2) distance = 2;

    printHeader("THE GETAWAY");
    printf("  Loot in the trunk: $%d. Sirens are closing in.\n", loot);
    printf("  Get your lead to 10 to vanish. Let it hit 0 and you're done.\n");

    while (distance > 0 && distance < 10 && round < 8) {
        printf("\n  Round %d/8  Lead: [", round + 1);
        for (i = 0; i < 10; i++) putchar(i < distance ? '=' : '.');
        printf("] %d   Health: %d\n", distance, health);
        printf("  1) Floor it down the highway\n");
        printf("  2) Cut through back alleys (gamble)\n");
        printf("  3) Smoke bomb out the window   (have %d)\n", itemOwned[SMOKE]);
        printf("  4) Swap to a stashed car       ($3000)\n");
        choice = readInt(1, 4);

        valid = 1;
        delta = 0;
        switch (choice) {
            case 1:
                delta = randRange(1, 3) + teamStat(DRIVING) / 2;
                printf("   Engine screaming, you tear down the highway.\n");
                if (randRange(1, 100) <= 30) {
                    printf("   Gunfire rakes the car!\n");
                    hurt(10);
                }
                break;
            case 2:
                if (randRange(1, 2) == 1) {
                    delta = 4;
                    printf("   The alleys swallow you. Cops overshoot the turn!\n");
                } else {
                    delta = -2;
                    printf("   Dead end! You scrape the walls reversing out.\n");
                }
                break;
            case 3:
                if (itemOwned[SMOKE] > 0) {
                    itemOwned[SMOKE]--;
                    delta = 4;
                    printf("   A smoke wall blinds the pursuit cars.\n");
                } else {
                    printf("   You have no smoke bombs left!\n");
                    valid = 0;
                }
                break;
            default:
                if (cash >= 3000) {
                    cash -= 3000;
                    delta = 3;
                    addHeat(-10);
                    printf("   You ditch the hot car for a clean one.\n");
                } else {
                    printf("   You can't afford the swap.\n");
                    valid = 0;
                }
                break;
        }
        if (!valid) continue;

        round++;
        delta -= randRange(0, 2);
        if (bankDifficulty[bank] >= 3) delta--;
        distance += delta;
        if (distance > 10) distance = 10;
        if (health <= 0) return 0;
    }

    if (distance >= 10 || (distance >= 7 && round >= 8)) {
        printf("\n   You lose the police in the city. You're free!\n");
        return 1;
    }
    printf("\n   The cruisers box you in. It's over.\n");
    return 0;
}

/* ========================== HEIST: DRIVER ========================== */

void handleArrest(int bank) {
    printHeader("BUSTED");
    printf("  Cuffs click around your wrists. The loot is gone.\n");
    strikes++;
    if (cash >= BAIL_COST) cash -= BAIL_COST; else cash = 0;
    printf("  Bail posted: $%d.  Strikes: %d / %d\n", BAIL_COST, strikes, MAX_STRIKES);
    heat = 40;
    health = 60;
    addHistory(bank, -1);
    advanceDay(3);
}

void runHeist(void) {
    int i, choice, bank, method, result;
    int loot, cut, net;

    printHeader("CHOOSE YOUR TARGET");
    for (i = 0; i < NUM_BANKS; i++) {
        printf("  %d) %-24s vault $%-7d guards %d  difficulty %d",
               i + 1, bankName[i], bankVault[i], bankGuards[i], bankDifficulty[i]);
        if (rep < bankRepReq[i]) {
            printf("  [LOCKED: need rep %d]", bankRepReq[i]);
        }
        printf("\n");
    }
    printf("  0) Back\n");
    choice = readInt(0, NUM_BANKS);
    if (choice == 0) return;
    bank = choice - 1;
    if (rep < bankRepReq[bank]) {
        printf("   Nobody will back you for that job yet. Build more reputation.\n");
        return;
    }
    if (health < 40) {
        printf("   You are too hurt to work. Lay low first.\n");
        return;
    }
    if (crewCount() == 0) {
        printf("   Warning: you're going in with no crew. Continue? 1) Yes 2) No\n");
        if (readInt(1, 2) == 2) return;
    }

    /* PLANNING PHASE */
    reconDone = 0;
    printf("\n  Case the joint first? $1000. (+1 stealth, and you learn one\n");
    printf("  digit of the vault code.)  1) Yes  2) No\n");
    if (readInt(1, 2) == 1) {
        if (cash >= 1000) {
            cash -= 1000;
            reconDone = 1;
            reconPos = randRange(0, 2);
            printf("   You spend a day watching the bank. Notes taken.\n");
            advanceDay(1);
        } else {
            printf("   You can't afford it. You'll go in blind.\n");
        }
    }

    printf("\n  How do you enter?\n");
    printf("  1) Stealth: sneak in through the service entrance\n");
    printf("  2) Loud: storm the lobby with guns drawn\n");
    method = readInt(1, 2);

    if (method == 1) {
        result = infiltrate(bank);
    } else {
        result = loudAssault(bank);
        pickupCash = 0;
    }

    if (result == 2) {
        handleArrest(bank);
        return;
    }
    if (result == 0) {
        printf("  You leave with $%d from the drawers and your dignity.\n", pickupCash);
        cash += pickupCash;
        totalStolen += pickupCash;
        addHistory(bank, pickupCash);
        advanceDay(1);
        return;
    }

    loot = crackVault(bank);
    if (loot < 0) {
        handleArrest(bank);
        return;
    }
    loot += pickupCash;

    if (escapeRun(bank, loot) == 0) {
        handleArrest(bank);
        return;
    }

    /* PAYOUT */
    cut = loot * crewCutTotal() / 100;
    net = loot - cut;
    cash += net;
    totalStolen += net;
    rep += bankDifficulty[bank] + 1;
    heistsWon++;
    addHeat(15);
    addHistory(bank, net);

    printHeader("PAYDAY");
    printf("  Total haul : $%d\n", loot);
    printf("  Crew cut   : $%d (%d%%)\n", cut, crewCutTotal());
    printf("  Your take  : $%d\n", net);
    printf("  Reputation +%d\n", bankDifficulty[bank] + 1);
    advanceDay(1);
}

/* ============================ CAMPAIGN ============================= */

/* 0 = keep playing, 1 = win, 2 = prison, 3 = out of time */
int checkEnding(void) {
    if (cash >= GOAL_CASH)      return 1;
    if (strikes >= MAX_STRIKES) return 2;
    if (day > MAX_DAYS)         return 3;
    return 0;
}

void finalScreen(int ending) {
    printHeader("THE END");
    if (ending == 1) {
        printf("  You did it. $%d in a duffel, a one-way ticket, and no\n", cash);
        printf("  forwarding address. The legend of the last score begins.\n");
    } else if (ending == 2) {
        printf("  Three strikes. The judge doesn't blink. You'll be\n");
        printf("  telling heist stories to cellmates for a long, long time.\n");
    } else if (ending == 3) {
        printf("  Time's up. The people you owe have found you.\n");
        printf("  You were $%d short.\n", GOAL_CASH - cash);
    } else {
        printf("  You walk away from the life, for better or worse.\n");
    }
    printf("\n  Final stats: day %d, cash $%d, heists won %d, stolen $%d\n",
           day, cash, heistsWon, totalStolen);
    if (totalStolen >= 400000)      printf("  Rank: MASTERMIND\n");
    else if (totalStolen >= 200000) printf("  Rank: PROFESSIONAL\n");
    else if (totalStolen >= 50000)  printf("  Rank: APPRENTICE\n");
    else                            printf("  Rank: PETTY THIEF\n");
}

int main(void) {
    int choice, ending = 0;

    srand((unsigned int) time(NULL));
    showTitle();
    pressEnter();

    while (ending == 0) {
        printHeader("HIDEOUT");
        printf("  Day %d/%d  Cash $%d/$%d  Heat %d%%  Health %d  Rep %d  Strikes %d/%d\n",
               day, MAX_DAYS, cash, GOAL_CASH, heat, health, rep, strikes, MAX_STRIKES);
        printf("\n  1) Plan a heist\n");
        printf("  2) Hire crew\n");
        printf("  3) Black market\n");
        printf("  4) Lay low\n");
        printf("  5) Status & record\n");
        printf("  6) Quit\n");
        choice = readInt(1, 6);

        switch (choice) {
            case 1: runHeist(); break;
            case 2: hireCrew(); break;
            case 3: shopMenu(); break;
            case 4: layLow(); break;
            case 5: showStatus(); pressEnter(); break;
            case 6:
                printf("  Really quit? 1) Yes  2) No\n");
                if (readInt(1, 2) == 1) {
                    finalScreen(4);
                    return 0;
                }
                break;
        }
        ending = checkEnding();
    }

    finalScreen(ending);
    return 0;
}
