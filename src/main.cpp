#include <Arduino.h>
#include <SPIFFS.h>
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>

#include <display.h>

#define SERVICE_UUID                "8a3da2e4-4d2a-48b2-801a-2fbf3ee8a160"
#define RX_CHARACTERISTIC_UUID      "c97aaafc-adf2-4cab-883d-76719b863155"
#define CMD_CHARACTERISTIC_UUID     "cd2de314-70f9-4573-bc71-9c9335e8c963"

// Commands to be send on the CMD characteristic
#define CMD_WRITE_BEGIN 1
#define CMD_WRITE_END 2

const char *filePath = "/maingif.gif";
File gifFile;
bool fileExists = false;

int bytesWritten = 0;
bool transfering = false;
bool deviceConnected = false;

// Callback for the raw data RX
class rxCharacteristicCallback: public BLECharacteristicCallbacks {
    void onWrite(BLECharacteristic *characteristic) {
        // Don't save data unless we've received the transfer command first
        if (!transfering) return;
        // Increment write tracker
        std::string value = characteristic->getValue();
        bytesWritten += value.length();

        // Write data to SPIFFS FILE
        gifFile.write(reinterpret_cast<const uint8_t*>(&value[0]), value.length());
    }
};

// Callback for the command data
class cmdCharacteristicCallback: public BLECharacteristicCallbacks {
    void onWrite(BLECharacteristic *characteristic) {
        std::string value = characteristic->getValue();

        // Request a write begin
        if (value[0] == CMD_WRITE_BEGIN) {
            // Stop possible read operations while we write
            fileExists = false;
            bytesWritten = 0;
            display_close_gif();
            // Try to open the file
            gifFile = SPIFFS.open(filePath, FILE_WRITE, true);
            if (gifFile) {
                transfering = true;
            }
            Serial.println("Begin transfer");
            return;
        }

        // Request a write end
        if (value[0] == CMD_WRITE_END) {
            // Stop the transfer
            transfering = false;
            // Close the file handle
            gifFile.close();
            // Allow read operations on the file again
            display_load_gif(filePath);
            fileExists = true;

            Serial.print("Bytes written: ");
            Serial.println(bytesWritten);
            return;
        }
    }
};

// General BLE callbacks
class ServerCallback: public BLEServerCallbacks {
    void onConnect(BLEServer* server) {
        deviceConnected = true;
        Serial.println("Device connected");
    };

    void onDisconnect(BLEServer* pServer) {
        deviceConnected = false;
        Serial.println("Device disconnected");
        // Device will stop advertising when connected to. Restart advertising
        // on disconnect to allow reconnections.
        pServer->startAdvertising();
    }
};

void setup() {
    Serial.begin(115200);
    delay(1000);

    // Start filesystem
    Serial.println("Loading SPIFFS...");
    if(!SPIFFS.begin()){
        // Hang on SPIFFS fail
        Serial.println("SPIFFS Mount Failed");
        while(true){};
    }

    // Start BLE
    BLEDevice::init("Matrix Panel");
    // Use high MTU for faster transfers
    BLEDevice::setMTU(517);
    // Create new GATT Server
    BLEServer *server = BLEDevice::createServer();
    server->setCallbacks(new ServerCallback());

    // Create the service
    BLEService *service = server->createService(SERVICE_UUID);

    // Create the RX characteristic
    BLECharacteristic *rxCharacteristic = service->createCharacteristic(RX_CHARACTERISTIC_UUID, BLECharacteristic::PROPERTY_WRITE);
    // We don't respond to these for faster transmissions
    // Undo: SPIFFS writes slow us down too much. Leave responses to slow things down a bit.
    // rxCharacteristic->setWriteNoResponseProperty(true);
    rxCharacteristic->setCallbacks(new rxCharacteristicCallback());

    // Create the CMD characteristic
    BLECharacteristic *cmdCharacteristic = service->createCharacteristic(CMD_CHARACTERISTIC_UUID, BLECharacteristic::PROPERTY_WRITE);
    cmdCharacteristic->setCallbacks(new cmdCharacteristicCallback());

    // Start the service
    service->start();

    // Add the service to the advertisement (for discovery) and begin advertising
    BLEAdvertising *advertising = server->getAdvertising();
    advertising->addServiceUUID(SERVICE_UUID);
    advertising->start();

    // Setup the display
    display_setup();

    // Load the gif if it exists
    fileExists = SPIFFS.exists(filePath);
    if (fileExists) {
        display_load_gif(filePath);
    }
}

long lastTime = 0;
char c = 0;

void loop() 
{
    // Advance frame
    if (fileExists) {
        display_advance_frame();
    }

    delay(1);
}