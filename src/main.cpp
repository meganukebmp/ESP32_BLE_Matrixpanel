#include <Arduino.h>
#include <SPIFFS.h>

#include <display.h>


void setup() {
    Serial.begin(115200);
    delay(1000);

    // Start filesystem
    Serial.println(" * Loading SPIFFS");
    if(!SPIFFS.begin()){
        // Hang on SPIFFS fail
        Serial.println("SPIFFS Mount Failed");
        while(true){};
    }

    display_setup();
}

// play all GIFs in this directory on the SD card
String gifDir = "/gifs";
char filePath[256] = { 0 };
File root, gifFile;

void loop() 
{
    root = SPIFFS.open(gifDir);
    if (root) {
        Serial.println("Opened root dir");
        gifFile = root.openNextFile();
        Serial.println(gifFile.path());
        while (gifFile) {
            if (!gifFile.isDirectory()) {
                // Zero filepath for Cstring null terminator
                memset(filePath, 0x0, sizeof(filePath));
                strcpy(filePath, gifFile.path());

                // Show it.
                display_play_gif(filePath);
            }

            gifFile.close();
            gifFile = root.openNextFile();
        }

        root.close();
    }
    delay(1000);
}