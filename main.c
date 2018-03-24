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

struct {
    ushort population;
    ushort storageCapacity;
    ushort honeyStores;
} hive;

//###################################################

typedef struct {
    char* cmdStr;
    void (*callback)(void);
    bool exactMatch; // is cmdStr a prefix or the full command?
} Command;

void test() {
    println("test$");
}

Command consoleCommands[] = {
    {"TEST$", &test, true}
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

    videoMode();
    #undef CMD
}

//###################################################

void dosmain(void) {
    displayInit();
    stb__rand_seed = clockTicks();

    drawSpring();
    drawHive();

    videoMode();
    while(true) {
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
