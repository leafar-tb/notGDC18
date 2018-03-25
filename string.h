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

static void makeLowerCase(char* str) {
    for( ; *str != '$'; ++str) {
        if(*str >= 'A' && *str <= 'Z')
            *str += 'a' - 'A'; // 'a' > 'A'
    }
}

static ushort strLen(char* str) {
    ushort len = 0;
    while(*str != '$') {
        ++len;
        ++str;
    }
    return len;
}

//###################################################

#define isdigit(chr) ( (chr) >= '0' && (chr) <= '9' )

static bool parse_ushort(char* str, ushort* result) {
    if(strLen(str) == 0)
        return false;

    ushort retVal = 0;
    for( ; *str != '$'; ++str) {
        if(!isdigit(*str))
            return false;
        //TODO check overflow
        retVal *= 10;
        retVal += (*str) - '0';
    }

    *result = retVal;
    return true;
}

static bool parse_short(char* str, short* result) {
    bool negative = false;
    if(*str == '+') {
        ++str;
    } else if(*str == '-') {
        ++str;
        negative = true;
    }

    if(strLen(str) == 0)
        return false;

    short retVal = 0;
    for( ; *str != '$'; ++str) {
        if(!isdigit(*str))
            return false;
        //TODO check overflow
        retVal *= 10;
        retVal += (*str) - '0';
    }

    if(negative)
        *result = -retVal;
    else
        *result = retVal;
    return true;
}
