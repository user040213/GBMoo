#include "CPU.hpp"

// this should be inside of the cpu clock cycle loop
void CPU::runCPU()
{
    
}

void CPU::executeOp()
{
    // do CB prefix instead
    if(opcode == 0xCB)
    {
        readMemory(pc);
        decode();
        switch(XX)
        {
            case 0:
                opROT();
                break;
            case 1:
                opBIT();
                break;
            case 2:
                opRES();
                break;
            case 3:
                opSET();
                break;
        }

        return;
    }

    if(XX = 0)
    {
        switch(ZZZ)
        {
            case 0:
                break;
            case 1:
                switch(P)
                {
                    case 0:
                        opLDrpnn();
                        break;
                    case 1:
                        opADDHLrp();
                        break;
                }
                break;
            case 2:
                break;
            case 3:
                opINCDECrp();
                break;
            case 4:
            case 5:
                opINCDECr();
                break;
            case 6:
                opLDrn();
                break;
            case 7:
                break;
        }
    }
    else if(XX = 1)
    {
        switch(YYY)
        {
            case 6:
                opHALT();
                break;
            default: 
                opLDrr();
        }
    }
    else if(XX = 2)
    {
        switch(YYY)
        {
            case 0:
            case 1:
                opADDAr();
                break;
            case 2:
            case 3:
                opSUBAr();
                break;
            case 4:
                opANDAr();
                break;
            case 5:
                opXORAr();
                break;
            case 6:
                opORAr();
                break;
            case 7:
                opCPAr();
                break;
        }     
    }
    else
    {
        switch(ZZZ)
        {
            case 0:
                x3z0Decode();
                break;
            case 1:
                x3z1Decode();
                break;
            case 2:
                x3z2Decode();
                break;
            case 3:
                switch(YYY)
                {
                    case 0:
                        opJPnn();
                        break;
                    case 6:
                        opDI();
                        break;
                    case 7:
                        opEI();
                        break;
                }
                break;
            case 4:
                switch(YYY)
                {
                    case 0:
                    case 1:
                    case 2:
                    case 3:
                        opCALLccnn();
                        break;
                }
                break;
            case 5:
                switch(Q)
                {
                    case 0:
                        opPUSHrp2();
                        break;
                    case 1:
                        if(P == 0){opCALLnn();}
                        break;
                }
            case 6:
                switch(YYY)
                {
                    case 0:
                    case 1:
                        opADDAn();
                        break;
                    case 2:
                    case 3:
                        opSUBAn();
                        break;
                    case 4:
                        opANDAn();
                        break;
                    case 5:
                        opXORAn();
                        break;
                    case 6:
                        opORAn();
                        break;
                    case 7:
                        opCPAn();
                        break;
                }
            case 7:
                opRST();
                break;
        }
        
    }
}

void CPU::x0z0Decode()
{
    switch(YYY)
    {
        case 0:
            opNOP();
            break;
        case 1:
            opLDnnSP();
            break;
        case 2:
            opSTOP();
            break;
        case 3:
            opJRd();
            break;
        case 4:
            opJRccd();
            break;
    }
}
void CPU::x0z2Decode()
{
    if(Q = 0)
    {
        switch(P)
        {
            case 0:
            case 1:
                opLDrrA();
                break;
            case 2:
            case 3:
                opLDHLIDA();
                break;  
        }
    }
    else
    {
        switch(P)
        {
            case 0:
            case 1:
                opLDArr();
                break;
            case 2:
            case 3:
                opLDAHLID();
                break;
        }
    }
}
void CPU::x0z7Decode()
{
    switch(YYY)
    {
        case 0:
            opRLCA();
            break;
        case 1:
            opRRCA();
            break;
        case 2:
            opRLA();
            break;
        case 3:
            opRRA();
            break;
        case 4:
            opDAA();
            break;
        case 5:
            opCPL();
            break;
        case 6:
            opSCF();
            break;
        case 7:
            opCCF();
            break;
    }
}
void CPU::x3z0Decode()
{
    switch(YYY)
    {
        case 0:
        case 1:
        case 2:
        case 3:
            opRETcc();
            break;
        case 4:
            opLDHnA();
            break;
        case 5:
            opADDSPd();
            break;
        case 6:
            opLDHAn();
            break;
        case 7:
            opLDHLSPId();
            break;
    }
}
void CPU::x3z1Decode()
{
    if(Q == 0)
    {
        opPOPrp2();
    }
    else
    {  
        switch(P)
        {
            case 0:
                opRET();
                break;
            case 1:
                opRETI();
                break;
            case 2:
                opJPHL();
                break;
            case 3:
                opLDSPHL();
                break;
        }
    }
}
void CPU::x3z2Decode()
{
    switch(YYY)
    {
        case 0:
        case 1:
        case 2:
        case 3:
            opJPccnn();
            break;
        case 4: 
            opLDHCA();
            break;
        case 5:
            opLDnnA();
            break;
        case 6:
            opLDHAC();
            break;
        case 7:
            opLDAnn();
            break;
    }
}

void CPU::incDIV()
{
    registers[0xFF04] += 1;
}
void CPU::resetDIV()
{
    registers[0xFF04] = 0;
}
void CPU::incTIMA()
{
    memMap[0xFF05] += 1;
}
void CPU::resetTIMA()
{
    memMap[0xFF05] = memMap[0xFF06];
}
