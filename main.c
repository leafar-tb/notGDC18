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
    GATHERER,
    NO_WORK,
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
    .plannedCells = 500,
    .honeyStores = 200
};

static ushort unassignedBees() {
    return hive.population - ( hive.workers + hive.gatherers );
}

//###################################################

static enum {
    SPRING,
    SUMMER,
    AUTUMN,
    WINTER
} season = SPRING;

static char* seasonStr[] = { [SPRING] = "Spring$", [SUMMER] = "Summer$", [AUTUMN] = "Autumn$", [WINTER] = "Winter$" };

#define GAME_TICKS_PER_SEASON 60
static ushort seasonTicks; // game ticks elapsed in current season

//###################################################

#define HIVE_WIDTH 30
#define HIVE_H_ROWS 9
#define HIVE_HEIGHT (HIVE_H_ROWS * 4)
#define HIVE_CENTER_Y 120

#define N_SCREEN_BEES 256
static ushort screenBees[N_SCREEN_BEES] = {[0 ... N_SCREEN_BEES-1] = (HIVE_CENTER_Y + HIVE_HEIGHT/2) * SCREEN_WIDTH + SCREEN_WIDTH/2};

//###################################################

static void drawSeasonBackdrop();
static void drawHive();
static void console();

void nextSeason() {
    season = ( season + 1 ) % 4;
    seasonTicks = 0;

    drawSeasonBackdrop();
    drawHive();

    if(season == WINTER) { // reset bee position for spring
        for(ushort b = 0; b < N_SCREEN_BEES; ++b)
            screenBees[b] = (HIVE_CENTER_Y + HIVE_HEIGHT/2) * SCREEN_WIDTH + SCREEN_WIDTH/2;
    }
}

//###################################################

// prevent overflow in add; return 'spillage'
static ushort checkedAdd(ushort* val, ushort inc, ushort limit) {
    if(limit-inc > *val) {
        *val += inc;
        return 0;
    } else {
        inc -= limit - *val;
        *val = limit;
        return inc;
    }
}

// prevent underflow in sub; return 'spillage'
static ushort checkedSub(ushort* val, ushort dec) {
    if(dec > *val) {
        dec -= *val;
        *val = 0;
        return dec;
    } else {
        *val -= dec;
        return 0;
    }
}

static void printLabeled(char* str, ushort n) {
    print(str);
    print_ushort(n);
    endl();
}

//###################################################

bool evt_birdFlock() {
    if(season == WINTER)
        return false;

    textMode();
    println("A flock of birds attacked our gatherers!$");
    ushort casualties = 234 + ( stb_randLCG() & 0xfff);
    casualties = MIN(hive.gatherers/2, casualties); // limit damage
    printLabeled("Casualties: $", casualties);
    hive.population -= casualties;
    hive.gatherers -= casualties;

    return true;
}

bool evt_bear() {
    if(hive.honeyStores < 1000)
        return false;

    textMode();
    println("A bear raided our hive!$");
    ushort casualties = 1234 + ( stb_randLCG() & 0x2fff);
    casualties = MIN(hive.workers/2, casualties); // limit damage
    printLabeled("Casualties: $", casualties);
    hive.population -= casualties;
    hive.workers -= casualties;

    ushort spoils = 999 + ( stb_randLCG() & 0xfff);
    spoils = MIN( 3 * (hive.honeyStores/4), spoils );
    printLabeled("Honey lost: $", spoils);
    hive.honeyStores -= spoils;

    ushort damage = spoils + ( stb_randLCG() & 0x1ff);
    damage = MIN(3*(hive.cells/4), damage);
    printLabeled("Cells destroyed: $", damage);
    hive.cells -= damage;

    return true;
}

bool evt_goodHarvest() {
    if(season == WINTER)
        return false;

    textMode();
    println("The good weather increased our honey harvest!$");
    ushort extraHoney = hive.gatherers / 4;
    // TODO could spill a lot
    checkedAdd(&hive.honeyStores, extraHoney, hive.cells);

    return true;
}

typedef bool (*eventFun)(void);

static eventFun events[] = {&evt_birdFlock, &evt_bear, &evt_goodHarvest};

// chance that any given event occurs; about 1%
#define EVENT_PROBABILITY ( MAX_UINT / 128 )

//###################################################

