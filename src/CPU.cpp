#include "CPU.hpp"


CPU::CPU()
{
    // Register resetting
    memset(registers, 0, 8);

    sp = 0;
    pc = 0;

    // Initialize memory map
    memMap = std::vector<uint8_t>(MEM_MAP_SIZE, 0); 

    // Reset cycle count
    cycleCount = 0;

    

    // Resetting opcode decode variables
    XX = 0;
    YYY = 0;
    ZZZ = 0;

    P = 0;
    Q = 0;
}

void CPU::loadROM(const std::string fileName)
{
    std::ifstream inputFile(fileName, std::ios::binary);
    if(!inputFile.is_open())
    {
        std::cout << "The file failed to be opened" << std::endl; 
        return;
    }
    
    uintmax_t fileSize{std::filesystem::file_size(fileName)};
    if(fileSize > 8*pow(10, 6))
    {
        std::cout << "ROM exceeds maximum available size of 8 MB" << std::endl;
        return;
    }
    
    char* fBuffer{new char[fileSize]};
    inputFile.read(fBuffer, fileSize);
    inputFile.close();
    // Need to first input first 0x8000 into memMap
    bool endOfFile{false};
    uintmax_t nextIndex{0};
    for(uintmax_t i{0}; i < fileSize; i++)
    {
        memMap[0x0] = fBuffer[i]; 
        if(i = (fileSize - 1))
        {
            endOfFile = true;
        }
        nextIndex = i+1;
    }

    // Setup rom and ram banks based on cartridge header (which we just input into memMap)
    getROMSize();
    getRAMSize();
    generateBanks();

    // Input rest of the ROM into the ROM banks as necessary
    // if we hit end of file, then we didnt need rom banks
    if(!endOfFile)
    {
        for(uintmax_t i{0}; i < (fileSize - nextIndex); i++)
        {
            ROMBanks[i] = fBuffer[i + nextIndex];
        }
    }

    delete[] fBuffer;
}

uint8_t CPU::readMemory(uint16_t addr)
{
    uint8_t returnVal{0};

    // between 0x4000 and 0x7FFF we need to use ROM banks
    if((addr >= ROM_NN) && (addr <= (VRAM - 1)))
    {
        if(curROMBank = 1)
        {
            // read directly from memMap
            returnVal = memMap[addr];
        }
        else
        {
            // subtract by 0x4000
            addr -= 0x4000;

            // add an offset depending on bank number
            addr += 0x4000*(curROMBank - 2);

            returnVal = ROMBanks[addr];
        }
    }

    // between 0xA000 and 0xBFFF we need to use RAM banks
    else if((addr >= EXT_RAM) && (addr <= 0xBFFF))
    {
        if(curRAMBank = 1)
        {
            // read directly from memMap
            returnVal = memMap[addr];
        }
        else
        {
            // subtract by 0xA000
            addr -= EXT_RAM;

            // add an offset depending on bank number
            addr += 0x2000*(curROMBank - 2);

            returnVal = RAMBanks[addr];
        }
    }

    // Any other case (no mem banking required)
    else
    {
        returnVal = memMap[addr];
    }
    pc++;

    return returnVal;
}

