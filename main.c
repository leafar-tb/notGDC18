asm (".code16gcc;"
     "call  dosmain;"
     "mov   $0x4C, %ah;"
     "int   $0x21;");

// gcc doesn't allow clobbering input registers
// so we just do an empty asm for clobbering those
// (original asm probably needs to be volatile)
#define CLOBBERED(...) asm volatile ( "" : : : __VA_ARGS__)

#define REPEAT(N_TIMES) for(uint __x = 0; __x < N_TIMES; ++__x)

// these aren't headers as such, just code moved to different files for organisation
#include "types.h"
#include "rand.h"
#include "display.h"
#include "text_io.h"
#include "time.h"

//###################################################

#define SKY_HEIGHT 50

static void drawClouds() {
    REPEAT(6) {
        fillArea(randRange(0, SCREEN_WIDTH), randRange(0, SKY_HEIGHT-15), randRange(10, 50), randRange(5, 15), White);
    }
}

static void drawSpring() {
    fillArea(0, 0, SCREEN_WIDTH, SKY_HEIGHT, LightBlue);
    fillArea(20, 20, 20, 20, Yellow); // Sun
    drawClouds();
    fillArea(0, SKY_HEIGHT, SCREEN_WIDTH, SCREEN_HEIGHT-SKY_HEIGHT, Green);
}

#define HIVE_WIDTH 30
#define HIVE_H_ROWS 9
#define HIVE_HEIGHT (HIVE_H_ROWS * 4)
#define HIVE_CENTER_Y 120

static void drawHive() {
    fillArea((SCREEN_WIDTH - HIVE_WIDTH)/2, HIVE_CENTER_Y - HIVE_HEIGHT/2, HIVE_WIDTH, HIVE_HEIGHT, Brown);
    for(uint i = 0; i < HIVE_H_ROWS; ++i) {
        fillArea((SCREEN_WIDTH - HIVE_WIDTH)/2, HIVE_CENTER_Y - HIVE_HEIGHT/2 + i*4, HIVE_WIDTH, 2, Brown);
        fillArea((SCREEN_WIDTH - HIVE_WIDTH)/2, HIVE_CENTER_Y - HIVE_HEIGHT/2 + 2 + i*4, HIVE_WIDTH, 2, Yellow);
    }
    fillArea(SCREEN_WIDTH/2 - 2, HIVE_CENTER_Y+HIVE_HEIGHT/2 - 6, 4, 6, Black);
}

static void drawBees() {
    beginDrawDirect();
    REPEAT(20) {
        uint rand = stb_randLCG();
        setPixel(rand % SCREEN_WIDTH, (rand >> 16) % SCREEN_HEIGHT, Yellow);
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

int dosmain(void) {
    displayInit();
    stb__rand_seed = clockTicks();

    drawSpring();
    drawHive();

    videoMode();
    while(true) {
        vsync();
        showDisplayBuffer();
        drawBees();

        waitMillis(50);
    }

    return 0;
}
