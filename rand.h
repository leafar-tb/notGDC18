// from https://github.com/nothings/

static uint stb__rand_seed=0;

static uint stb_srandLCG(unsigned long seed) {
   uint previous = stb__rand_seed;
   stb__rand_seed = seed;
   return previous;
}

static uint stb_randLCG(void) {
   stb__rand_seed = stb__rand_seed * 2147001325 + 715136305; // BCPL generator
   // shuffle non-random bits to the middle, and xor to decorrelate with seed
   return 0x31415926 ^ ((stb__rand_seed >> 16) + (stb__rand_seed << 16));
}

//#########################

// 0 or 1
static bool coinFlip() {
    return stb_randLCG() & 1;
}
