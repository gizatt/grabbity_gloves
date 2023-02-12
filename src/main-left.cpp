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

#define FAN_PIN 17
#define HEART_PIN_1 7
#define HEART_PIN_2 21
#define HEART_PIN_3 16

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

Adafruit_NeoPixel pixels_1 = Adafruit_NeoPixel(1, 22, NEO_RGBW);
Adafruit_NeoPixel pixels_2 = Adafruit_NeoPixel(1, 23, NEO_RGBW);

#define LOGO_HEIGHT 16
#define LOGO_WIDTH 16

void display_setup()
{
    display.clearDisplay();
    display.setRotation(2);
    display.setTextSize(1);              // Normal 1:1 pixel scale
    display.setTextColor(SSD1306_WHITE); // Draw white text
    display.cp437(true);                 // Use full 256 char 'Code Page 437' font
}

const char *header_text = "OVERWATCH 711.1MHZ";
#define DISPLAY_LINE_HEIGHT 8
#define DISPLAY_WIDTH_CHARS 20
#define DISPLAY_HEIGHT_START 1
void display_write_header()
{
    display.setCursor(0, DISPLAY_HEIGHT_START);
    display.print(header_text);
}

int get_current_hour()
{
    return millis() / 3.6E6;
}
int get_current_minute()
{
    return (millis() / 60000) % 60;
}
int get_current_second()
{
    return (millis() / 1000) % 60;
}

char writing_timestamp_buffer[DISPLAY_WIDTH_CHARS];
void display_timestamp()
{
    snprintf(writing_timestamp_buffer, DISPLAY_WIDTH_CHARS, "LISTEN [%02d:%02d:%02d]", get_current_hour(), get_current_minute(), get_current_second());
    display.setCursor(0, DISPLAY_HEIGHT_START + DISPLAY_LINE_HEIGHT * 1);
    display.write(writing_timestamp_buffer);
}

const char *data_text_ptr = bee_movie_script;
void display_advance()
{
    display.clearDisplay();
    display.stopscroll();
    display_write_header();
    display_timestamp();

    // Starting at our current location, write two full lines
    // to the display. Assume lines are sane lengths.
    const char *first_line_break = 0;
    int n_lines = 0;
    int line_length = 1;
    display.setCursor(0, DISPLAY_HEIGHT_START + DISPLAY_LINE_HEIGHT * 2);
    display.write(">");
    while (n_lines < 2)
    {
        if (*data_text_ptr == '\n' || line_length >= DISPLAY_WIDTH_CHARS)
        {
            n_lines += 1;
            if (first_line_break == 0)
            {
                first_line_break = data_text_ptr + 1;
            }
            display.setCursor(0, DISPLAY_HEIGHT_START + DISPLAY_LINE_HEIGHT * (n_lines + 2));
            display.write(">");
            line_length = 1;
        }
        else
        {
            display.print(data_text_ptr[0]);
            line_length += 1;
        }
        data_text_ptr = data_text_ptr + 1;
    }
    display.display();
}

void setup()
{
    Serial.begin(9600);

    pinMode(HEART_PIN_1, OUTPUT);
    pinMode(HEART_PIN_2, OUTPUT);
    pinMode(HEART_PIN_3, OUTPUT);
    pinMode(HEART_PIN_1, OUTPUT);

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
    display_timestamp();
    display.display();
}

bool led_state = false;
int heart_state = 0;

void loop()
{
    led_state = !led_state;
    heart_state = (heart_state + 1) % 4;
    digitalWrite(HEART_PIN_1, heart_state == 1);
    digitalWrite(HEART_PIN_2, heart_state == 2);
    digitalWrite(HEART_PIN_3, heart_state == 3);
    digitalWrite(FAN_PIN, led_state);

    pixels_1.setPixelColor(0, 255 * (heart_state == 1), 255 * (heart_state == 2), 255 * (heart_state == 3));
    pixels_2.setPixelColor(0, 255 * (heart_state == 1), 255 * (heart_state == 2), 255 * (heart_state == 3));
    pixels_1.show();
    pixels_2.show();

    display_advance();
    delay(1000);
}
