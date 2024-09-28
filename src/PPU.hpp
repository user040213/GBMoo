#ifndef PPU_H
#define PPU_H

#include <cstdint>
#include <vector>

/* GREENSCALE OR GREYSCALE */
constexpr bool GREENSCALE = true;

/* SCREEN SIZE AND PIXEL FORMAT */
constexpr uint16_t PPU_SCREENWIDTH = 160;
constexpr uint16_t PPU_SCREENHEIGHT = 144;
constexpr uint16_t BYTES_PER_PIXEL = 4;

/* GRAPHICS REGISTERS */
constexpr uint16_t GBWINDOW_Y = 0xFF4A;
constexpr uint16_t GBWINDOW_X = 0xFF4B;
constexpr uint16_t LCD_STATUS = 0xFF41;
constexpr uint16_t LCDC = 0xFF40;
constexpr uint16_t SCANLINE_REGISTER = 0xFF44;
constexpr uint16_t SCY = 0xFF42;
constexpr uint16_t SCX = 0xFF43;
constexpr uint16_t OAM_START = 0xFE00;
constexpr uint16_t VRAM_START = 0x8000;
constexpr uint16_t OBP0 = 0xFF48;
constexpr uint16_t OBP1 = 0xFF49;

/* OTHER USEFUL CONSTANTS */
constexpr uint16_t SCANLINE_TIME = 456; // takes 456 cpu cycles for one scanline
constexpr uint8_t MAX_SCANLINES = 153;

/* MODE CONSTANTS */
constexpr uint8_t MODE_HORI_BLANK = 0;
constexpr uint8_t MODE_VERT_BLANK = 0b1;
constexpr uint8_t MODE_OAM_SCAN = 0b10;
constexpr uint8_t MODE_DRAW_PIX = 0b11;

constexpr uint16_t MODE_HORI_TIME = 204;
constexpr uint16_t MODE_VERT_TIME = 456; // we do this one ten times, each time moving the scanline
constexpr uint16_t MODE_OAM_TIME = 80;
constexpr uint16_t MODE_DRAW_TIME = 172;

class PPU
{
    public:
        PPU();
        ~PPU();
    
        // This happens on every frame
        void PPUCycle(uint16_t thisCycles, std::vector<uint8_t>& memMap);
        std::vector<uint8_t> mPixelArray;
        std::vector<uint8_t> bgWinArray;
        bool reqLCDInterrupt;
        bool reqVBInterrupt;

        void displayDebug();

    private:
        uint8_t getPPUMode(const std::vector<uint8_t>& memMap);
        void setPPUMode(uint8_t newMode, std::vector<uint8_t>& memMap);
        
        // Timers
        int16_t mElapsedModeTime;
        
        bool mLCDPPUEn;
        bool cgbMode;

        void drawScanline(std::vector<uint8_t> &memMap);
        void renderBG(std::vector<uint8_t> &memMap);
        void renderWindow(std::vector<uint8_t> &memMap);
        void renderObj(std::vector<uint8_t> &memMap);

        void flipY();

};


#endif