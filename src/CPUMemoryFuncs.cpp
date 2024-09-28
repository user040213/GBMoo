#include "CPU.hpp"

void CPU::loadROM(const std::string fileName)
{
    std::ifstream inputFile(fileName, std::ios::binary);
    if(!inputFile.is_open())
    {
        std::cout << "Failed to open file" << std::endl;
        return;
    }

    uint32_t fileSize{std::filesystem::file_size(fileName)};

    // No larger than 8 MB
    if(fileSize > (8*pow(10, 6)))
    {
        std::cout << "No ROMs greater than 8 MB allowed";
        return;
    }

    std::vector<char> fBuffer = std::vector<char>(std::istreambuf_iterator<char>(inputFile), std::istreambuf_iterator<char>());
    inputFile.close();

    // setup banks first
    getBankMode(fBuffer[CART_TYPE]);
    getROMSize(fBuffer[ROM_HEADER]);
    getRAMSize(fBuffer[RAM_HEADER]);
    generateBanks();

    for(uint32_t i{0}; i < fileSize; i++)
    {
        // only do if i has not exceeded 0x7FFF
        if(i < 0x8000)
        {
            memMap[i] = fBuffer[i];
        }

        // we have exceeded space available in ROM banks 0 to 1 so check if banks exist
        // this also checks if the banks will even be enough
        else if((maxROMBanks != 2) && (ROMBanks.size() >= (fileSize - 0x8000)))
        {
            ROMBanks[i - 0x8000] = fBuffer[i];
        }
        
        // Banks don't exist and we ran out of space so the ROM has some issue
        else
        {
            std::cout << "ERROR: Check ROM header and see if enough ROM banks are allocated " << std::endl; 
            return;
        }   
    }
}

