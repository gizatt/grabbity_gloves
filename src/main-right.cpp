/**************************************************************************

 Driver for the right hand grabbity glove.
  - Blink LED with a square wave passed through a low-pass filter.
  -  On screen, displays:
  top line: OVERWATCH FREQ 711MHZ
  second line: LISTEN [timestamp]
  third + fourth line: scrolling lines from the bee movie.
  - Colors finger LEDs a pleasant pulsing orange.

 **************************************************************************/

#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_NeoPixel.h>
#include <Adafruit_SSD1306.h>
#include "bee_movie_wrapped.h"
#include "utils.h"

#define LED_PIN_1 16
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

char writing_timestamp_buffer[DISPLAY_WIDTH_CHARS];
void display_timestamp()
{
  snprintf(writing_timestamp_buffer, DISPLAY_WIDTH_CHARS, "LISTEN [%02d:%02d:%02d]", get_current_hour(), get_current_minute(), get_current_second());
  display.setCursor(0, DISPLAY_HEIGHT_START + DISPLAY_LINE_HEIGHT * 1);
  display.write(writing_timestamp_buffer);
}

const char *data_text_ptr = bee_movie_script;
int last_display_update_millis = 0;
void display_advance()
{
  int now = millis();
  if (now - last_display_update_millis > 2000)
  {
    last_display_update_millis = now;

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
      if (*data_text_ptr == 0)
      {
        data_text_ptr = bee_movie_script;
      }
    }
    data_text_ptr = first_line_break;
    display.display();
  }
}

void setup()
{
  Serial.begin(9600);

  pinMode(LED_PIN_1, OUTPUT);
  analogWriteFrequency(LED_PIN_1, 375000); // Teensy 3.0 pin 3 also changes to 375 kHz
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

float led_pwm = 0.0;
#define LED_RC 1.0
void update_led(float dt)
{
  int second = get_current_second();
  int led_target_state = ((second / 2) % 2);
  float alpha = dt / (LED_RC + dt);
  led_pwm = led_pwm * alpha + led_target_state * (1. - alpha);
  analogWrite(LED_PIN_1, 100 * led_pwm);
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
  update_led(UPDATE_DT);
  update_fingers();
  display_advance();
  delay(UPDATE_DT * 1000);
}
