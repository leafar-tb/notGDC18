asm (".code16gcc;"
     "call dosmain;"
     "call exit;");

// gcc doesn't allow clobbering input registers
// so we just do an empty asm for clobbering those
// (original asm probably needs to be volatile)
#define CLOBBERED(...) asm volatile ( "" : : : __VA_ARGS__)

#define REPEAT(N_TIMES) for(uint __x = 0; __x < N_TIMES; ++__x)

// these aren't headers as such, just code moved to different files for organisation
#include "types.h"
#include "string.h"
#include "rand.h"
#include "display.h"
#include "text_io.h"
#include "time.h"

//###################################################

void exit() {
    textMode();
    asm volatile (
        "mov   $0x4C00, %ax;"
        "int   $0x21;"
    );
}

//###################################################

typedef enum {
    WORKER,
    GATHERER
} BeeType;

struct {
    ushort population;
    ushort workers;
    ushort gatherers;
    BeeType draftTo;

    ushort growRate;

    ushort cells;
    ushort plannedCells;
    ushort honeyStores;
} hive = {
    // init with defaults
    .population = 1000,
    .workers = 300,
    .gatherers = 700,
    .draftTo = GATHERER,

    .growRate = 100,

    .cells = 500,
    .plannedCells = 0,
    .honeyStores = 200
};

//###################################################

// ~18 clock ticks per second
#define CLOCK_TICKS_PER_GAME_TICK ( 20 * 18 )

// prevent overflow in add
static ushort checkedAdd(ushort* val, ushort inc, ushort limit) {
    if(limit-inc > *val)
        *val += inc;
    else
        *val = limit;
}

static uint lastGameTick;

// Wikipedia:
// average population of a healthy hive in midsummer may be as high as 40,000 to 80,000 bees.
// On average during the year, about one percent of a colony's worker bees die naturally per day.
// At the height of the season, the queen may lay over 2,500 eggs per day
// honey consumed during the winter [..] ranges in temperate climates from 15 to 50 kilograms

static void gameTick() {
    //TODO timer will eventually overflow
    if(lastGameTick + CLOCK_TICKS_PER_GAME_TICK > clockTicks())
        return;
    lastGameTick = clockTicks();

    ushort newHoney = hive.gatherers / 2;
    ushort honeyConsumed = CEIL_DIV(hive.population, 8);
    if(newHoney >= honeyConsumed) { // net gain
        checkedAdd(&hive.honeyStores, newHoney - honeyConsumed, hive.cells);
    } else if(honeyConsumed - newHoney <= hive.honeyStores) { // balance with stores
        hive.honeyStores -= honeyConsumed - newHoney;
    } else { // not enough
        ushort missingHoney = honeyConsumed - newHoney - hive.honeyStores;
        hive.honeyStores = 0;

        ushort starved = MIN(missingHoney * 8, hive.population);
        hive.population -= starved;

        if(starved <= hive.workers) {
            hive.workers -= starved;
        } else {
            hive.gatherers -= starved - hive.workers;
            hive.workers = 0;
        }

        if(hive.population == 0)
            exit(); // TODO proper game over screen
    }

    // new bees need honey and worker's care
    ushort newBees = MIN(hive.growRate, hive.honeyStores / 2);
    newBees = MIN(newBees, hive.workers);
    newBees = MIN(newBees, MAX_USHORT - hive.population); // keep limit

    hive.population += newBees;
    switch(hive.draftTo) {
        case WORKER: hive.workers += newBees; break;
        case GATHERER: hive.gatherers += newBees; break;
    }
    hive.honeyStores -= newBees * 2;
    ushort occupiedWorkers = newBees;

    // building new cells needs workers, which consume extra honey in the effort
    ushort newCells = MIN(hive.plannedCells - hive.cells, hive.workers - occupiedWorkers);
    if(CEIL_DIV(newCells, 4) > hive.honeyStores)
        newCells = hive.honeyStores * 4;
    newCells = MIN(newCells, MAX_USHORT - hive.cells);
    hive.cells += newCells;
    hive.honeyStores -= CEIL_DIV(newCells, 4);
}

//###################################################

#define SKY_HEIGHT 50

static void drawClouds() {
    REPEAT(6) {
        fillArea(randRange(0, SCREEN_WIDTH), randRange(0, SKY_HEIGHT-15), randRange(10, 50), randRange(5, 15), White);
    }
}

static void drawFlowers() {
    static const byte flowerColours[8] = {Red, Blue, Magenta, Yellow, Orange, LightMagenta, LightRed, LightBlue};
    REPEAT(40) {
        short x = randRange(0, SCREEN_WIDTH-5);
        short y = randRange(SKY_HEIGHT, SCREEN_HEIGHT-5);
        fillArea(x, y, 5, 5, flowerColours[stb_randLCG() & 7]);
        setPixel(x+2, y+2, Black);
    }
}

