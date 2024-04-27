//================ Bluetooth ================//

#include "BluetoothSerial.h"

#if !defined(CONFIG_BT_ENABLED) || !defined(CONFIG_BLUEDROID_ENABLED)
#error Bluetooth is not enabled!
#endif

BluetoothSerial SerialBT;

//================ OLED display ================//

#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels

// Declaration for an SSD1306 display connected to I2C (SDA, SCL pins)
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

//================ Temperature measuring module  ================//

#include "DHT.h"

#define DHTPIN 4
#define DHTTYPE DHT22

DHT dht(DHTPIN, DHTTYPE);

float t, f, h, hic, hif;

//================ Rotary encoder ================//

#include "AiEsp32RotaryEncoder.h"
#include "Arduino.h"

#define CLK 19
#define DT 18
#define BTN 5
#define ROTARY_ENCODER_VCC_PIN -1
#define ROTARY_ENCODER_STEPS 2

AiEsp32RotaryEncoder rotaryEncoder = AiEsp32RotaryEncoder(CLK, DT, BTN, ROTARY_ENCODER_VCC_PIN, ROTARY_ENCODER_STEPS);

String options[3] = {"Temperature", "Humidity", "Heat index"};
String toPrint = "Rotate to start";
bool inMenu = true;

//================ Code ================//

void printOnDisplay(String text, int off_x, int off_y) {
    display.clearDisplay();
    display.setCursor(off_x, off_y);
    display.println(text);
    display.display();
}

void rotaryLoop()
{
    if (inMenu && rotaryEncoder.encoderChanged()) {
        toPrint = options[rotaryEncoder.readEncoder()];
    }
    if (rotaryEncoder.isEncoderButtonClicked()) {
        inMenu = (inMenu) ? false : true;
        Serial.print("InMenu:");
        Serial.println(inMenu);
    }
}

void setup() {
    Serial.begin(115200);
    
    SerialBT.begin("AirConditionsMeter"); //Bluetooth device name
    
    rotaryEncoder.begin();
    rotaryEncoder.setup([]{rotaryEncoder.readEncoder_ISR();});
    rotaryEncoder.setBoundaries(0, 2, true);
    rotaryEncoder.disableAcceleration();
  
    if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
        Serial.println(F("SSD1306 allocation failed"));
        for(;;);
    }
    
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(WHITE);
    printOnDisplay(toPrint, 16, 28);
    
    dht.begin();
}

void writeBT(String str) {
    int i = 0;
    while (str[i] != '\0') {
        Serial.write(str[i]);
        SerialBT.write(str[i]);
        i++;
    }
}

void BTManager() {
    int readBT = SerialBT.read();
    int endline = SerialBT.read();
    Serial.write(readBT);
    Serial.write(endline);

    if (endline != '\n') {
        while (endline != '\n') {
            endline = SerialBT.read();
            Serial.write(endline);
        }
        writeBT("Unknown command, try: T - for temperature | H - for Humidity | I - for Heat index; also note that endline must be set to LF only\n");
        return;
    }

    if (readBT == 'T') {
        t = dht.readTemperature();
        f = dht.readTemperature(true);

        if (isnan(t) || isnan(f)) {
            writeBT("Something went wrong with measuring temperature\n");
            return;
        }

        writeBT("Temperature: ");
        writeBT(String(t));
        writeBT(" C; ");
        writeBT(String(f));
        writeBT(" F\n");
    } else if (readBT == 'H') {
        h = dht.readHumidity();

        if (isnan(h)) {
            writeBT("Something went wrong with measuring humidity\n");
            return;
        }
        
        writeBT("Humidity: ");
        writeBT(String(h));
        writeBT("%\n");
    } else if (readBT == 'I') {
        t = dht.readTemperature();
        f = dht.readTemperature(true);
        h = dht.readHumidity();

        if (isnan(t) || isnan(f) || isnan(h)) {
            writeBT("Something went wrong with measuring heat index\n");
            return;
        }

        hif = dht.computeHeatIndex(f, h);
        hic = dht.computeHeatIndex(t, h, false);
        
        writeBT("Heat index: ");
        writeBT(String(hic));
        writeBT(" C; ");
        writeBT(String(hif));
        writeBT(" F\n");
    } else {
        writeBT("Unknown command, try: T - for temperature | H - for Humidity | I - for Heat index\n");
    }
}

void loop() {
    if (SerialBT.available()) {
        BTManagaer();
    }
    
    rotaryLoop();
    if (inMenu) {
        printOnDisplay(toPrint, 16, 28);
        return;
    }

    display.clearDisplay();
    display.setCursor(0, 20);
    delay(1500);
    if (toPrint == "Temperature") {
        t = dht.readTemperature();
        f = dht.readTemperature(true);

        if (isnan(t) || isnan(f)) {
            printOnDisplay("Something went wrong", 16, 28);
            return;
        }

        display.println("Temperature: ");
        display.print(t);
        display.println(" C");
        display.print(f);
        display.println(" F");
    } else if (toPrint == "Humidity") {
        h = dht.readHumidity();

        if (isnan(h)) {
            printOnDisplay("Something went wrong", 16, 28);
            return;
        }
        
        display.print("Humidity: ");
        display.print(h);
        display.println("%");
    } else {
        t = dht.readTemperature();
        f = dht.readTemperature(true);
        h = dht.readHumidity();

        if (isnan(t) || isnan(f) || isnan(h)) {
            printOnDisplay("Something went wrong", 16, 28);
            return;
        }

        hif = dht.computeHeatIndex(f, h);
        hic = dht.computeHeatIndex(t, h, false);
        
        display.println("Heat index: ");
        display.print(hic);
        display.println(" C");
        display.print(hif);
        display.println(" F");
    }
    display.display();
}
