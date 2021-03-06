// string needs to be terminated with a '$'
static void print(char* string) {
    asm volatile (
        "mov   $0x09, %%ah;"
        "int   $0x21;"
        : : "d"(string) : "ah"
    );
}

static void printCharacter(char chr) {
    asm volatile (
        "mov $0x02, %%ah;" // print char in dl
        "int $0x21;"
        : : "d"(chr) : "ah"
    );
}

#define endl() printCharacter('\n')

static void println(char* string) {
    print(string);
    endl();
}

//###################################################

// layout expected by DOS buffered input call
struct KeyBuffer {
    byte maxLength;
    byte count;
    char data[256];
};

// set up max length and make sure data is always terminated with '$'
static struct KeyBuffer inputBuffer = {255, 0, {[255]='$'}};

#define USER_INPUT inputBuffer.data

static void bufferedInput() {
    asm volatile (
        "mov $0x0A, %%ah;" // fill KeyBuffer at address DS:DX
        "int $0x21;"
        : : "d"(&inputBuffer) : "ah", "memory"
    );
    // input is confirmed with enter, but cursor stays in line
    endl();
    // replace terminating '\r' with '$'
    inputBuffer.data[inputBuffer.count] = '$';
}

static void prompt() {
    print("> $");
    bufferedInput();
}

//###################################################

static void print_ushort(ushort val) {
    if(val == 0) {
        printCharacter('0');
        return;
    }

    ushort pow10 = 10000;
    while(val < pow10)
        pow10 /= 10;
    for(; pow10 > 0; pow10 /= 10) {
        printCharacter('0' + val/pow10);
        val = val % pow10;
    }
}

//###################################################

// single key input

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
