#include <iostream>
#include <unistd.h>
#include <windows.h>
#include <canlib.h>

#define CAN_ID 0x100
#define PERIOD_MS 1000
#define DATA_LENGTH 8

int main(void)
{
    canHandle h;
    unsigned char msg[DATA_LENGTH] = {0};
    unsigned int dlc = DATA_LENGTH;
    int count = 0;

    // CANlib initialization
    canInitializeLibrary();

    // ===== API: canOpenChannel =====
    // In:  channel=0, flags=canOPEN_ACCEPT_VIRTUAL
    // Out: Handle (channel reference)
    // Function: Open CAN channel
    h = canOpenChannel(0, canOPEN_ACCEPT_VIRTUAL);

    // ===== API: canSetBusParams =====
    // In:  h=Handle, freq=500000, tseg1=0, tseg2=0, sjw=0, noMsgs=0, flags=0
    // Out: Status
    // Function: Set CAN bus speed and timing (500 kbps)
    canSetBusParams(h, 500000, 0, 0, 0, 0, 0);

    // ===== API: canBusOn =====
    // In:  h=Handle
    // Out: Status
    // Function: Enable CAN bus (start communication)   
    canBusOn(h);

    std::cout << "CAN transmission start (Ctrl+C to exit)\n" << std::endl;
    std::cout << "CAN ID: 0x" << std::hex << CAN_ID << std::dec << ", Period: 1 second\n" << std::endl;

    while (true) {
        // ===== API: canWrite =====
        // In:  h=Handle, id=0x100, msg_ptr=Data pointer, dlc=8, flags=canMSG_STD
        // Out: Status
        // Function: Add CAN message to transmission queue
        canWrite(h, CAN_ID, msg, dlc, canMSG_STD);

        count++;
        std::cout << "[" << count << "] Sent: ID=0x" << std::hex << CAN_ID 
                  << std::dec << ", Data=[00 00 00 00 00 00 00 00]" << std::endl;

        Sleep(PERIOD_MS);  // Wait 1 second (Windows)
    }

    // Cleanup
    canBusOff(h);
    canClose(h);

    return 0;
}