static void drawTrees() {
    static const short crownDim = 30;
    static const short trunkH = 50;
    static const short trunkW = 14;

    short x = randRange(45, 55);
    REPEAT(3) {
        short y = randRange(0, SCREEN_HEIGHT - (crownDim+trunkH));
        fillArea(x, y, crownDim, crownDim, LightGreen); //crown
        fillArea(x + (crownDim-trunkW)/2, y+crownDim, trunkW, trunkH, Brown); // trunk
        x <<= 1;
    }
}

static void drawSpring() {
    fillArea(0, 0, SCREEN_WIDTH, SKY_HEIGHT, LightBlue);
    fillArea(20, 20, 20, 20, Yellow); // Sun
    drawClouds();
    fillArea(0, SKY_HEIGHT, SCREEN_WIDTH, SCREEN_HEIGHT-SKY_HEIGHT, Green);
    drawFlowers();
    drawTrees();
}

#define HIVE_WIDTH 30
#define HIVE_H_ROWS 9
#define HIVE_HEIGHT (HIVE_H_ROWS * 4)
#define HIVE_CENTER_Y 120

static void drawHive() {
    fillArea((SCREEN_WIDTH - HIVE_WIDTH)/2, HIVE_CENTER_Y - HIVE_HEIGHT/2, HIVE_WIDTH, HIVE_HEIGHT, Brown);
    for(uint i = 0; i < HIVE_H_ROWS; ++i) {
        fillArea((SCREEN_WIDTH - HIVE_WIDTH)/2, HIVE_CENTER_Y - HIVE_HEIGHT/2 + i*4, HIVE_WIDTH, 2, Brown);
        fillArea((SCREEN_WIDTH - HIVE_WIDTH)/2, HIVE_CENTER_Y - HIVE_HEIGHT/2 + 2 + i*4, HIVE_WIDTH, 2, Orange);
    }
    fillArea(SCREEN_WIDTH/2 - 2, HIVE_CENTER_Y+HIVE_HEIGHT/2 - 6, 4, 6, Black);
}

#define N_SCREEN_BEES 50
static ushort screenBees[N_SCREEN_BEES] = {[0 ... N_SCREEN_BEES-1] = (HIVE_CENTER_Y + HIVE_HEIGHT/2) * SCREEN_WIDTH + SCREEN_WIDTH/2};

static void drawBees() {
    static const short moves[8] = { +1, -1, +SCREEN_WIDTH, -SCREEN_WIDTH, +SCREEN_WIDTH+1, -SCREEN_WIDTH-1, +SCREEN_WIDTH-1, -SCREEN_WIDTH+1};

    beginDrawDirect();
    for(ushort b = 0; b < N_SCREEN_BEES; ++b) {
        screenBees[b] += moves[stb_randLCG() & 3]; //
        asm ( "mov %1, %%es:(%%bx);"
              : : "b"(screenBees[b]), "r"((byte)Yellow) :
        );
    }
    endDrawDirect();
}

//###################################################

static void printLabeled(char* str, ushort n) {
    print(str);
    print_ushort(n);
    endl();
}

void printHiveStatus() {
    printLabeled("Population $", hive.population);
    printLabeled("Workers $", hive.workers);
    printLabeled("Gatherers $", hive.gatherers);
    printLabeled("Grow Rate $", hive.growRate);
    printLabeled("Cells $", hive.cells);
    printLabeled("Honey stored $", hive.honeyStores);
}

//###################################################

typedef struct {
    char* cmdStr;
    void (*callback)(void);
    bool exactMatch; // is cmdStr a prefix or the full command?
} Command;

Command consoleCommands[] = {
    {"STAT$", &printHiveStatus, true}
};

static void console() {
    #define CMD consoleCommands
    textMode();
    println("What do you want to do?$");
    while(true) {
        prompt();
        makeUpperCase(USER_INPUT);

        bool matched = false;
        for(uint i = 0; i < sizeof(consoleCommands)/sizeof(Command); ++i) {
            if(CMD[i].exactMatch && strEquals(USER_INPUT, CMD[i].cmdStr)) {
                (*CMD[i].callback)();
                matched = true;
                break;
            }
            if(!CMD[i].exactMatch && startsWith(USER_INPUT, CMD[i].cmdStr)) {
                (*CMD[i].callback)();
                matched = true;
                break;
            }
        }

        if( strEquals(USER_INPUT, "DONE$") )
            break;
        if( strEquals(USER_INPUT, "EXIT$") )
            exit();
        if(!matched)
            println("Sorry, I didn't understand that.$");
    }

    lastGameTick = clockTicks(); // don't count time in menu
    videoMode();
    #undef CMD
}

//###################################################

void dosmain(void) {
    displayInit();
    stb__rand_seed = clockTicks();
    lastGameTick = clockTicks();

    drawSpring();
    drawHive();

    videoMode();
    while(true) {
        gameTick();

        vsync();
        showDisplayBuffer();
        drawBees();

        while(keyInputAvailable()) {
            if(readASCII_blocking() == ' ')
                console();
        }

        //waitMillis(50);
    }
}
