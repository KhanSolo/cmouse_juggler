#pragma once

#include <time.h>

static char* GetTime(){
    time_t now = time(NULL);
    char* now_str = ctime(&now);
    return  now_str;
}

