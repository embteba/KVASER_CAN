#include <iostream>
#include <unistd.h>
#include <windows.h>
#include <canlib.h>

class CANSender {
private:
    static constexpr long CAN_ID = 0x100;
    static constexpr unsigned int PERIOD_MS = 1000;
    static constexpr unsigned int DATA_LENGTH = 8;

    canHandle h;
    int messageCount;

    // ===== API: canOpenChannel =====
    // In:  channel=0, flags=canOPEN_ACCEPT_VIRTUAL
    // Out: Handle (channel reference)
    // Function: Open CAN channel
    void initializeChannel() {
        h = canOpenChannel(0, canOPEN_ACCEPT_VIRTUAL);
    }

    // ===== API: canSetBusParams =====
    // In:  h=Handle, freq=500000, tseg1=0, tseg2=0, sjw=0, noMsgs=0, flags=0
    // Out: Status
    // Function: Set CAN bus speed and timing (500 kbps)
    void setBusParams() {
        canSetBusParams(h, 500000, 0, 0, 0, 0, 0);
    }

    // ===== API: canBusOn =====
    // In:  h=Handle
    // Out: Status
    // Function: Enable CAN bus (start communication)
    void startBus() {
        canBusOn(h);
    }

    // ===== API: canBusOff =====
    // In:  h=Handle
    // Out: Status
    // Function: Disable CAN bus (stop communication)
    void stopBus() {
        canBusOff(h);
    }

public:
    CANSender() : h(-1), messageCount(0) {
        // CANlib initialization
        canInitializeLibrary();
        initializeChannel();
        setBusParams();
        startBus();
    }

    ~CANSender() {
        stopBus();
        canClose(h);
    }

    void run() {
        unsigned char msg[DATA_LENGTH] = {0};
        unsigned int dlc = DATA_LENGTH;

        std::cout << "CAN transmission start (Ctrl+C to exit)\n" << std::endl;
        std::cout << "CAN ID: 0x" << std::hex << CAN_ID << std::dec << ", Period: 1 second\n" << std::endl;

        while (true) {
            // ===== API: canWrite =====
            // In:  h=Handle, id=0x100, msg_ptr=Data pointer, dlc=8, flags=canMSG_STD
            // Out: Status
            // Function: Add CAN message to transmission queue
            canWrite(h, CAN_ID, msg, dlc, canMSG_STD);

            messageCount++;
            std::cout << "[" << messageCount << "] Sent: ID=0x" << std::hex << CAN_ID 
                      << std::dec << ", Data=[00 00 00 00 00 00 00 00]" << std::endl;

            Sleep(PERIOD_MS);  // Wait 1 second (Windows)
        }
    }
};

int main(void)
{
    CANSender sender;
    sender.run();

    return 0;
}

