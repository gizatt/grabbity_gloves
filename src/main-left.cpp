/**************************************************************************

 Driver for the left hand grabbity glove.

  - Every thirty seconds, moves from three hearts illuminated, to two hearts,
    to one heart, back to three.
  - Shows a resin icon with '_%02d_*%02d_%02d', showing the runtime (hh:mm:ss)
    of the device.
  - Every five seconds, alternates the fan between not running and running.

 **************************************************************************/

#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_NeoPixel.h>
#include <Adafruit_SSD1306.h>
#include "utils.h"
#include "resin_bmp.h"

#define FAN_PIN 17
#define HEART_PIN_1 7
#define HEART_PIN_2 21
#define HEART_PIN_3 16
#define PIXEL_PIN_1 22
#define PIXEL_PIN_2 23

// Finger pixels
Adafruit_NeoPixel pixels_1 = Adafruit_NeoPixel(1, PIXEL_PIN_1, NEO_GRBW);
Adafruit_NeoPixel pixels_2 = Adafruit_NeoPixel(1, PIXEL_PIN_2, NEO_GRBW);

// Screen stuff
#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 32 // OLED display height, in pixels

// Declaration for an SSD1306 display connected to I2C (SDA, SCL pins)
// The pins for I2C are defined by the Wire-library.
// On an arduino UNO:       A4(SDA), A5(SCL)
// On an arduino MEGA 2560: 20(SDA), 21(SCL)
// On an arduino LEONARDO:   2(SDA),  3(SCL), ...
#define OLED_RESET 4        // Reset pin # (or -1 if sharing Arduino reset pin)
#define SCREEN_ADDRESS 0x3C ///< See datasheet for Address; 0x3D for 128x64, 0x3C for 128x32
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

void display_setup()
{
    display.clearDisplay();
    display.setRotation(0);
    display.setTextSize(4);              // Normal 1:1 pixel scale
    display.setTextColor(SSD1306_WHITE); // Draw white text
    display.cp437(true);                 // Use full 256 char 'Code Page 437' font
}

int last_display_update_millis = 0;
int n_resin = 0;
#define WRITING_BUFFER_SIZE 5
#define RESIN_RATE_PER_SECOND (1. / 300.)
#define DISPLAY_UPDATE_DT 0.2
char writing_buffer[WRITING_BUFFER_SIZE];
void display_advance()
{
    int now = millis();
    if (now - last_display_update_millis > DISPLAY_UPDATE_DT * 1000)
    {
        last_display_update_millis = now;

        // Maybe increase resin count
        if (random(100000) < (DISPLAY_UPDATE_DT * 100000) * RESIN_RATE_PER_SECOND)
        {
            n_resin = (n_resin + 1) % 100;
        }

        display.clearDisplay();
        display.stopscroll();

        display.drawBitmap(8, 0, resin_bmp_data, resin_bmp_width, resin_bmp_height, 1);
        display.setCursor(48, 0);
        snprintf(writing_buffer, WRITING_BUFFER_SIZE, "_%02d", n_resin);
        display.write(writing_buffer);

        display.display();
    }
}

void setup()
{
    Serial.begin(9600);

    pinMode(HEART_PIN_1, OUTPUT);
    pinMode(HEART_PIN_2, OUTPUT);
    pinMode(HEART_PIN_3, OUTPUT);

    pinMode(FAN_PIN, OUTPUT);
    // analogWriteFrequency(FAN_PIN, 375000); // Teensy 3.0 pin 3 also changes to 375 kHz
    pixels_1.begin();
    pixels_2.begin();

    // SSD1306_SWITCHCAPVCC = generate display voltage from 3.3V internally
    if (!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS))
    {
        Serial.println(F("SSD1306 allocation failed"));
        for (;;)
            ; // Don't proceed, loop forever
    }

    // Show initial display buffer contents on the screen --
    // the library initializes this with an Adafruit splash screen.
    display.display();
    delay(500);

    // Clear the buffer
    display.clearDisplay();
    display_setup();
    display.display();
}

void update_hearts(float dt)
{
    int second = get_current_second();
    digitalWrite(HEART_PIN_3, second < 45);
    digitalWrite(HEART_PIN_2, second < 55);
    digitalWrite(HEART_PIN_1, 1); // Always on

    bool fan_on = (second % 10) < 5;
    // Only *very* slightly turn it off, on battery power the board can barely
    // spin it at full power.
    analogWrite(FAN_PIN, 10 * fan_on + 245);
}

#define FINGER_HUE 0.08 // Orange
#define FINGER_SATURATION 0.9
#define FINGER_VALUE 0.7
inline float get_pulsing_noise(float x, float t)
{
    return cos(2 * x + t) * sin(x - 0.5 * t);
}
void update_fingers()
{
    double t = millis() / 1000.;

    float hue_1 = FINGER_HUE + get_pulsing_noise(0., t * 2.) * 0.02;
    float saturation_1 = FINGER_SATURATION + get_pulsing_noise(0., t * 0.2 + 1.) * 0.1;
    float value_1 = FINGER_VALUE + get_pulsing_noise(0., t * 0.5 + 2.) * 0.3;

    float hue_2 = FINGER_HUE + get_pulsing_noise(0., t * 2. + 4.) * 0.02;
    float saturation_2 = FINGER_SATURATION + get_pulsing_noise(0., t * 0.09 + 3.) * 0.1;
    float value_2 = FINGER_VALUE + get_pulsing_noise(0., t * 0.5 + 6) * 0.3;

    pixels_1.setPixelColor(0, pixels_1.gamma32(pixels_1.ColorHSV(hue_1 * 65535, saturation_1 * 255, value_1 * 255)));
    pixels_2.setPixelColor(0, pixels_1.gamma32(pixels_2.ColorHSV(hue_2 * 65535, saturation_2 * 255, value_2 * 255)));
    pixels_1.show();
    pixels_2.show();
}

#define UPDATE_DT 0.1
void loop()
{
    update_hearts(UPDATE_DT);
    update_fingers();
    display_advance();
    delay(UPDATE_DT * 1000);
}