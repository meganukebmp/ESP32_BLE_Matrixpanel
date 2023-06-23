#include <Arduino.h>
#include <HardwareSerial.h>
#include <SPIFFS.h>
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>
#include <TinyGPS++.h>

#include <pins.h>
#include <display.h>

#define SERVICE_UUID                "8a3da2e4-4d2a-48b2-801a-2fbf3ee8a160"
#define RX_CHARACTERISTIC_UUID      "c97aaafc-adf2-4cab-883d-76719b863155"
#define CMD_CHARACTERISTIC_UUID     "cd2de314-70f9-4573-bc71-9c9335e8c963"
#define LAT_CHARACTERISTIC_UUID     "b6120bfa-46b3-45df-93fe-40f4c18ecbb7"
#define LON_CHARACTERISTIC_UUID     "f038d4d6-ddde-4191-b1a3-735bc1af7489"

// Commands to be send on the CMD characteristic
#define CMD_WRITE_BEGIN 1
#define CMD_WRITE_END 2

const char *filePath = "/maingif.gif";
File gifFile;
bool fileExists = false;

int bytesWritten = 0;
bool transfering = false;
bool deviceConnected = false;

// Use UART2 for GPS
HardwareSerial SerialGPS(2);
TinyGPSPlus gps;
BLECharacteristic *latCharacteristic;
BLECharacteristic *lonCharacteristic;

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
            // Show the loading screen
            display_show_loading_screen();
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
    // Rebind UART pins.
    SerialGPS.begin(9600, SERIAL_8N1, GPS_RX_PIN, GPS_TX_PIN);
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

    // GPS characteristics
    latCharacteristic = service->createCharacteristic(LAT_CHARACTERISTIC_UUID, BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_NOTIFY);
    lonCharacteristic = service->createCharacteristic(LON_CHARACTERISTIC_UUID, BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_NOTIFY);
    // For Notify connections we need a CCC descriptor
    latCharacteristic->addDescriptor(new BLE2902());
    lonCharacteristic->addDescriptor(new BLE2902());

    // Start the service
    service->start();

    // Add the service to the advertisement (for discovery) and begin advertising
    BLEAdvertising *advertising = server->getAdvertising();
    advertising->addServiceUUID(SERVICE_UUID);
    advertising->start();

    // Setup the display
    display_setup();

    // Stop on file error
    fileExists = SPIFFS.exists(filePath);
    if (!fileExists) {
        Serial.println("File not found! Corrupted flash!");
        while(true) {};
    }

    display_load_gif(filePath);
}

int failed = 0;

void loop() 
{
    // Advance frame
    if (fileExists) {
        display_advance_frame();
    }

    // Don't bother with GPS stuff while no one is there to listen or while image transfering
    if (deviceConnected && !transfering) {
        // Fill the GPS buffer
        while (SerialGPS.available() > 0) {
            gps.encode(SerialGPS.read());
        }

        // On valid GPS message decode
        if (gps.location.isUpdated()) {
            char lat[20] = {0};
            char lon[20] = {0};

            // Get a full precision GPS message
            sprintf(lat, "%s%d.%d",
                    gps.location.rawLat().negative ? "-" : "",
                    gps.location.rawLat().deg,
                    gps.location.rawLat().billionths);
            sprintf(lon, "%s%d.%d",
                    gps.location.rawLng().negative ? "-" : "",
                    gps.location.rawLng().deg,
                    gps.location.rawLng().billionths);

            // Fill up the characteristics and trigger a notify
            latCharacteristic->setValue((uint8_t*)lat, strlen(lat));
            lonCharacteristic->setValue((uint8_t*)lon, strlen(lon));
            latCharacteristic->notify();
            lonCharacteristic->notify();
        }

        if (gps.failedChecksum() > failed) {
            Serial.print("Sentences that failed checksum=");
            Serial.println(gps.failedChecksum());
            failed = gps.failedChecksum();
        }
    }
}