// ~18 clock ticks per second
#define CLOCK_TICKS_PER_GAME_TICK ( 10 * 18 )

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

    uint p = stb_randLCG();
    bool eventTriggered = false;
    for(ushort e = 0; e < sizeof(events)/sizeof(eventFun); ++e) {
        if(p < EVENT_PROBABILITY) {
            eventTriggered = (*(events[e]))();
            break;
        } else
            p -= EVENT_PROBABILITY;
    }


    ushort newHoney;
    switch(season) {
        case SPRING: newHoney = hive.gatherers / 2; break;
        case SUMMER: newHoney = hive.gatherers; break;
        case AUTUMN: newHoney = hive.gatherers / 4; break;
        case WINTER: newHoney = 0; break;
    }

    ushort honeyConsumed = CEIL_DIV(hive.population, 8);
    // clear production with usage
    honeyConsumed = checkedSub(&newHoney, honeyConsumed);
    // add remaining net gain; keep spillage
    newHoney = checkedAdd(&hive.honeyStores, newHoney, hive.cells);
    // sub remaining net loss
    honeyConsumed = checkedSub(&hive.honeyStores, honeyConsumed);
    // stores were too small
    if(honeyConsumed) {
        ushort starved = MIN(honeyConsumed * 8, hive.population);
        hive.population -= starved;

        starved = checkedSub(&hive.workers, starved);
        checkedSub(&hive.gatherers, starved);

        if(!eventTriggered) // don't want to clear event text
            textMode();
        eventTriggered = true;
        printLabeled("We do not have enough honey. Bees starved: $", starved);
    }

    if(hive.population == 0) {
        textMode();
        println("Your hive died.$");
        println("(Press any key to quit.)$");
        readASCII_blocking();
        exit();
    }

    // new bees need honey and worker's care
    ushort newBees = MIN(hive.growRate, hive.honeyStores / 2);
    newBees = MIN(newBees, hive.workers);
    newBees = MIN(newBees, MAX_USHORT - hive.population); // keep limit

    hive.population += newBees;
    switch(hive.draftTo) {
        case WORKER: hive.workers += newBees; break;
        case GATHERER: hive.gatherers += newBees; break;
        case NO_WORK: break;
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

    // if there was spillage earlier, try storing it now
    newHoney = checkedAdd(&hive.honeyStores, newHoney, hive.cells);
    if(newHoney) {
        if(!eventTriggered) // don't want to clear event text
            textMode();
        eventTriggered = true;
        println("Not all gathered honey could be stored. Consider planning more cells.$");
    }

    // some bees die naturally
    ushort died = hive.workers / 128;
    hive.workers -= died;
    hive.population -= died;

    died = hive.gatherers / 128;
    hive.gatherers -= died;
    hive.population -= died;

    died = unassignedBees() / 128;
    hive.population -= died;

    ++seasonTicks;
    if(seasonTicks >= GAME_TICKS_PER_SEASON) {
        nextSeason();
        if(!eventTriggered) // don't want to clear event text
            textMode();
        eventTriggered = true;
        print("It is now $");
        println(seasonStr[season]);
    }

    if(eventTriggered)
        console();
}

//###################################################

#define SKY_HEIGHT 50

static void drawClouds() {
    static const ushort countBy[] = { [SPRING] = 6, [SUMMER] = 2, [AUTUMN] = 7, [WINTER] = 10 };
    REPEAT(countBy[season]) {
        byte colour = season == WINTER ? LightGray : White;
        fillArea(randRange(0, SCREEN_WIDTH), randRange(0, SKY_HEIGHT-15), randRange(10, 50), randRange(5, 15), colour);
    }
}

static void drawFlowers() {
    static const byte flowerColours[8] = {Red, Blue, Magenta, Yellow, Orange, LightMagenta, LightRed, LightBlue};
    static const ushort countBy[] = { [SPRING] = 40, [SUMMER] = 80, [AUTUMN] = 20, [WINTER] = 0 };

    REPEAT(countBy[season]) {
        short x = randRange(0, SCREEN_WIDTH-5);
        short y = randRange(SKY_HEIGHT+1, SCREEN_HEIGHT-5);
        fillArea(x, y, 5, 5, flowerColours[stb_randLCG() & 7]);
        setPixel(x+2, y+2, Black);
    }
}

