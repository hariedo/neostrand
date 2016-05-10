// Generic but useful Arduino helper functions and macros.
//
// Generic.h
// Copyright (c) by Ed Halley and Jaime Halley
//
// Generic.h is licensed under a
// Creative Commons Attribution-ShareAlike 4.0 International License.
// 
// You should have received a copy of the license along with this
// work. If not, see <http://creativecommons.org/licenses/by-sa/4.0/>.
//

#ifndef __GENERIC_H__
#define __GENERIC_H__

//-------------------------------------------------------------------------------------

// The sizeof operator gives the byte size of the whole array.
// This macro gives the number of elements in an array.
//
#define countof(array)  ( sizeof(array) / sizeof(*(array)) )

//-------------------------------------------------------------------------------------

// Entropy is a measure of disorder or chaos.
//
// In cryptography or random numbers, you want to mix in any available unpredictable
// numbers so that the calculations will be highly unpredictable.
//
// An Arduino has very poor support for strong entropy, and it would be time-consuming
// to gather it anyway.  This function collects what is readily available with minimal
// delay:  all of the device ports.
//
// Call this function when you need to seed a PRNG but not whenever you need a random
// number from the PRNG.  For example, shuffle the deck once (to seed) but draw many
// cards from the deck before shuffling again.
//
// Enhancements to get stronger entropy would be to attach a sensor that reads from a
// real-world chaotic input (humidity, noise, vibrations, camera pixel data, etc.).
// This routine would then naturally combine that data with other cheap entropy data.
// Additionally, store some seed state into EEPROM:  read a previously recorded value
// from EEPROM, combine it with new entropy, seed the PRNG from this, and store a new
// value back into EEPROM for the next power-up.  Don't write EEPROM excessively as it
// will wear out over time.
//
inline unsigned long getCheapEntropy()
{
  static unsigned long entropy = 0xDEADBEEF;
  static unsigned char shift = 0x0F;

  // The current low-order bits of Arduino runtime in micros() is great... if this
  // function is called after a human-based interaction.  If only called during the
  // Arduino setup() function or called on a very regular basis, it's going to be
  // horribly predictable.
  //
  entropy ^= micros() << shift;

  // The analog input pins will always have a little bit of repeatability error in
  // them.  This is almost zero if hardwired to ground or VCC, or configured as an
  // output pin.  The repeatability jitter is pretty weak if attached to a real input
  // device.  If a pin is left unconnected there is a little more room for
  // unpredictable values coming in.  Note that small Arduino Pro Mini style boards
  // often have no connection points (or inconvenient ones) attached to the analog
  // pins A4 and A5.
  //
  entropy ^= analogRead(0) << shift;
  entropy ^= analogRead(1) << (shift+3);
  entropy ^= analogRead(2) << (shift+6);
  entropy ^= analogRead(3) << (shift+9);
  entropy ^= analogRead(4) << (shift+12);
  entropy ^= analogRead(5) << (shift+15);

  // The digital input pins can be read 8 at a time.  These are even less likely to
  // be useful as entropy sources unless the user is actively pushing buttons or
  // there is some kind of data transmission going on across the SDI or serial pins
  // at the time.
  //
  entropy ^= PORTB << shift;
  entropy ^= PORTC << (shift+8);
  entropy ^= PORTD << (shift+16);

  // On successive calls to this routine, we alter how we mix our inputs into the
  // current entropy pool.  This helps ensure that calling us multiple times does
  // not defeat the entropy gained.  However, it's still not a good idea to call
  // this routine constantly, such as in every main loop().
  //
  shift = entropy & 0x0F;

  return entropy;
}

// Retrieve a value INPUT, INPUT_PULLUP, or OUTPUT, from the current pin setting.
//
inline int getPinMode(uint8_t pin)
{
  uint8_t bit = digitalPinToBitMask(pin);
  uint8_t port = digitalPinToPort(pin);

  if (port == NOT_A_PIN)
    return -1;

  volatile uint8_t* reg = portModeRegister(port);
  volatile uint8_t* out = portOutputRegister(port);

  if ((~(*reg) & bit) == bit) // INPUT OR PULLUP
  {
    if ((~(*out) & bit) == bit)
      return INPUT;
    else
      return INPUT_PULLUP;
  }

  return OUTPUT;
}

// Turn the INPUT or INPUT_PULLUP digital input into a useful 0=unpressed/1=pressed return value.
//
inline bool isButtonPressed(uint8_t pin)
{
  int mode = getPinMode(pin);
  if (mode == OUTPUT || mode < 0)
    return false;
  bool pressed = digitalRead(pin);
  if (mode == INPUT_PULLUP)
    pressed = !pressed;
  return pressed;
}

//-------------------------------------------------------------------------------------

#endif // __GENERIC_H__