void CPU::writeMemory(uint16_t addr, uint8_t data)
{
    /* ROM BANKS */
    if(addr < VRAM)
    {
        // can't write here
    }
    
    /* WRITE TO VRAM */
    else if((addr >= VRAM) && (addr < EXT_RAM))
    {
        memMap[addr] = data;
    }
    
    /* WRITE TO EXTERNAL RAM */
    else if((addr >= EXT_RAM) && (addr < WRAM_0))
    {
        // write directly to memory
        if(curRAMBank = 1)
        {
            memMap[addr] = data;
        }
        else
        {
            addr -= 0xA000;

            // add offset depending on bank number
            RAMBanks[addr + (0x2000*(curRAMBank - 2))] = data;
        }
    }
    
    /* WRITE TO WRAM 0 */
    else if((addr >= WRAM_0) && (addr < WRAM_N))
    {
        memMap[addr] = data;
    }
    
    /* WRITE TO WRAM 1-N */
    else if((addr >= WRAM_N) && (addr < ECHO_RAM))
    {

        memMap[addr] = data;
        
    }
    
    /* WRITE TO ECHO RAM */
    else if((addr >= ECHO_RAM) && (addr < SPRITE_TABLE))
    {
        // any writes here are duplicated to 0xC000-0xDDFF
        // ie 0x2000 addresses below
        memMap[addr] = data;
        memMap[addr - 0x2000] = data;

        /* NOTE/Question: Does writing between 0xC000 and 0xDDFF duplicate to echo? */
    }

    /* SPRITE AREA */
    else if((addr >= SPRITE_TABLE) && (addr < 0xFEA0))
    {
        memMap[addr] = data;
    }

    /* RESTRICTED */
    else if((addr >= 0xFEA0) && (addr < 0xFEFF))
    {
        // unwritable
    }
    /* I/O, HRAM and interrupts */
    else
    {
        memMap[addr] = data;
    }
    
}

void CPU::getBankMode()
{
    switch(readMemory(CART_TYPE))
    {
        pc--;

        // No MBC
        case 0x0:
            mbcType = NO_MBC;
            break;

        // MBC1
        case 0x1:
        case 0x2:
        case 0x3:
            mbcType = MBC1;
            break;

        // MBC2
        case 0x5:
        case 0x6:
            mbcType = MBC2;
            break;
        
        // MBC3
        case 0x0F:
        case 0x10:
        case 0x11:
        case 0x12:
        case 0x13:
            mbcType = MBC3;
            break;
        
        // MBC5
        case 0x19:
        case 0x1A:
        case 0x1B:
        case 0x1C:
        case 0x1D:
        case 0x1E:
            mbcType = MBC5;
            break;

        default:
            mbcType = NO_MBC;

    }
}

void CPU::getROMSize()
{
    uint8_t val{readMemory(ROM_HEADER)};
    pc--;

    switch(val)
    {
        case 0:
            maxROMBanks = 2;
            break;
        case 1:
            maxROMBanks = 4;
            break;
        case 2:
            maxROMBanks = 8;
            break;
        case 3:
            maxROMBanks = 16;
            break;
        case 4:
            maxROMBanks = 32;
            break;
        case 5:
            maxROMBanks = 64;
            break;
        case 6:
            maxROMBanks = 128;
            break;
        case 7:
            maxROMBanks = 256;
            break;
        case 8:
            maxROMBanks = 512;
            break;
        case 52:
            maxROMBanks = 72;
            break;
        case 53:
            maxROMBanks = 80;
            break;
        case 0x54:
            maxROMBanks = 96;
            break;
        default:
            maxROMBanks = 2;
    }
}

void CPU::getRAMSize()
{
    uint8_t val{readMemory(RAM_HEADER)};
    pc--;

    switch(val)
    {
        case 0x00:
        case 0x01:
            maxRAMBanks = 0;
            break;
        case 0x02:
            maxRAMBanks = 1;
            break;
        case 0x03:
            maxRAMBanks = 4;
            break;
        case 0x04:
            maxRAMBanks = 16;
            break;
        case 0x05:
            maxRAMBanks = 8;
            break;
        default:
            maxRAMBanks = 0;
    }
}

void CPU::generateBanks()
{
    /*  we need 2 less than maxROMBanks number of banks
        each bank has 0x4000 addressable locations
        Banks 00 and 01 are located in the memory map
        Banks 02 and onwards are located contiguously
        in the ROMBanks vector (so index 0 is bank 02, index 0x4000 is bank 03)
    */
    ROMBanks = std::vector<uint8_t>((maxROMBanks - 2)*0x4000, 0);


    /*  we need 1 less than maxRAMBanks number of banks
        each bank has 0x2000 addressable locations
        Bank 0 is located in the memory map beginning a 0xA000
        Banks 1 and onwards are located contiguously
        in the RAMBanks vector (index 0 is bank 1, index 0x2000 is bank 2)
    */
    RAMBanks = std::vector<uint8_t>((maxRAMBanks - 1)*0x2000, 0);
}
