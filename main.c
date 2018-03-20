asm (".code16gcc;"
     "call  dosmain;"
     "mov   $0x4C, %ah;"
     "int   $0x21;");

// gcc doesn't allow clobbering input registers
// so we just do an empty asm for clobbering those
// (original asm probably needs to be volatile)
#define CLOBBERED(...) asm volatile ( "" : : : __VA_ARGS__)

// these aren't headers as such, just code moved to different files for organisation
#include "types.h"
#include "rand.h"
#include "display.h"
#include "text_io.h"

//###################################################

static void waitMillis(ushort millis) {
    asm volatile (
        "mov $1000, %%cx;"
        "mul  %%cx;" // DX:AX = millis*1000
        "mov  %%dx, %%cx;"
        "mov  %%ax, %%dx;"
        // wait for CX:DX microseconds
        "mov  $0x86, %%ah;"
        "int  $0x15;"
        : : "a"(millis) :
    );
    CLOBBERED("ax", "cx", "dx");
}

//###################################################

int dosmain(void) {
    displayInit();

    return 0;
}
