typedef unsigned char byte;
typedef unsigned short ushort;
typedef unsigned int uint;
typedef byte bool;

#define true 1
#define false 0

#define MAX_USHORT ( ( 1 << 16 ) - 1 )
#define MAX_UINT ( (uint) -1 )

//###################################################

#define MIN(X, Y) ( (X) < (Y) ? (X) : (Y) )
#define MAX(X, Y) ( (X) > (Y) ? (X) : (Y) )

#define CEIL_DIV(VAL, DIV) ( ( (VAL) + (DIV) - 1 ) / (DIV) )

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
