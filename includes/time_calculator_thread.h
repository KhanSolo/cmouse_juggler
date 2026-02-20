#pragma once

#include <time.h>

static char* GetTime(){
    time_t now = time(NULL);
    char* now_str = ctime(&now);
    return  now_str;
}

static void SetTimer(){
  //SetTimer(hwnd, TIMER_ID, TIMER_INTERVAL_MS, NULL);
}

static void KillTimer(){
  //SetTimer(hwnd, TIMER_ID, TIMER_INTERVAL_MS, NULL);
}