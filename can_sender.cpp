#include <iostream>
#include <cstring>
#include <cstdio>
#include <direct.h>
#include <windows.h>
#include <canlib.h>

class CANSender {
private:
    // ===== Message Definition from message_db.dbc =====
    static constexpr long CAN_ID = 123;              // ENGINE_DATA message ID
    static constexpr unsigned int PERIOD_MS = 1000;  // Transmission period (ms)
    static constexpr unsigned int DATA_LENGTH = 8;   // Message data length (bytes)
    
    // Signal definitions (static from DBC)
    // ENGINE_Torque: start bit 0, length 16 bits, scale 0.1, offset 0, unit Nm
    // ENGINE_RPM: start bit 16, length 16 bits, scale 0.25, offset 0, unit rpm
    static constexpr double TORQUE_SCALE = 0.1;     // Nm per unit
    static constexpr double RPM_SCALE = 0.25;       // rpm per unit

    canHandle h;
    int messageCount;

    // ===== API: canOpenChannel =====
    // In:  channel=0, flags=canOPEN_ACCEPT_VIRTUAL
    // Out: Handle (channel reference)
    // Function: Open CAN channel
    void initializeChannel() {
        h = canOpenChannel(0, canOPEN_ACCEPT_VIRTUAL);
        if (h < 0) {
            std::cerr << "✗ Failed to open channel, error code: " << h << std::endl;
        }
    }

    // ===== API: canSetBusParams =====
    // In:  h=Handle, freq=500000, tseg1=0, tseg2=0, sjw=0, noMsgs=0, flags=0
    // Out: Status
    // Function: Set CAN bus speed and timing (500 kbps)
    void setBusParams() {
        canStatus status = canSetBusParams(h, 500000, 0, 0, 0, 0, 0);
        if (status != canOK) {
            std::cerr << "✗ Failed to set bus params, error code: " << status << std::endl;
        }
    }

    // ===== API: canBusOn =====
    // In:  h=Handle
    // Out: Status
    // Function: Enable CAN bus (start communication)
    void startBus() {
        canStatus status = canBusOn(h);
        if (status != canOK) {
            std::cerr << "✗ Failed to start bus, error code: " << status << std::endl;
        }
    }

    // ===== API: canBusOff =====
    // In:  h=Handle
    // Out: Status
    // Function: Disable CAN bus (stop communication)
    void stopBus() {
        if (h >= 0) {
            canStatus status = canBusOff(h);
            if (status != canOK) {
                std::cerr << "✗ Failed to stop bus, error code: " << status << std::endl;
            }
        }
    }

public:
    CANSender() : h(-1), messageCount(0) {
        std::cout << "Initializing CAN..." << std::endl;
        std::cout.flush();
        
        // CANlib initialization
        canInitializeLibrary();
        std::cout << "✓ CANlib initialized" << std::endl;
        std::cout.flush();
        
        std::cout << "Opening CAN channel..." << std::endl;
        std::cout.flush();
        initializeChannel();
        if (h < 0) {
            std::cerr << "✗ Error: Failed to open CAN channel, handle=" << h << std::endl;
            std::cerr.flush();
            return;
        }
        std::cout << "✓ CAN channel opened (handle=" << h << ")" << std::endl;
        std::cout.flush();
        
        std::cout << "Setting bus parameters..." << std::endl;
        std::cout.flush();
        setBusParams();
        std::cout << "✓ Bus parameters set" << std::endl;
        std::cout.flush();
        
        std::cout << "Starting CAN bus..." << std::endl;
        std::cout.flush();
        startBus();
        std::cout << "✓ CAN bus started" << std::endl;
        std::cout.flush();
    }

    ~CANSender() {
        std::cout << "Shutting down CAN..." << std::endl;
        std::cout.flush();
        stopBus();
        if (h >= 0) canClose(h);
        std::cout << "✓ CAN closed" << std::endl;
        std::cout.flush();
    }

    void run() {
        unsigned char msg[DATA_LENGTH] = {0};
        unsigned int dlc = DATA_LENGTH;

        std::cout << "\nCAN transmission start (Ctrl+C to exit)\n" << std::endl;
        std::cout << "Message: ENGINE_DATA (ID=" << CAN_ID << ")" << std::endl;
        std::cout << "Signal 1: ENGINE_Torque (scale=" << TORQUE_SCALE << " Nm/unit)" << std::endl;
        std::cout << "Signal 2: ENGINE_RPM (scale=" << RPM_SCALE << " rpm/unit)" << std::endl;
        std::cout << "Period: " << PERIOD_MS << "ms\n" << std::endl;

        double rpm = 800.0;
        double torque = 10.0;

        while (true) {
            memset(msg, 0, sizeof(msg));
            
            // Encode ENGINE_Torque signal (bytes 0-1)
            // Scale: 0.1 Nm per unit
            unsigned short torque_raw = (unsigned short)(torque / TORQUE_SCALE);
            msg[0] = (unsigned char)(torque_raw & 0xFF);
            msg[1] = (unsigned char)((torque_raw >> 8) & 0xFF);
            
            // Encode ENGINE_RPM signal (bytes 2-3)
            // Scale: 0.25 rpm per unit
            unsigned short rpm_raw = (unsigned short)(rpm / RPM_SCALE);
            msg[2] = (unsigned char)(rpm_raw & 0xFF);
            msg[3] = (unsigned char)((rpm_raw >> 8) & 0xFF);
            
            // Reserved bytes
            msg[4] = 0x00;
            msg[5] = 0x00;
            msg[6] = 0x00;
            msg[7] = 0x00;

            canStatus writeStatus = canWrite(h, CAN_ID, msg, dlc, canMSG_STD);
            if (writeStatus != canOK) {
                std::cerr << "✗ canWrite failed with status: " << writeStatus << std::endl;
            }

            messageCount++;
            std::cout << "[" << messageCount << "] ENGINE_DATA: RPM=" << rpm 
                      << " rpm, Torque=" << torque << " Nm | Hex=[";
            for (int i = 0; i < (int)DATA_LENGTH; ++i) {
                if (i) std::cout << " ";
                printf("%02X", msg[i]);
            }
            std::cout << "]" << std::endl;

            // Update values
            rpm += 50.0;
            if (rpm > 4000.0) rpm = 800.0;
            
            torque += 1.0;
            if (torque > 200.0) torque = 10.0;

            Sleep(PERIOD_MS);
        }
    }
};

int main()
{
    try {
        std::cout << "==== CAN Sender (using static DBC definitions) ====" << std::endl;
        CANSender sender;
        std::cout << "\n✓ CAN Sender initialized successfully!\n" << std::endl;
        sender.run();
    }
    catch (const std::exception& e) {
        std::cerr << "✗ Exception: " << e.what() << std::endl;
        return 1;
    }
    catch (...) {
        std::cerr << "✗ Unknown exception occurred" << std::endl;
        return 1;
    }

    return 0;
}

