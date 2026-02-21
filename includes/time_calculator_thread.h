#pragma once

//#include <time.h>

const int TIMER_ID = 1;
static int current_timer_interval = 1;
const int TIMER_INTERVAL_MS = 1000; // раз в секунду

const wchar_t* months[] = {
    L"января", L"февраля", L"марта", L"апреля",
    L"мая", L"июня", L"июля", L"августа",
    L"сентября", L"октября", L"ноября", L"декабря"
};

// static char* GetTime(){
//     time_t now = time(NULL);
//     char* now_str = ctime(&now);
//     return  now_str;
// }

// static void SetTimer(){
//   //SetTimer(hwnd, TIMER_ID, TIMER_INTERVAL_MS, NULL);
// }

// static void KillTimer(){
//   //SetTimer(hwnd, TIMER_ID, TIMER_INTERVAL_MS, NULL);
// }