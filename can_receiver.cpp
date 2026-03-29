#include <iostream>
#include <iomanip>
#include <windows.h>
#include <canlib.h>

#define DATA_LENGTH 8

int main(void)
{
    canHandle h;
    long id;
    unsigned char msg[DATA_LENGTH];
    unsigned int dlc;
    unsigned int flags;
    unsigned long time_stamp;
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

    std::cout << "CAN reception start (Ctrl+C to exit)\n" << std::endl;
    std::cout << "Waiting for CAN messages...\n" << std::endl;

    while (true) {
        // ===== API: canRead =====
        // In:  h=Handle, id_ptr=ID pointer, msg_ptr=Data pointer, dlc_ptr=DLC pointer, 
        //      flags_ptr=Flags pointer, time_stamp_ptr=Timestamp pointer
        // Out: Status
        // Function: Read CAN message from reception queue
        canStatus status = canRead(h, &id, msg, &dlc, &flags, &time_stamp);

        if (status == canOK) {
            count++;
            std::cout << "[" << count << "] Received: ID=0x" << std::hex << id 
                      << std::dec << ", DLC=" << dlc << ", Data=[";
            
            for (unsigned int i = 0; i < dlc; i++) {
                std::cout << std::setfill('0') << std::setw(2) << std::hex 
                          << (int)msg[i] << std::dec;
                if (i < dlc - 1) std::cout << " ";
            }
            std::cout << "]" << std::endl;
        }

        Sleep(100);  // Check for messages every 100ms
    }

    // Cleanup
    canBusOff(h);
    canClose(h);

    return 0;
}
