typedef unsigned char byte;
typedef unsigned short ushort;
typedef unsigned int uint;
typedef byte bool;

#define true 1
#define false 0

//###################################################

static short sign(short in) {
    short retval;
    asm (
        "cwd;"
        : "=d"(retval) : "a"(in) :
    );
    return (retval << 1) + 1;
}

static ushort abs(short in) {
    ushort retval;
    asm (
        "cwd;"
        "xor %%dx, %%ax;"
        "sub %%dx, %%ax;"
        : "=a"(retval) : "a"(in) : "%dx"
    );
    return retval;
}
