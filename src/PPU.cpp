#include "PPU.hpp"
#include "Helper.hpp"
#include <iostream>
#include <algorithm>

// Might change greyscale for better contrast
/* Greenscale:  00 = #e0f8d0    Greyscale:  00 = #e6e6e6 Not full white, because LCD off should be even lighter
                01 = #88c070                01 = #cccccc       
                10 = #346856                10 = #777777
                11 = #081820                11 = #000000  */
// Each array is 4 by 3, 4 colours, each needing 3 8-bit channels

const uint8_t mDMGGreenscale[4][3] 
{
    {0xe0, 0xf8, 0xd0},
    {0x88, 0xc0, 0x70},
    {0x34, 0x68, 0x56},
    {0x08, 0x18, 0x20}
};
const uint8_t mDMGGreyscale[4]
{
    0xe6, 0xcc, 0x77, 0x00
};

PPU::PPU()
{
    mElapsedModeTime = 0;
    mLCDPPUEn = false;
    cgbMode = false;
    reqLCDInterrupt = false;
    reqVBInterrupt = false;

    mPixelArray = std::vector<uint8_t>(PPU_SCREENWIDTH * PPU_SCREENHEIGHT * BYTES_PER_PIXEL, 0xFF);
    bgWinArray = std::vector<uint8_t>(PPU_SCREENWIDTH * PPU_SCREENHEIGHT * BYTES_PER_PIXEL, 0xFF);
}

PPU::~PPU()
{

}


void PPU::PPUCycle(uint16_t thisCycles, std::vector<uint8_t>& memMap)
{
    // check if LCD disabled
    mLCDPPUEn = Helper::getBit(memMap[LCDC], 7);
    if(!(mLCDPPUEn))
    {
        return;
    }

    // multiply by 4 so we can get T cycles
    mElapsedModeTime += thisCycles*4;

    switch(getPPUMode(memMap))
    {
        // mode 2, takes 80 dots (1 dot = 1 cpu clock = 1/4 m cycle)
        case MODE_OAM_SCAN:
            if(mElapsedModeTime >= MODE_OAM_TIME)
            {
                mElapsedModeTime -= MODE_OAM_TIME;
                setPPUMode(MODE_DRAW_PIX, memMap);
            }
            
            break;

        // mode 3, takes between 172-289 dots (we will only emulate the 172 dots)
        case MODE_DRAW_PIX:
            if(mElapsedModeTime >= MODE_DRAW_TIME)
            {
                mElapsedModeTime -= MODE_DRAW_TIME;
                drawScanline(memMap);
                setPPUMode(MODE_HORI_BLANK, memMap);
            }
            break;
        
        // mode 0, takes 87-204 dots (we do 204 dots so they add up to 456)
        case MODE_HORI_BLANK:
            if(mElapsedModeTime >= MODE_HORI_BLANK)
            {
                mElapsedModeTime -= MODE_HORI_TIME;
                // increment scanline counter
                memMap[SCANLINE_REGISTER]++;
                if(memMap[SCANLINE_REGISTER] == 144)
                {
                    setPPUMode(MODE_VERT_BLANK, memMap);
                }
                else
                {
                    setPPUMode(MODE_OAM_SCAN, memMap);
                }

                
            }
            break;

        // mode 1, 4560 dots (10 scanlines)
        case MODE_VERT_BLANK:
            if(mElapsedModeTime >= MODE_VERT_TIME)
            {
                mElapsedModeTime -= MODE_VERT_TIME;
                // move scanline
                memMap[SCANLINE_REGISTER]++;
                if(memMap[SCANLINE_REGISTER] == 154)
                {
                    memMap[0xFF44] = 0;
                    setPPUMode(MODE_OAM_SCAN, memMap);
                }

            }
            break;
    }
    // check if LYC = LY and we should call interrupt due to it
    if((Helper::getBit(memMap[LCD_STATUS], 6)) && (memMap[SCANLINE_REGISTER] == memMap[0xFF45]))
    {
        Helper::setBit(memMap[LCD_STATUS], 2);

        reqLCDInterrupt = true;
    }
    else
    {
        Helper::resetBit(memMap[LCD_STATUS], 2);
    }
    
}


uint8_t PPU::getPPUMode(const std::vector<uint8_t>& memMap)
{
    uint8_t statByte{memMap[LCD_STATUS]};
    return (statByte & 0b11);
}

