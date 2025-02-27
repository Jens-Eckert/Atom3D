#define VMA_IMPLEMENTATION
#include "vk_mem_alloc.hpp"

#if defined(WIN32)
#include <Windows.h>

const HANDLE console = GetStdHandle(STD_OUTPUT_HANDLE);
#endif

#include "App.hpp"

int main() {
#if defined(WIN32) && defined(_DEBUG)
    SetConsoleMode(console, ENABLE_PROCESSED_OUTPUT | ENABLE_VIRTUAL_TERMINAL_PROCESSING);
#endif

    App app;

    app.init();
    app.windowLoop();
    app.destroy();

    return 0;
}