static uint treeSeed;
static void drawTrees() {
    static const short crownDim = 30;
    static const short trunkH = 50;
    static const short trunkW = 14;
    static const byte autumnColours[] = {Red, Yellow, Yellow2, Orange};

    // fixed seed, so trees don't move between seasons
    uint curSeed = stb__rand_seed;
    stb__rand_seed = treeSeed;

    short x = randRange(45, 55);
    REPEAT(3) {
        short y = randRange(0, SCREEN_HEIGHT - (crownDim+trunkH));
        if(season != WINTER) {
            byte colour = LightGreen;
            if(season == AUTUMN)
                colour = autumnColours[y & 3];
            fillArea(x, y, crownDim, crownDim, colour); //crown
        }
        fillArea(x + (crownDim-trunkW)/2, y+crownDim, trunkW, trunkH, Brown); // trunk
        x *= 2;
    }

    stb__rand_seed = curSeed;
}

static void drawSeasonBackdrop() {
    static const byte skyColour[] = { [SPRING] = LightBlue, [SUMMER] = LightBlue, [AUTUMN] = Blue, [WINTER] = Blue };
    static const byte groundColour[] = { [SPRING] = Green, [SUMMER] = Green, [AUTUMN] = Green, [WINTER] = White };

    fillArea(0, 0, SCREEN_WIDTH, SKY_HEIGHT, skyColour[season]);
    fillArea(20, 20, 20, 20, Yellow2); // Sun
    drawClouds();
    fillArea(0, SKY_HEIGHT, SCREEN_WIDTH, SCREEN_HEIGHT-SKY_HEIGHT, groundColour[season]);
    drawFlowers();
    drawTrees();
}

static void drawHive() {
    fillArea((SCREEN_WIDTH - HIVE_WIDTH)/2, HIVE_CENTER_Y - HIVE_HEIGHT/2, HIVE_WIDTH, HIVE_HEIGHT, Brown);
    for(uint i = 0; i < HIVE_H_ROWS; ++i) {
        fillArea((SCREEN_WIDTH - HIVE_WIDTH)/2, HIVE_CENTER_Y - HIVE_HEIGHT/2 + i*4, HIVE_WIDTH, 2, Brown);
        fillArea((SCREEN_WIDTH - HIVE_WIDTH)/2, HIVE_CENTER_Y - HIVE_HEIGHT/2 + 2 + i*4, HIVE_WIDTH, 2, Orange);
    }
    fillArea(SCREEN_WIDTH/2 - 2, HIVE_CENTER_Y+HIVE_HEIGHT/2 - 6, 4, 6, Black);
}

static void drawBees() {
    static const short moves[4] = { +1, -1, +SCREEN_WIDTH, -SCREEN_WIDTH};

    if(season == WINTER)
        return;

    beginDrawDirect();
    for(ushort b = 0; b < CEIL_DIV(hive.gatherers, 256); ++b) {
        screenBees[b] += moves[stb_randLCG() & 3];
        asm ( "mov %1, %%es:(%%bx);"
              : : "b"(screenBees[b]), "r"((byte)Yellow) :
        );
    }
    endDrawDirect();
}

//###################################################

void printHiveStatus() {
    print("The season is $");
    println(seasonStr[season]);
    printLabeled("Population $", hive.population);
    printLabeled("Workers $", hive.workers);
    printLabeled("Gatherers $", hive.gatherers);
    printLabeled("Unassigned $", unassignedBees());
    printLabeled("Grow Rate $", hive.growRate);
    print("Drafting $");
    switch(hive.draftTo) {
        case WORKER : println("Workers $"); break;
        case GATHERER : println("Gatherers $"); break;
        case NO_WORK : println("Unassigned $"); break;
    }

    printLabeled("Cells $", hive.cells);
    printLabeled("Planned Cells $", hive.plannedCells);
    printLabeled("Honey stored $", hive.honeyStores);
}

void setGrowRate(short newRate) {
    if(newRate > 0)
        hive.growRate = newRate;
    else
        checkedSub(&hive.growRate, -newRate);
    printLabeled("New grow rate is $", hive.growRate);
}

void draftGatherers() {
    hive.draftTo = GATHERER;
    println("New bees will gather honey.$");
}

