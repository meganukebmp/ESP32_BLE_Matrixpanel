#include <Arduino.h>
#include <SPIFFS.h>
#include <BluetoothSerial.h>

#include <display.h>

BluetoothSerial SerialBT;
char *filePath = "/gifs/maingif.gif";
File gifFile;
bool fileExists = false;

void setup() {
    Serial.begin(115200);
    delay(1000);
    SerialBT.begin("ESP32test"); //Bluetooth device name


    // Start filesystem
    Serial.println("Loading SPIFFS...");
    if(!SPIFFS.begin()){
        // Hang on SPIFFS fail
        Serial.println("SPIFFS Mount Failed");
        while(true){};
    }

    display_setup();

    fileExists = SPIFFS.exists(filePath);
    if (fileExists) {
        display_load_gif(filePath);
    }
}

uint8_t headerArray[] = {'G', 'I', 'F', '8', '9', 'a'};
uint8_t headerIndex = 0;

void loop() 
{
    if (SerialBT.available()) {
        uint8_t recv = SerialBT.read();
        if (recv == headerArray[headerIndex]) {
            headerIndex++;
            if (headerIndex >= sizeof(headerArray)) {
                fileExists = false;
                display_close_gif();
                File f = SPIFFS.open(filePath, FILE_WRITE, true);
                if (f) {
                    f.write(headerArray, sizeof(headerArray));
                    uint32_t lastTime = millis();
                    while(millis() - lastTime < 1000) {
                        if (SerialBT.available()) {
                            recv = SerialBT.read();
                            f.write(recv);
                            lastTime = millis();
                        }
                    }
                    f.close();
                    fileExists = SPIFFS.exists(filePath);
                    display_load_gif(filePath);
                    headerIndex = 0;
                }
            }
        }
        else {
            headerIndex = 0;
        }
    }

    if (fileExists) {
        display_advance_frame();
    }

    // root = SPIFFS.open(gifDir);
    // if (root) {
    //     Serial.println("Opened root dir");
    //     gifFile = root.openNextFile();
    //     Serial.println(gifFile.path());
    //     while (gifFile) {
    //         if (!gifFile.isDirectory()) {
    //             // Zero filepath for Cstring null terminator
    //             memset(filePath, 0x0, sizeof(filePath));
    //             strcpy(filePath, gifFile.path());

    //             // Show it.
    //             display_play_gif(filePath);
    //         }

    //         gifFile.close();
    //         gifFile = root.openNextFile();
    //     }

    //     root.close();
    // }
    // delay(1000);
}