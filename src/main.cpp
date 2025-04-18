#define VMA_IMPLEMENTATION
#include "vk_mem_alloc.hpp"

#if defined(WIN32) && defined(_DEBUG)
#include <Windows.h>

const HANDLE console = GetStdHandle(STD_OUTPUT_HANDLE);
#endif

#include "App.hpp"

int main() {
#if defined(WIN32) && defined(_DEBUG)
    DWORD currentConfig;
    GetConsoleMode(console, &currentConfig);

    SetConsoleMode(console, ENABLE_PROCESSED_OUTPUT |
                                ENABLE_VIRTUAL_TERMINAL_PROCESSING |
                                currentConfig);
#endif

    App app;

    if (!app.init()) {
        system("PAUSE");
        return -1;
    }
    app.windowLoop();
    app.destroy();

    return 0;
}