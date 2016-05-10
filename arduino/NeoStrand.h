// NeoStrand extension to the Adafruit_NeoPixel library class.
//
// NeoStrand
// Copyright (c) by Ed Halley and Jaime Halley
//
// NeoStrand is licensed under a
// Creative Commons Attribution-ShareAlike 4.0 International License.
// 
// You should have received a copy of the license along with this
// work. If not, see <http://creativecommons.org/licenses/by-sa/4.0/>.
//

#ifndef __NEOSTRAND_H__
#define __NEOSTRAND_H__

#include <Adafruit_NeoPixel.h>
#ifdef __AVR__
  #include <avr/power.h>
#endif

//-------------------------------------------------------------------------------------

// Extends the core Adafruit_NeoPixel class with some additional useful capabilities.
//
class NeoStrand : public Adafruit_NeoPixel
{
public:
  NeoStrand(uint16_t n, uint8_t p=6, neoPixelType t=NEO_GRB + NEO_KHZ800) :
    Adafruit_NeoPixel(n, p, t) { ; }
  NeoStrand(void) :
    Adafruit_NeoPixel() { ; }

public:

  static uint8_t White(uint32_t color) { return (color>>24) & 0xFF; }
  static uint8_t Red(uint32_t color) { return (color>>16) & 0xFF; }
  static uint8_t Green(uint32_t color) { return (color>>8) & 0xFF; }
  static uint8_t Blue(uint32_t color) { return color & 0xFF; }

  // Scale the brightness of a color (accepts values 0~255).
  //
  static uint32_t Bright(uint32_t color, uint8_t bright)
  {
    int factor = bright;
    factor++;
    uint16_t w = NeoStrand::White(color) * factor / 256;
    uint16_t r = NeoStrand::Red(color) * factor / 256;
    uint16_t g = NeoStrand::Green(color) * factor / 256;
    uint16_t b = NeoStrand::Blue(color) * factor / 256;
    return NeoStrand::Color(r, g, b, w);
  }

  // Compute a bright color based on a given hue (color wheel position) 0~255.
  //
  static uint32_t Wheel(uint8_t WheelPos)
  {
    WheelPos = 255 - WheelPos;
    if (WheelPos < 85) {
      return Color(255 - WheelPos * 3, 0, WheelPos * 3);
    }
    if (WheelPos < 170) {
      WheelPos -= 85;
      return Color(0, WheelPos * 3, 255 - WheelPos * 3);
    }
    WheelPos -= 170;
    return Color(WheelPos * 3, 255 - WheelPos * 3, 0);
  }

  // Instantly or slowly wipes a constant color from the first to the last pixel.
  // Displays immediately; no strand.show() call is required.
  //
  void wipeWithColor(uint32_t color, uint16_t wait = 0)
  {
    for (uint16_t i = 0; i < numPixels(); i++)
    {
      setPixelColor(i, color);
      if (wait)
      {
        show();
        delay(wait);
      }
    }
    if (!wait)
      show();
  }

  // Instantly or slowly wipes a rainbow from the first to the last pixel.
  // If given an initial hue value (0~255), the whole rainbow is shifted to align.
  // The rainbow is scaled so that the whole strand ranges through one hue cycle.
  // Displays immediately; no strand.show() call is required.
  //
  void wipeWithRainbow(uint8_t shift = 0, uint16_t wait = 0)
  {
    for (uint16_t i = 0; i < numPixels(); i++)
    {
      uint16_t hue = shift + (i * 256 / numPixels());
      setPixelColor(i, Wheel(hue & 0xFF));
      if (wait)
      {
        show();
        delay(wait);
      }
    }
    if (!wait)
      show();
  }

  // Shifts all pixel color contents forward (away from pixel 0) by a given number of
  // pixels. If given a color, the nearest pixel(s) are loaded with that color; black
  // is the default. If not given a number of pixels to shift, 1 pixel is the default.
  // Does not display immediately; follow up with a strand.show() call.
  //
  void scrollForward(uint16_t amount = 1, uint32_t color = 0)
  {
    if (!numPixels())
      return;
    uint16_t stride = bytesPerPixel();
    amount = amount % numPixels();
    if (!amount)
      return;
    memmove(pixels+stride*amount, pixels, (numPixels()-amount)*stride);
    while (amount--)
      setPixelColor(amount, color);
  }

  // Shifts all pixel color contents backward (toward pixel 0) by a given number of
  // pixels. If given a color, the farthest pixel(s) are loaded with that color; black
  // is the default. If not given a number of pixels to shift, 1 pixel is the default.
  // Does not display immediately; follow up with a strand.show() call.
  //
  void scrollBackward(uint16_t amount = 1, uint32_t color = 0)
  {
    if (!numPixels())
      return;
    uint16_t stride = bytesPerPixel();
    amount = amount % numPixels();
    if (!amount)
      return;
    memmove(pixels, pixels+stride*amount, (numPixels()-amount)*stride);
    while (amount--)
      setPixelColor(numPixels()-amount-1, color);
  }

protected:
  bool isRGB() const { return (wOffset == rOffset); }
  bool isRGBW() const { return (wOffset != rOffset); }
  uint8_t bytesPerPixel() const { return isRGBW()? 4 : 3; }

};

//
// Comments on the original Adafruit_NeoPixel code, which maybe Adafruit will read
// and incorporate in future versions.
//
// 1. The number of times the original code uses (wOffset == rOffset) to figure out
//    the channel storage scheme is excessive. It's not immediately obvious in
//    purpose but it's pervasive. Definitely wrap those in an inline member function
//    isRGB() and isRGBW() to make the code more clear and maintainable.
//
// 2. A class that does dynamic allocation (operator new or malloc()) should use a
//    virtual destructor, so that extension classes will get the right behavior
//    when code uses polymorphic calls.  This change costs one pointer per instance
//    of the class. Of course, using malloc() on a tiny microcontroller is already a
//    little bit risky as memory can fragment over multiple construct/destruct cycles
//    and you end up with a heap crash, but the likelihood that users will change
//    strand length during a run is pretty rare.
//

//-------------------------------------------------------------------------------------

#endif // __NEOSTRAND_H__

