// target display mode is vga with 256 colours
#define SCREEN_WIDTH 320
#define SCREEN_HEIGHT 200

// set up segment registers for the screen and double buffer
// segments 0 and 1 are in use by bios and our program
// so we use segment 2 for our double-buffering
static void displayInit() {
    asm volatile (
        "mov $0x2000, %%ax;"
        "mov %%ax, %%es;" // %%es -> off-screen buffer
        "mov $0xA000, %%ax;"
        "mov %%ax, %%fs;" // %%fs -> VGA segment
        : : : "%ax"
    );
}

static void videoMode() {
    asm volatile ( //0x00: set mode; 0x13: vga 256
        "mov   $0x0013, %%ax;"
        "int   $0x10;" // BIOS video
        : : : "ax"
    );
}

static void textMode() {
    asm volatile ( //0x00: set mode; 0x03: text
        "mov   $0x0003, %%ax;"
        "int   $0x10;"
        : : : "ax"
    );
}

//###################################################

static void setPixel(short x, short y, byte colour) {
    asm volatile (
        "mov %1, %%es:(%%bx);"
        : : "b"(x+y*320), "r"(colour) :
    );
}

static void fillArea(short leftx, short topy, ushort width, ushort height, byte colour) {
    for(short y = topy;  y < topy+height; ++y) {
        asm volatile (
            "cld;"
            "rep"
            " stosb;"
            : : "c"(width), "D"((short)(leftx+y*320)), "a"(colour) :
        );
        CLOBBERED("%cx", "%di");
    }
}

//###################################################

// fills the off-screen buffer with colour
static void clearDisplayBuffer(byte colour) {
    uint col = ( colour << 8 ) | colour;
    col |= col << 16;
    asm volatile (
        "mov $16000, %%ecx;"
        "mov $0, %%edi;"
        "cld;"
        "rep"
        " stosl;"
        : : "a"(col) : "%ecx", "%edi", "memory"
    );
}

static void vsync() {
    asm volatile (
        "1: "
        "mov $0x3DA, %%dx;"
        "in %%dx, %%al;" // vga register
        "and $0x8, %%al;" // bit 3 indicates vertical retrace
        "jz 1b;"
        : : : "ax", "dx"
    );
}

// move contents of the off-screen buffer to the screen
static void showDisplayBuffer() {
    asm volatile (
        "push %%ds;"
        "mov %%es, %%ax;" // src = offscreen buffer
        "mov %%ax, %%ds;"
        "mov %%fs, %%ax;" // dest = screen buffer
        "mov %%ax, %%es;"

        "mov $16000, %%ecx;"
        "mov $0, %%edi;"
        "mov $0, %%esi;"
        "cld;"
        "rep"
        " movsl;"

        // restore segments
        "mov %%es, %%ax;"
        "mov %%ax, %%fs;"
        "mov %%ds, %%ax;"
        "mov %%ax, %%es;"
        "pop %%ds;"
        : : : "%ax", "%ecx", "%edi", "%esi", "memory"
    );
}
