#include "CPU.hpp"
#include "FrontendSystem.hpp"


int main(int argc, char* argv[])
{

    FrontendSystem frontEnd = FrontendSystem("GBMoo", 600, 600);
    std::string fileAppend = ".gb";
    frontEnd.loadCPURom(argv[1] + fileAppend);
    frontEnd.run();
   
    return 0;
    
}