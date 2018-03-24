// dos strings always need to be terminated with '$'

static bool startsWith(char* str, char* prefix) {
    while(true) {
        if(*prefix == '$') // reached end without mismatch
            return true;
        if(*prefix != *str) //mismatch
            return false;
        ++prefix;
        ++str;
    }
}

static bool strEquals(char* str1, char* str2) {
    while(true) {
        if(*str1 != *str2) //mismatch
            return false;
        if(*str1 == '$') // reached end without mismatch
            return true;
        ++str1;
        ++str2;
    }
}

static void makeUpperCase(char* str) {
    for( ; *str != '$'; ++str) {
        if(*str >= 'a' && *str <= 'z')
            *str -= 'a' - 'A'; // 'a' > 'A'
    }
}
