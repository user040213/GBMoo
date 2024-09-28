#include "CPUTest.hpp"
#include "nlohmann/json.hpp"

void CPUTest::JsonState(const std::string fileName)
{
    // Json parsing
    std::string inputFile{fileName};
    std::ifstream fJson(fileName);
    if(fJson.fail())
    {
        std::cout << "File does not exist" << std::endl;
        return;
    }
    nlohmann::json data = nlohmann::json::parse(fJson);

    // Testing variables
    uint16_t verifyCycles{0};
    uint8_t verifyRegisters[8]{0};
    uint16_t verifyPC{0};
    uint16_t verifySP{0};

    uint16_t verifyMem[20][2];
    uint8_t verifyMemLimit{0};
    std::string opName{""};
    std::string failMessage{""};

    bool failed{false};
    bool thisFailed{false};

    // Store initial and test conditions
    for(const auto& item : data.items())
    {
        std::cout << item.key() << std::endl;
        thisFailed = false;
        opName = "";
        failMessage = "";
        verifyMemLimit = 0;
        verifyCycles = 0;
        cycleCount = 0;
        for (const auto& subItem : item.value().items())
        {
            if(subItem.key() == "cycles")
            {
                for(const auto& sub2Item : subItem.value().items())
                {
                    verifyCycles++;
                }
            }
            else if(subItem.key() == "name")
            {
                opName = subItem.value();
            }
            else if(subItem.key() == "final")
            {
                for(const auto& sub2Item : subItem.value().items())
                {
                    if(sub2Item.key() == "ram")
                    {
                        for(const auto& sub3Item : sub2Item.value().items())
                        {
                            verifyMem[verifyMemLimit][0] = sub3Item.value()[0];
                            verifyMem[verifyMemLimit][1] = sub3Item.value()[1];
                            verifyMemLimit++;
                        }
                    }
                    else if(sub2Item.key() == "pc")
                    {
                        verifyPC = sub2Item.value();
                    }
                    else if(sub2Item.key() == "sp")
                    {
                        verifySP = sub2Item.value();
                    }
                    else if(sub2Item.key() == "a")
                    {
                        verifyRegisters[R_A] = sub2Item.value();
                    }
                    else if(sub2Item.key() == "b")
                    {
                        verifyRegisters[R_B] = sub2Item.value();
                    }
                    else if(sub2Item.key() == "c")
                    {
                        verifyRegisters[R_C] = sub2Item.value();
                    }
                    else if(sub2Item.key() == "d")
                    {
                        verifyRegisters[R_D] = sub2Item.value();
                    }
                    else if(sub2Item.key() == "e")
                    {
                        verifyRegisters[R_E] = sub2Item.value();
                    }
                    else if(sub2Item.key() == "f")
                    {
                        verifyRegisters[R_F] = sub2Item.value();
                    }
                    else if(sub2Item.key() == "h")
                    {
                        verifyRegisters[R_H] = sub2Item.value();
                    }
                    else if(sub2Item.key() == "l")
                    {
                        verifyRegisters[R_L] = sub2Item.value();
                    }

                }
            }
            else if(subItem.key() == "initial")
            {
                for(const auto& sub2Item : subItem.value().items())
                {
                    if(sub2Item.key() == "ram")
                    {
                        for(const auto& sub3Item : sub2Item.value().items())
                        {
                            memMap[sub3Item.value()[0]] = sub3Item.value()[1];
                        }
                    }
                    else if(sub2Item.key() == "pc")
                    {
                        pc = sub2Item.value();
                        pc--;
                    }
                    else if(sub2Item.key() == "sp")
                    {
                        sp = sub2Item.value();
                    }
                    else if(sub2Item.key() == "a")
                    {
                        registers[R_A] = sub2Item.value();
                    }
                    else if(sub2Item.key() == "b")
                    {
                        registers[R_B] = sub2Item.value();
                    }
                    else if(sub2Item.key() == "c")
                    {
                        registers[R_C] = sub2Item.value();
                    }
                    else if(sub2Item.key() == "d")
                    {
                        registers[R_D] = sub2Item.value();
                    }
                    else if(sub2Item.key() == "e")
                    {
                        registers[R_E] = sub2Item.value();
                    }
                    else if(sub2Item.key() == "f")
                    {
                        registers[R_F] = sub2Item.value();
                    }
                    else if(sub2Item.key() == "h")
                    {
                        registers[R_H] = sub2Item.value();
                    }
                    else if(sub2Item.key() == "l")
                    {
                        registers[R_L] = sub2Item.value();
                    }
                }
            }  
        }

        opcode = readMemory(pc);
        pc++;
        decode();
        executeOp();

        // Test cases use a hardware accurate decode execute prefetch pipeline
        // But this uses fetch, decode, execute since it is a bit easier to emulate
        pc++; // so we add +1 to account for this

        // do a check against final and verified final values
        if(verifyCycles != cycleCount)
        {
            failed = true;
            thisFailed = true;
            failMessage += "Incorrect cycle count: Yours: " + std::to_string(cycleCount) + " Correct: " + std::to_string(verifyCycles) + "\n";
        }
            

        if(verifyPC != pc)
        {
            failed = true;
            thisFailed = true;
            failMessage += "Incorrect PC: Yours: " + std::to_string(pc) + " Correct: " + std::to_string(verifyPC) + "\n";
        }

        if(verifySP != sp)
        {
            failed = true;
            thisFailed = true;
            failMessage += "Incorrect SP: Yours: " + std::to_string(sp) + " Correct: " + std::to_string(verifySP) + "\n";
        }

        for(uint8_t regVal{0}; regVal < 8; regVal++)
        {
            if(verifyRegisters[regVal] != registers[regVal])
            {
                failed = true;
                thisFailed = true;
                std::string failedRegister{""};
                if(regVal == R_A)
                {
                    failedRegister = "A";
                }
                else if(regVal == R_B)
                {
                    failedRegister = "B";
                }
                else if(regVal == R_C)
                {
                    failedRegister = "C";
                }
                else if(regVal == R_D)
                {
                    failedRegister = "D";
                }
                else if(regVal == R_E)
                {
                    failedRegister = "E";
                }
                else if(regVal == R_F)
                {
                    failedRegister = "F";
                }
                else if(regVal == R_H)
                {
                    failedRegister = "H";
                }
                else if(regVal == R_L)
                {
                    failedRegister = "L";
                }
                failMessage += "Incorrect " + failedRegister + " Yours: " + std::to_string(registers[regVal]) 
                                + " Correct: " + std::to_string(verifyRegisters[regVal]) + "\n";

            }
        }

        for(uint8_t i{0}; i < verifyMemLimit; i++)
        {
            if(memMap[verifyMem[i][0]] != verifyMem[i][1])
            {
                failed = true;
                thisFailed = true;
                failMessage += "Incorrect memory at " + std::to_string(verifyMem[i][0]) 
                            + " Yours: " + std::to_string(memMap[verifyMem[i][0]]) + " Correct: " + std::to_string(verifyMem[i][1]) 
                            + "\n";
            }
        }

        if(thisFailed)
        {
            std::cout << "Failed " << opName << std::endl;
            std::cout << failMessage << std::endl;
            std::getchar();
        }
    }

    if(failed)
    {
        std::cout << "Failed " << opName << std::endl;
        std::getchar();
    }
    else
    {
        std::cout << "All tests passed" << std::endl;
    }
    
}