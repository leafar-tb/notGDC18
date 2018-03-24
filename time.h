
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

static uint cpuTicks() {
    uint retVal;
    // set edx:eax to cpu cycle count; high bits dropped
    asm ("rdtsc;" : "=a"(retVal) : : "edx" );
    return retVal;
}

static uint clockTicks() {
    uint retVal;
    asm volatile (
        "mov $0, %%ah;" // read system clock counter
        "int $0x1A;" // BIOS clock services
        // count stored in cx:dx, combined into ecx
        "shl $16, %%ecx;"
        "mov %%dx, %%cx;"
        : "=c"(retVal) : : "ax", "dx" );
    return retVal;
}
