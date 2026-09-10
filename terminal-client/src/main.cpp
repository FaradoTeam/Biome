#include "common/log/log.h"

#include "application.h"

int main()
{
    initLog(
        "biome-terminal-client",
        "./logs/",
        10 * 1024 * 1024,
        true,
        true,
        "%Y-%m-%d %H:%M:%S.%f",
        "info",
        false // console disabled (ftxui полностью занимает терминал)
    );

    terminal::Application app;
    return app.run();
}
