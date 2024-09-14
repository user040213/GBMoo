#include "CPU.hpp"
#include "Emulator.hpp"

int main(int argc, char* argv[])
{
    Emulator* emuObj = new Emulator("GBMoo", DISPLAY_WIDTH, DISPLAY_HEIGHT);

    return 0;
}