#pragma once

#include <cstring>
#include <iostream>
#include <windows.h>

std::string welcome1 = 
"    ____  ____        _____            __                      \n"
"   / __/ /_/ /__     / ___/__  _______/ /____  ____ ___        \n"
"  / /_  / / / _ \\    \\__ \\/ / / / ___/ __/ _ \\/ __ `__ \\  \n"
" / __/ / / /  __/   ___/ / /_/ (__  / /_/  __/ / / / / /       \n"
"/_/   /_/_/\\___/   /____/\\__, /____/\\__/\\___/_/ /_/ /_/    \n"
"                        /____/                                 \n";

struct SharedMemory {
    char command[256];
    char result[1024];
    bool ready;
    bool done;
};