void PPU::setPPUMode(uint8_t newMode, std::vector<uint8_t>& memMap)
{
    memMap[LCD_STATUS] &= ~(0b11); // resets PPU mode bits
    memMap[LCD_STATUS] |= newMode;

    uint8_t status{memMap[LCD_STATUS]};

    // set potential interrupt depending on newMode
    switch(newMode)
    {
        case MODE_HORI_BLANK:
            if(Helper::getBit(status, 3))
            {
                reqLCDInterrupt = true;
            }
            break;
        case MODE_VERT_BLANK:
            reqVBInterrupt = true;
            if(Helper::getBit(status, 4))
            {
                reqLCDInterrupt = true;
            }
            break;
        case MODE_OAM_SCAN:
            if(Helper::getBit(status, 5))
            {
                reqLCDInterrupt = true;
            }
            break;
    }
}

void PPU::drawScanline(std::vector<uint8_t> &memMap)
{
    // overwrites all other conflicting enables
    bool bgWinEnable{Helper::getBit(memMap[LCDC], 0)};
    bool objEnable{Helper::getBit(memMap[LCDC], 1)};
    bool winEnable{Helper::getBit(memMap[LCDC], 5)};
    
    if(bgWinEnable)
    {
        renderBG(memMap);
        if(winEnable)
        {
            renderWindow(memMap);
        }
    }

    // set screen to white
    else
    {
        uint32_t pixelArrayIndex;
        for(uint8_t i{0}; i < 160; i++)
        {
            pixelArrayIndex = ((PPU_SCREENWIDTH * memMap[SCANLINE_REGISTER]) + i)*4;
            bgWinArray[pixelArrayIndex] = 0xFF;
            bgWinArray[pixelArrayIndex + 1] = 0xFF;
            bgWinArray[pixelArrayIndex + 2] = 0xFF;
            bgWinArray[pixelArrayIndex + 3] = 0xFF;
        }
    }

    // copy background and window array over, then let objects draw over
    mPixelArray = bgWinArray;

    if(objEnable)
    {
        renderObj(memMap);
    }

    // Because of OpenGL's top left coordinate system
    //flipY();
}

void PPU::renderBG(std::vector<uint8_t> &memMap)
{
    uint16_t bgTileMapArea{0};
    switch(Helper::getBit(memMap[LCDC], 3))
    {
        case 0:
            bgTileMapArea = 0x9800;
            break;
        case 1:
            bgTileMapArea = 0x9C00;
            break;
    }

    uint16_t bgTileDataArea{0};
    switch(Helper::getBit(memMap[LCDC], 4))
    {
        case 0:
            bgTileDataArea = 0x9000;
            break;
        case 1:
            bgTileDataArea = 0x8000;
            break;
    }
    uint8_t yPos{((memMap[SCANLINE_REGISTER] + memMap[SCY]) / 8) % 32};

    for(uint8_t i{0}; i < 160; i++) // draw each pixel along this horizontal line
    {
        uint8_t xPos{((i + memMap[SCX]) / 8) % 32};

        // get tile number from Map area of memory
        uint8_t tileNumber{memMap[bgTileMapArea + xPos + (yPos*32)]};

        // get tile data from memory
        //  Each tile is 16 bytes. If we address starting from 0x9000 we use signed operations
        //  Otherwise, we use unsigned operations
        uint16_t tileDataIndex{0};
        if(bgTileDataArea == 0x9000)
        {
            tileDataIndex = (uint8_t)(bgTileDataArea + ((int8_t)tileNumber * 16));
        }
        else
        {
            tileDataIndex = bgTileDataArea + (tileNumber * 16);
        }

        // add an additional offset to account for each line being 2 bytes
        tileDataIndex += ((memMap[SCANLINE_REGISTER] + memMap[SCY]) % 8) * 2;

        // now we can finally get the two bytes that represents the pixels
        // loByte is the LSB of the pixel, each pixel is represented by the combined bits of loByte and hiByte
        // so bit 7 of hiByte + loByte is the leftmost pixel and combined they are a number 0-3 which represents
        // colour
        uint8_t loByte{memMap[tileDataIndex]};
        uint8_t hiByte{memMap[tileDataIndex + 1]};

        uint8_t curBit{7 - ((i + memMap[SCX]) % 8)};
        uint8_t loBit{Helper::getBit(loByte, curBit)};
        uint8_t hiBit{Helper::getBit(hiByte, curBit) << 1};
        uint8_t colorIndex{hiBit + loBit};

        // we are using srgba8, so each pixel in our array is 4 bytes with the first byte being r channel
        uint32_t pixelArrayIndex{((PPU_SCREENWIDTH * memMap[SCANLINE_REGISTER]) + i)*4};

        uint8_t paletteIndex{memMap[0xFF47]};
        uint8_t paletteIndexArray[4]
        {
            (paletteIndex & 0x03), (paletteIndex >> 2) & 0x03, (paletteIndex >> 4) & 0x03, (paletteIndex >> 6) & 0x03
        };
        uint8_t paletteID{paletteIndexArray[colorIndex]};

        if(GREENSCALE)
        {
            bgWinArray[pixelArrayIndex + 3] = mDMGGreenscale[paletteID][0];      // Red channel
            bgWinArray[pixelArrayIndex + 2] = mDMGGreenscale[paletteID][1];  // Green channel
            bgWinArray[pixelArrayIndex + 1] = mDMGGreenscale[paletteID][2];  // Blue channel
            bgWinArray[pixelArrayIndex] = 0xFF;                           // Alpha channel
        }
        else
        {
            bgWinArray[pixelArrayIndex + 3] = mDMGGreyscale[paletteID];
            bgWinArray[pixelArrayIndex + 2] = mDMGGreyscale[paletteID];
            bgWinArray[pixelArrayIndex + 1] = mDMGGreyscale[paletteID];
            bgWinArray[pixelArrayIndex] = 0xFF;
        }




    }
}

