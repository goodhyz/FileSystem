#pragma once

#include <cstring>
#include <iostream>
#include <windows.h>

struct SharedMemory {
    char command[256];
    char result[1024];
    bool ready;
    bool done;
};