void makeGatherers(short delta) {
    if(delta > 0) {
        hive.gatherers += MIN(delta, unassignedBees());
    } else
        checkedSub(&hive.gatherers, -delta);
    printLabeled("Gatherers $", hive.gatherers);
    printLabeled("Unassigned $", unassignedBees());
}

void draftWorkers() {
    hive.draftTo = WORKER;
    println("New bees will work in the hive.$");
}

void makeWorkers(short delta) {
    if(delta > 0) {
        hive.workers += MIN(delta, unassignedBees());
    } else
        checkedSub(&hive.workers, -delta);
    printLabeled("Workers $", hive.workers);
    printLabeled("Unassigned $", unassignedBees());
}

void draftUnassigned() {
    hive.draftTo = NO_WORK;
    println("New bees will wait for assignment.$");
}

void planCells(short extraCells) {
    if(extraCells >= 0)
        checkedAdd(&hive.plannedCells, extraCells, MAX_USHORT);
    else {
        hive.plannedCells -= MIN(-extraCells, hive.plannedCells);
        hive.plannedCells = MAX(hive.cells, hive.plannedCells); // can't deconstruct cells
    }
    printLabeled("Target number of cells is now $", hive.plannedCells);
    printLabeled("Current number of cells is $", hive.cells);
}

void consoleHelp();

//###################################################

typedef struct {
    char* cmdStr;
    void *callback;
    bool exactMatch; // is cmdStr a prefix or the full command?
} Command;

typedef void (*exactCommandFun)(void);
typedef void (*prefixCommandFun)(short);

Command consoleCommands[] = {
    {"stat$", &printHiveStatus, true},
    {"draft gather$", &draftGatherers, true},
    {"draft work$", &draftWorkers, true},
    {"draft none$", &draftUnassigned, true},
    {"make gather $", &makeGatherers, false},
    {"make work $", &makeWorkers, false},
    {"grow rate $", &setGrowRate, false},
    {"plan cells $", &planCells, false},
    {"help$", &consoleHelp, true},
};

#define CMD consoleCommands

void consoleHelp() {
    for(uint i = 0; i < sizeof(CMD)/sizeof(Command); ++i) {
        if(CMD[i].exactMatch)
            println(CMD[i].cmdStr);
        else {
            print(CMD[i].cmdStr);
            println("[-]N$");
        }
    }
    endl();
    println("replace 'N' with a number$");
    println("'done' to return to the game screen$");
    println("'exit' to return to DOS$");
}

static void console() {
    println("What do you want to do? ('help' for list of commands)$");
    while(true) {
        prompt();
        makeLowerCase(USER_INPUT);

        bool matched = false;
        for(uint i = 0; i < sizeof(CMD)/sizeof(Command); ++i) {
            if(CMD[i].exactMatch && strEquals(USER_INPUT, CMD[i].cmdStr)) {
                (*(exactCommandFun)CMD[i].callback)();
                matched = true;
                break;
            }
            if(!CMD[i].exactMatch && startsWith(USER_INPUT, CMD[i].cmdStr)) {
                char* afterPrefix = USER_INPUT + strLen(CMD[i].cmdStr);
                short param;
                if( parse_short(afterPrefix, &param) )
                    (*(prefixCommandFun)CMD[i].callback)(param);
                else
                    println("Error when trying to parse number.$");
                matched = true;
                break;
            }
        }

        if( strEquals(USER_INPUT, "done$") )
            break;
        if( strEquals(USER_INPUT, "exit$") )
            exit();
        if( strEquals(USER_INPUT, "ff$") ) { // debug fast-forward
            matched = true;
            lastGameTick = 0;
            gameTick();
        }
        if( strEquals(USER_INPUT, "nexts$") ) { // debug next season
            matched = true;
            nextSeason();
            break;
        }

        if(!matched)
            println("Sorry, I didn't understand that. ('help' for list of commands)$");
    }

    videoMode();
}

#undef CMD

//###################################################

void dosmain(void) {
    displayInit();
    stb__rand_seed = clockTicks();
    treeSeed = stb_randLCG();
    lastGameTick = clockTicks();

    drawSeasonBackdrop();
    drawHive();

    videoMode();
    while(true) {
        gameTick();

        vsync();
        showDisplayBuffer();
        drawBees();

        while(keyInputAvailable()) {
            if(readASCII_blocking() == ' ') {
                textMode();
                console();
            }
        }

        //waitMillis(50);
    }
}