void PPU::renderWindow(std::vector<uint8_t> &memMap)
{
    // if we are off the screen
    if(memMap[SCANLINE_REGISTER] < memMap[GBWINDOW_Y])
    {
        return;
    }

    uint16_t winTileMapArea{0};
    switch(Helper::getBit(memMap[LCDC], 6))
    {
        case 0:
            winTileMapArea = 0x9800;
            break;
        case 1:
            winTileMapArea = 0x9C00;
            break;
    }

    uint16_t winTileDataArea{0};
    switch(Helper::getBit(memMap[LCDC], 4))
    {
        case 0:
            winTileDataArea = 0x9000;
            break;
        case 1:
            winTileDataArea = 0x8000;
            break;
    }

    int winY{memMap[SCANLINE_REGISTER] - memMap[GBWINDOW_Y]};
    uint8_t windowTileY{(winY) / 8};
    uint8_t windowTileYOffset(winY % 8);

    // this is where the window starts
    uint8_t windowX{memMap[GBWINDOW_X] - 7}; // for some reason the x offset is +7 (x=7 corresponds to leftmost)

    for(uint8_t i{0}; i < 160; i++)
    {
        if(windowX < i)
        {
            uint8_t windowTileX{(i - windowX) / 8};

            uint8_t tileNumber{memMap[winTileMapArea + windowTileX + (windowTileY*32)]};

            uint8_t tileDataIndex{0};
            if(winTileDataArea == 0x9000)
            {
                tileDataIndex = (uint8_t)(winTileDataArea + ((int8_t)tileNumber * 16));
            }
            else
            {
                tileDataIndex = winTileDataArea + (tileNumber * 16);
            }

            tileDataIndex += (windowTileYOffset*2);

            uint8_t loByte{memMap[tileDataIndex]};
            uint8_t hiByte{memMap[tileDataIndex + 1]};

            uint8_t curBit{7 - (i % 8)};
            uint8_t loBit{Helper::getBit(loByte, curBit)};
            uint8_t hiBit{Helper::getBit(hiByte, curBit) << 1};
            uint8_t colorIndex{hiBit + loBit};


            uint32_t pixelArrayIndex{((PPU_SCREENWIDTH * memMap[SCANLINE_REGISTER]) + i)*4};

            uint8_t paletteIndex{memMap[0xFF47]};
            uint8_t paletteIndexArray[4]
            {
                (paletteIndex & 0x03), (paletteIndex >> 2) & 0x03, (paletteIndex >> 4) & 0x03, (paletteIndex >> 6) & 0x03
            };
            uint8_t paletteID{paletteIndexArray[colorIndex]};

            if(GREENSCALE)
            {
                bgWinArray[pixelArrayIndex + 3] = mDMGGreenscale[paletteID][0];      // Red channel
                bgWinArray[pixelArrayIndex + 2] = mDMGGreenscale[paletteID][1];  // Green channel
                bgWinArray[pixelArrayIndex + 1] = mDMGGreenscale[paletteID][2];  // Blue channel
                bgWinArray[pixelArrayIndex] = 0xFF;                           // Alpha channel
            }
            else
            {
                bgWinArray[pixelArrayIndex + 3] = mDMGGreyscale[paletteID];
                bgWinArray[pixelArrayIndex + 2] = mDMGGreyscale[paletteID];
                bgWinArray[pixelArrayIndex + 1] = mDMGGreyscale[paletteID];
                bgWinArray[pixelArrayIndex] = 0xFF;
            }
        }
    }
}