uint8_t CPU::readMemory(uint16_t addr)
{
    uint8_t returnVal{0};
    // between 0x4000 and 0x7FFF we need to use ROM banks
    if(addr < ROM_NN)
    {
        returnVal = memMap[addr];
    }

    // between 0x4000 and 0x7FFF we need to use ROM banks
    else if((addr >= ROM_NN) && (addr < VRAM))
    {
        if(curROMBank == 1)
        {
            // read directly from ROM
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

    else if((addr >= VRAM) && (addr < EXT_RAM))
    {
        returnVal = memMap[addr];
    }

    // between 0xA000 and 0xBFFF we need to use RAM banks
    else if((addr >= EXT_RAM) && (addr < WRAM_0))
    {
        if(curRAMBank == 0)
        {
            // read directly
            returnVal = memMap[addr];
        }
        else
        {

            // add an offset depending on bank number
            addr += 0x2000*(curRAMBank - 1);

            returnVal = RAMBanks[addr];
        }
    }

    else if((addr >= WRAM_0) && (addr < WRAM_N))
    {
        returnVal = memMap[addr];
    }

    else if((addr >= WRAM_N) && (addr < ECHO_RAM))
    {
        returnVal = memMap[addr];

        // need to add a cgb wram bank handler here
    }

    else if((addr >= ECHO_RAM) && (addr < SPRITE_TABLE))
    {
        returnVal = memMap[addr];
    }

    else if((addr >= SPRITE_TABLE) && (addr < UNUSABLE_AREA))
    {
        returnVal = memMap[addr];
    }

    else if((addr >= UNUSABLE_AREA) && (addr < IO_REGISTERS))
    {
        returnVal = memMap[addr];
    }

    else if((addr >= IO_REGISTERS) && (addr < HRAM_LOC))
    {
        if(addr == 0xFF00){ returnVal = mJoypad.getJoypadState(memMap); }
        //else if(addr == SCANLINE_REGISTER){ returnVal = 0x90;}
        else{returnVal = memMap[addr];}
    }

    else if((addr >= HRAM_LOC) && (addr < INTERRUPT_ENABLE))
    {
        returnVal = memMap[addr];
    }

    else if(addr == 0xFFFF)
    {
        returnVal = memMap[addr];
    }
    
    return returnVal;
}

void CPU::writeMemory(uint16_t addr, uint8_t data)
{
    // writes to ROM are blocked
    if(addr < VRAM)
    {
        doBanking(addr, data);
    }

    else if((addr >= VRAM) && (addr < EXT_RAM))
    {
        memMap[addr] = data;
    }

    // between 0xA000 and 0xBFFF we need to use RAM banks
    else if((addr >= EXT_RAM) && (addr < WRAM_0))
    {
        if(enableRAM)
        {
            if(curRAMBank == 0)
            {
                
                memMap[addr] = data;
            }
            else
            {

                // add an offset depending on bank number
                addr += 0x2000*(curROMBank - 1);

                RAMBanks[addr] = data;
            }
        }
    }

    else if((addr >= WRAM_0) && (addr < WRAM_N))
    {
        memMap[addr] = data;
        memMap[addr + 0x2000] = data;
    }

    else if((addr >= WRAM_N) && (addr < ECHO_RAM))
    {
        memMap[addr] = data;
        if(addr <= 0xDDFF)
        {
            memMap[addr + 0x2000] = data;
        }

        // need to add a cgb wram bank handler here
    }

    // direct write to echo should be blocked I think
    else if((addr >= ECHO_RAM) && (addr < SPRITE_TABLE))
    {
        //memMap[addr];
    }

    else if((addr >= SPRITE_TABLE) && (addr < UNUSABLE_AREA))
    {
        memMap[addr] = data;
    }

    else if((addr >= UNUSABLE_AREA) && (addr < IO_REGISTERS))
    {
        //memMap[addr];
    }

    else if((addr >= IO_REGISTERS) && (addr < HRAM_LOC))
    {
        if(addr == 0xFF00)
        {
            data &= 0xF0;
            memMap[addr] = data;
        }
        else if(addr == 0xFF46)
        {
            DMATransfer(data);
        }
        else if(addr == DIV_LOC)
        {
            memMap[addr] = 0;
        }
        else if(addr == SCANLINE_REGISTER)
        {
            memMap[addr] = 0;
        }
        else{memMap[addr] = data;}
    }

    else if((addr >= HRAM_LOC) && (addr < INTERRUPT_ENABLE))
    {
        memMap[addr] = data;
    }

    else if(addr == 0xFFFF)
    {
        memMap[addr] = data;
    }

    if (addr == 0xFF02 && data == 0x81) 
    {
		std::cout << memMap[0xFF01];
    }

     
}

void CPU::getBankMode(const uint8_t type)
{
    switch(type)
    {

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

void CPU::getROMSize(const uint8_t type)
{
    switch(type)
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
        case 0x52:
            maxROMBanks = 72;
            break;
        case 0x53:
            maxROMBanks = 80;
            break;
        case 0x54:
            maxROMBanks = 96;
            break;
        default:
            maxROMBanks = 2;
    }
}

void CPU::getRAMSize(const uint8_t type)
{
    switch(type)
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
    if(DEBUG_MODE){std::cout << "Generating " << (int)maxROMBanks-2 << " ROM banks" << std::endl; std::getchar();}
    if(maxROMBanks > 2){ROMBanks = std::vector<uint8_t>((maxROMBanks - 2)*0x4000, 0);}
    /*  because of how RAM banking works, 
        banks = 0 means the allocated RAM section in memMap wont be used
        so, if ram banks = 1, we don't actually need to create any new memory, 
        so we create maxRAMBanks - 1 banks, only doing so if banks != 0 or 1
    */
    if(DEBUG_MODE){std::cout << "Generating " << (int)maxRAMBanks-1 << " RAM banks" << std::endl; std::getchar();}
    if(maxRAMBanks > 1){RAMBanks = std::vector<uint8_t>((maxRAMBanks - 1)*0x2000, 0);}

}

void CPU::DMATransfer(uint8_t data)
{
    uint16_t addr{data * 0x100};
    for(uint16_t i{0}; i < 0xA0; i++)
    {
        writeMemory(0xFE00 + i, readMemory(addr + i));
    }
}

void CPU::doBanking(const uint16_t addr, const uint8_t data)
{
    if (addr < 0x2000)
    {
        if(mbcType == MBC1)
        {
            if((data & 0xF) == 0xA)
            {
                enableRAM = true;
            }
            else
            {
                enableRAM = false;
            }
        }
    }
    else if(addr < 0x4000)
    {
        uint8_t maskedData{data & 0b11111};
        curROMBank &= 0b11100000;
        curROMBank |= maskedData;
        if(curROMBank == 0)
        {
            curROMBank++;
        }
    }
    else if(addr < 0x6000)
    {
        if(doingROMBanking)
        {
            uint8_t maskedData{data & 0b11100000};
            curROMBank &= 0b00011111;
            curROMBank |= maskedData;
            if(curROMBank == 0)
            {
                curROMBank++;
            } 
        }
        else
        {
            curRAMBank = data & 0b11;
        }
    }
    else if(addr < 0x8000)
    {
        uint8_t maskedData{data & 0b1};
        if(maskedData == 0)
        {
            doingROMBanking = true;
        }
        else
        {
            doingROMBanking = false;
        }
        if(doingROMBanking)
        {
            curRAMBank = 0;
        }
        
    }
}