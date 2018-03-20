// string needs to be terminated with a '$'
static void print(char* string) {
    asm volatile (
        "mov   $0x09, %%ah;"
        "int   $0x21;"
        : : "d"(string) : "ah"
    );
}

//###################################################

static bool keyInputAvailable() {
    bool result;
    asm volatile (
        // read keyboard status; clears zero-flag, if char in keyboard buffer
        "mov   $0x11, %%ah;"
        "int   $0x16;"
        "jnz 0f;"
            "mov $0, %0;"
            "jmp 1f;"
        "\n 0:\n"
            "mov $1, %0;"
        "\n 1:\n"
        : "=r"(result) : : "ax"
    );
    return result;
}

static char readASCII_blocking() {
    char result;
    asm volatile (
        // read keyboard input
        "mov   $0, %%ah;"
        "int   $0x16;"
        // al=ascii code; ah=scan code
        "mov %%al, %0;"
        : "=r"(result) : : "ax"
    );
    return result;
}

// returns 0 if no character ready
static char readASCII() {
    char result;
    asm volatile (
        // read keyboard status; clears zero-flag, if char in keyboard buffer
        "mov   $0x11, %%ah;"
        "int   $0x16;"
        "jnz 1f;"
            "mov $0, %0;" // no char -> set 0
            "jmp 2f;"
        "\n 1: \n"
            // read keyboard input
            "mov   $0, %%ah;"
            "int   $0x16;"
            // al=ascii code; ah=scan code
            "mov %%al, %0;"
        "\n 2: \n"
        : "=r"(result) : : "ax"
    );
    return result;
}