void PPU::renderObj(std::vector<uint8_t> &memMap)
{
    bool tallSprite{false}; // 8x8 pixels
    int spriteHeight{8};
    if(Helper::getBit(memMap[LCDC], 2))
    {
        tallSprite = true; // 8x16 pixels
        spriteHeight = 16;
    }

    // Sprites further on the right get drawn over by sprites on the left
    // (Or is it the other way around?)
    for(int i{156}; i >= 0; i -= 4)
    {   
        uint8_t objY = memMap[OAM_START + i];
        int height = spriteHeight;

        int yPos = objY - 16;
        // This means that some part of the sprite is on the scanline
        if((yPos <= memMap[SCANLINE_REGISTER]) && (memMap[SCANLINE_REGISTER] < (yPos + height)))
        {
            uint8_t objX = memMap[OAM_START + i + 1];
            uint8_t sTileNumber = memMap[OAM_START + i + 2];
            uint8_t flags = memMap[OAM_START + i + 3];

            if(tallSprite)
            {
                sTileNumber &= 0xFE;
            }

            uint8_t paletteNum{Helper::getBit(flags, 4)};
            int xPos = objX - 8;
            int paletteData = memMap[0xFF48];
            if(paletteNum)
            {
                paletteData = memMap[0xFF49];
            }


            uint8_t paletteIndexArray[4]
            {
                0, (paletteData >> 2) & 0x03, (paletteData >> 4) & 0x03, (paletteData >> 6) & 0x03
            };

            uint16_t tilePointer = VRAM_START + (sTileNumber * 16);
            uint8_t tileYOffset = memMap[SCANLINE_REGISTER] - yPos;
            if(Helper::getBit(flags, 6))
            {
                tileYOffset = (height - 1) - tileYOffset;
            }

            tilePointer += (tileYOffset * 2);
            uint8_t loData{memMap[tilePointer]};
            uint8_t hiData{memMap[tilePointer + 1]};

            // bit 0 is the rightmost pixel
            for(uint8_t curX{0}; curX < 8; curX++)
            {
                int16_t xIndex = curX + xPos;
                if(xIndex < 160 && xIndex >= 0)
                {
                    uint8_t curBit = 7 - curX;

                    // everything needs to be read in backwards
                    if(Helper::getBit(flags, 5))
                    {
                        curBit = curX;
                    }

                    // loData is LSB of colorIndex
                    uint8_t loBit{Helper::getBit(loData, curBit)};
                    uint8_t hiBit{Helper::getBit(hiData, curBit) << 1};

                    uint8_t colorIndex{loBit + hiBit};
                    uint8_t paletteID{paletteIndexArray[colorIndex]};
                    if(paletteID != 0x00)
                    {
                        uint32_t pixelIndex{((memMap[SCANLINE_REGISTER] * 160) + xIndex) * 4};
                        if(GREENSCALE)
                        {
                            mPixelArray[pixelIndex + 3] = mDMGGreenscale[paletteID][0]; 
                            mPixelArray[pixelIndex + 2] = mDMGGreenscale[paletteID][1]; 
                            mPixelArray[pixelIndex + 1] = mDMGGreenscale[paletteID][2]; 
                        }
                        else
                        {
                            mPixelArray[pixelIndex + 3] = mDMGGreyscale[paletteID]; 
                            mPixelArray[pixelIndex + 2] = mDMGGreyscale[paletteID]; 
                            mPixelArray[pixelIndex + 1] = mDMGGreyscale[paletteID]; 
                        }
                        mPixelArray[pixelIndex] = 0xFF; 
                    }
                } 
            }
        }
    }
}

void PPU::flipY()
{
    std::vector<uint8_t> tempVec(PPU_SCREENHEIGHT * PPU_SCREENWIDTH * 4, 0xFF);
    int curIndex = 0;
    for(int i{PPU_SCREENHEIGHT - 1}; i >= 0; i--)
    {
        for(int j{0}; j < PPU_SCREENWIDTH; j++)
        {
            for(int k{0}; k < 4; k++)
            {
                tempVec[curIndex] = mPixelArray[((PPU_SCREENWIDTH * 4) * i) + j + k];
                curIndex++;
            }
        }
    }

    mPixelArray = tempVec;
}

void PPU::displayDebug()
{
    if(mPixelArray[0] == 0xFF)
    {
        std::fill(mPixelArray.begin(), mPixelArray.end(), 0x00);
    }
    else{
        std::fill(mPixelArray.begin(), mPixelArray.end(), 0xFF);
    }
}

