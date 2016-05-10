// Vocaloid NeoPixel Strand Controller
// Copyright (c) by Ed Halley and Jaime Halley
//
// Vocaloid NeoPixel Strand Controller is licensed under a
// Creative Commons Attribution-ShareAlike 4.0 International License.
// 
// You should have received a copy of the license along with this
// work. If not, see <http://creativecommons.org/licenses/by-sa/4.0/>.
//

#include "Generic.h"

//
// Circuit Concept for the Vocaloid NeoPixel Strand Controller:
//
//                                  |< accessory >| == |< character color >|
// Battery ==== Arduino =========== |A|A|A|A|A|A|A| == |C|C|C|C|C|C|C|...|C|
//              ^B ^B ^Buttons
//
// A battery is attached to a small box with an Arduino and three momentary buttons.
// A longer cord stretches from the small box to a hairpiece that has one or two
// NeoPixel(tm) WS2812-style shift register RGB LED devices in series.  The wearer can
// select the modes and special effects by pressing combinations of buttons on the
// control box.  The first N pixels are dedicated as an accent/accessory color, while
// the remaining pixels are intended to display the main character signature color.
// These pixels are typically arranged on the wearer's head, such as from a hairband.
// There are convenient rings of NeoPixels that work well as the hair accessory.
// If no hair accessory component is attached, then set the ACCESSORY_LENGTH to 0 and
// no pixels for accent colors will be reserved.  Set the CHARACTER_LENGTH to the
// length of the main NeoPixel device; the macro STRAND_LENGTH is the sum total of
// these.
//
// To celebrate the main Vocaloid star, Hatsune Miku, the wearer should have two
// complete (independent) hardware units, with a 12- or 16-LED ring for the accessory
// and a 1-meter high density 144-pixel strand for each of two long ponytails
// (twintails).
//
// A large-capacity USB charger that can supply over 2A per port is sufficient if the
// LEDs are not driven at full power.  A dimming potentiometer option is included to
// adjust the brightness.  The pixels will jitter and show inaccurate colors if there
// is not enough current available for the brightness setting.  If not using a USB
// charging device and your supply is more than 5V, you MUST INCLUDE a suitable 5V DC
// REGULATOR or you will damage the NeoPixels.  An Arduino's DC regulator is typically
// not able to drive multiple amps of current.
//

//-------------------------------------------------------------------------------------

//
// Define all our program constants.  These are chosen based on the wiring and timing,
// and never change without recompiling and re-loading the Arduino.
//

// We connect a NeoPixel strand to one data pin on the Arduino.  We have to declare
// how many LEDs (total) are in the overall NeoPixel strand.  We can reserve a certain
// portion of the top of the strand for an accessory color, or use 0 for no accessory.
//
#include "NeoStrand.h"
#define STRAND_PIN 11
#define ACCESSORY_LENGTH 16
#define CHARACTER_LENGTH 144
#define STRAND_LENGTH (ACCESSORY_LENGTH+CHARACTER_LENGTH)
NeoStrand strand = NeoStrand(STRAND_LENGTH, STRAND_PIN);

// We connect three momentary buttons (with helpfully colored caps) to three data
// pins on the Arduino.  The combination of these buttons will be monitored.  So
// if the user pushes one button, such as the LUKA/pink button, then LUKA's mode
// will be selected; if the user pushes a combo like LUKA+TWIN, then a different mode
// is selected instead.  In this way, we have a maximum of 8 combos (all off, three
// ways to push one button, three ways to push two buttons together, and one combo
// if pushing all three buttons together. 
//
#define LUKA_BUTTON 9
#define TWIN_BUTTON 8
#define MIKU_BUTTON 7

// If a special debugging button is wired up, we can react to it.  This can be
// useful on the breadboard stage, but left out of the final circuit buildup.
//
#define DEBUG_BUTTON 4

// These are timing constants that we use to control the speed of various parts of
// the sketch.  Each loop cycle is roughly 1~3ms, depending on the length of the
// strand.
//
#define SCROLL_CYCLES 2
#define RAINBOW_CYCLES 2
#define CONFIRMATION_CYCLES 12
#define HISTORY_CYCLES 250
#define HOLD_MODE_CYCLES 800

// We want to keep some historical data on recent button pushes, to detect special
// patterns of presses like hold, double-tap, etc.  In this way, we can greatly
// increase the power of the limited user interface.
//
#define HISTORY_LENGTH 6
unsigned long history_millis = 0;
unsigned long history_time[HISTORY_LENGTH];
int history_vector[HISTORY_LENGTH];

// This list of names corresponds to all of the major Vocaloid modes that we want
// to support.  Since we have three buttons, we have a maximum of eight MAJOR modes
// that are easy to select.  The mode numbers are also used as the "input vector,"
// representing the user's button combination pressed at any given instant.
//
enum
{
  NOBODY=0,
  MIKU=1,   // green alone gives miku aquamarine (and dark red accent)
  TWIN=2,   // yellow alone for the blonde twins (and white accent)
  KAITO=3,  // green+yellow gives bold blue (and baby blue accent)
  LUKA=4,   // red button alone gives pink (and teal accent)
  HAKU=5,   // green+red gives white (and pastel purple accent)
  MEIKO=6,  // yellow+red gives bold red (and brown accent)
  EVERYONE, // all three buttons
};

// The current major Vocaloid character mode.
int mode = NOBODY;

// For each vocaloid mode we support, we have a signature color based on a
// Vocaloid(tm) character's hair or costume color.  These colors can be any RGB
// color, 0~255 Red + 0~255 Green + 0~255 Blue.  Brighter is better.
//
uint32_t VocaloidColors[] =
{
  strand.Color(0, 0, 0),       // NOBODY
  strand.Color(40, 255, 130),  // MIKU AQUAMARINE
  strand.Color(200, 200, 30),  // TWINS RIN/LEN BLONDE
  strand.Color(10, 10, 255),   // KAITO BLUE
  strand.Color(240, 70, 70),   // LUKA PINK
  strand.Color(180, 180, 180), // IA/HAKU SILVERY WHITE OUTFIT
  strand.Color(255, 10, 10),   // MEIKO RED OUTFIT
  strand.Color(200, 0, 200),   // RAINBOW [updated all the time]
};

// For each vocaloid mode we support, we have a secondary color based on the
// same Vocaloid(tm) character's costume accent color.
//
uint32_t AccessoryColors[] =
{
  strand.Color(0, 0, 0),       // NOBODY
  strand.Color(80, 10, 10),    // MIKU RED/BLACK HAIR RINGS
  strand.Color(200, 200, 200), // TWINS RIN/LEN WHITE HAIR BOW
  strand.Color(70, 70, 140),   // KAITO BABY BLUE SCARF
  strand.Color(40, 200, 250),  // LUKA TEAL HEADPHONES
  strand.Color(180, 100, 180), // IA/HAKU PASTEL MAGENTA HAIR
  strand.Color(30, 20, 10),    // MEIKO BROWN HAIR
  strand.Color(200, 0, 200),   // RAINBOW [updated all the time]
};

// This list of names corresponds to all of the minor effect modes that we want to
// support.  We can have as many as we want here, but we have to have some kind of
// way of tapping, pushing, double-tapping, or holding the buttons to achieve the
// selection, so keep it simple.
//
enum
{
  NONE=0,     // no change
  SOLID,      // default effect for a single tap
  PULSING,    // tap the buttons for at least 3 beats, it will try to match rhythm
  SPARKLING,  // double-tap the buttons quickly and stop
  SHUTDOWN,   // hold the buttons for 3 seconds
};
unsigned long pulsingPeriod = 1000;

// NeoPixel brightness ranges from 0~255.
// Full brightness is energy-inefficient and blindingly bright.  We also want to
// reserve some extra brightness for special effects. So here, we define the
// typical brightness level.
//
#define RESTING_BRIGHTNESS 130
#define DIMMER_PIN (A0)
int dimmer = 255;

//-------------------------------------------------------------------------------------

// The "setup" function is run one time, shortly after power is provided.
// In this function, we can perform whatever things we think are necessary
// before the main loop begins.
//
void setup()
{
  Serial.begin(115200);

  // Buttons are input devices.
  // For these buttons, we will use the Arduino internal pullup-resistor mechanism,
  // which means that they will read as HIGH when not pushed, and LOW when pushed.
  // Wire each button to their respective input pin and to the common circuit ground.
  //
  pinMode(LUKA_BUTTON, INPUT_PULLUP);
  pinMode(TWIN_BUTTON, INPUT_PULLUP);
  pinMode(MIKU_BUTTON, INPUT_PULLUP);
  pinMode(DEBUG_BUTTON, INPUT_PULLUP);
  //pinMode(DIMMER_PIN, INPUT);
  
  // LEDs are output devices.
  // Set up the NeoStrand device which will initialize the pin mode and all of the
  // pixels on the strand are cleared to black/off.
  //
  strand.begin();
  strand.show();

  // When we first power on, we wait for user input before full effect.
  //
  mode = performWait(MIKU);
  mode = performBoot(mode);
}

//-------------------------------------------------------------------------------------

int performWait(int target)
{
  // Wipe the slate clean first.
  strand.wipeWithColor(0);
  strand.show();

  // Ensure we're not already being manipulated with buttons.
  while (NOBODY != getConfirmedInputVector())
    delay(1);

  // Indicate power is available on the first pixel.
  // Otherwise you might forget you need to start pushing buttons.
  //
  uint32_t color;
  int decay = 0;
  int delta = +1;
  while (NOBODY == getConfirmedInputVector())
  {
    // The EVERYONE mode's special rainbow color is updated all the time.
    updateRainbow();
    color = VocaloidColors[target];
    
    strand.setPixelColor(ACCESSORY_LENGTH, NeoStrand::Bright(color, decay));
    strand.show();
    delay(1);

    // breathing pattern
    decay += delta;
    if (decay > 255 || decay < 0)
    {
      decay = constrain(decay, 0, 255);
      delta = -delta;
    }
  }

  while (1)
  {
    // The EVERYONE mode's special rainbow color is updated all the time.
    updateRainbow();
    color = VocaloidColors[target];

    strand.setPixelColor(ACCESSORY_LENGTH, color);
    strand.show();
    delay(1);

    int vector = getConfirmedInputVector();
    if (vector == NOBODY)
      break;
    target = vector;
  }

  // We have poor entropy at power startup, but it is much better
  // if we can allow for some kind of user interaction before seeding.
  // So we seed the number generator after this first button push.
  // Luckily, quality random numbers aren't too important here.
  //
  randomSeed(getCheapEntropy());

  return target;
}

int performBoot(int target)
{
  int first = target;
  int duration = 400;

  // Slowly build up the distribution of sparkles.
  //
  int decay = 0;
  for (int i = 0; i < duration; i++)
  {
    // The EVERYONE mode's special rainbow color is updated all the time.
    updateRainbow();

    uint32_t color = VocaloidColors[target];
    
    int chance = random(duration);
    if (chance < i)
    {
      decay = 255;
      if (random(1000) < 50 && i < duration*7/8)
        target = random(8);
      else
        target = first;
    }
    else
    {
      decay /= 3;
      decay = constrain(decay, 0, 255);
      color = NeoStrand::Bright(color, decay);
    }

    dimmer = updateDimmer();
    if (dimmer < 255)
      color = NeoStrand::Bright(color, dimmer);

    strand.scrollForward();
    strand.setPixelColor(0, color);
    strand.show();

    // Ramp in the speed of the cascade.
    //
    int speed = map(i, 0, duration, 20, 1);
    delay(speed);
  }
  target = first;

  // Fade down into resting brightness level.
  //
  duration = 300;
  for (int i = 0; i < duration+STRAND_LENGTH; i++)
  {
    uint32_t color = VocaloidColors[target];
    int decay = map(i, 0, duration, 255, RESTING_BRIGHTNESS);
    decay = constrain(decay, RESTING_BRIGHTNESS, 255);
    color = NeoStrand::Bright(color, decay);
    if (dimmer < 255)
      color = NeoStrand::Bright(color, dimmer);
    strand.scrollForward();
    strand.setPixelColor(0, color);
    strand.show();
    delay(1);
  }

  // Past user input is irrelevant.
  //
  clearHistory();

  return target;
}

//-------------------------------------------------------------------------------------

// The "loop" function is run repeatedly, until power is removed or the
// device is reset.  In this function, we can perform whatever things we
// want to have happen endlessly.
//
void loop()
{
  unsigned long now = millis();

  // Check if debugging has been requested.
  updateDebug();

  // The EVERYONE mode's special rainbow color is updated all the time.
  updateRainbow();

  // Check all our inputs and get one number representing all the buttons together.
  dimmer = updateDimmer();

  int currentVector = getConfirmedInputVector();

  // Any positive change in confirmed vector instantly changes the overall "mode"
  // of our system to a new Vocaloid character, and feeds the new mode's color into
  // the top of the strand.
  //
  static int effect = SOLID;
  static int lastMode = EVERYONE;
  if (currentVector != NOBODY)
  {
    mode = currentVector;
    if (mode != lastMode)
      effect = SOLID;
  }

  // Monitor the history of this input vector.  Certain timing patterns can be
  // detected to select a sub-mode special effect.
  //
  int command = detectEffectCommand(currentVector);
  if (command != NONE)
    effect = command;

  // Special combo of holding all buttons means to go dark instead.
  //
  if (effect == SHUTDOWN)
  {
    strand.wipeWithColor(0, 1);
    mode = performWait(mode);
    mode = performBoot(mode);
    clearHistory();
    lastMode = NOBODY;
    effect = SOLID;
  }

  updateStrand(mode, effect);

  // Tell the strand device we've finally decided what we want to display.
  strand.show();
  while (now == millis())
    ;

  lastMode = mode;
}

//-------------------------------------------------------------------------------------

// Sometimes it can be tough to figure out why a feature is not working, since you
// can't just look inside a variable while the chip is running the program.  So this
// function offers a way to pause and report any useful information back to the
// host computer.
//
// If the button is pushed, print anything you want, then wait for the button to be
// released.  You don't even need an actual button.  Just short the DEBUG_BUTTON pin
// to any ground pin.
//
void updateDebug()
{
  if (isButtonPressed(DEBUG_BUTTON))
  {
    // Print whatever you want back to the host computer.
    Serial.print("---\n");
    Serial.print("history_vector = {");
    for (int i = 0; i < HISTORY_LENGTH; i++)
    {
      if (i > 0) Serial.print(", ");
      Serial.print(history_vector[i]);
    }
    Serial.print("};\n");
    Serial.print("history_time = {");
    for (int i = 0; i < HISTORY_LENGTH; i++)
    {
      if (i > 0) Serial.print(", ");
      Serial.print(history_time[i]);
    }
    Serial.print("};\n");

    Serial.print("dimmer = ");
    Serial.print(dimmer);
    Serial.print(";\n");

    // Wait for the debug button to be released, so as not to spam the terminal.
    delay(100);
    while (isButtonPressed(DEBUG_BUTTON))
      ;
  }
}

//-------------------------------------------------------------------------------------

// Read the potentiometer option and scale the results to our NeoPixel dimmer range.
// We don't allow the potentiometer to dim all the way down to zero, because it makes
// it pretty confusing to debug the circuit with no lights.  We also don't want it to
// go too dark because the colors get a bit inaccurate if scaled too low.
//
int updateDimmer()
{
    int trimmer = analogRead(DIMMER_PIN - A0);
    dimmer = map(trimmer, 0, 1023, 100, 255);
    return dimmer;
}

// Wipe the history arrays of all the past user input behavior.
//
void clearHistory()
{
  memset(history_time, 0, sizeof(history_time));
  memset(history_vector, 0, sizeof(history_vector));
  history_millis = millis();
}

// Check out the history arrays to see if the user has executed a triple-tap of the
// same button vector combo.  If so, set up the PULSING effect.
//
int detectTripleTap()
{
#if HISTORY_LENGTH < 6
#error Need more HISTORY_LENGTH to support detectTripleTap()
#endif

  // The vector history arrays need to match this pattern:
  //
  //                   < most recent                          least recent >
  // history_vector:   [ NOBODY, x, NOBODY,     x, NOBODY, x, ... ]
  // history_time:     [      ?, y,      w,     y,      w, ?, ... ]
  //
  // The 'y+w' times have to be approximately equal (within HISTORY_CYCLES).
  // The pulse rate becomes defined as the average of the last two 'y+w' times.
  //
  if (history_vector[0] == NOBODY &&
      history_vector[2] == NOBODY &&
      history_vector[4] == NOBODY)
  {
    if (history_vector[1] == NOBODY)
      return NONE;

    if (history_vector[1] == history_vector[3] &&
        history_vector[3] == history_vector[5])
    {
      unsigned long period0 = history_time[1]+history_time[2];
      unsigned long period1 = history_time[3]+history_time[4];
      if (period1 > period0 && period1-period0 > HISTORY_CYCLES)
        return NONE;
      if (period0 > period1 && period0-period1 > HISTORY_CYCLES)
        return NONE;

      pulsingPeriod = (period0+period1)/2;
      return PULSING;
    }
  }

  return NONE;
}

// Check out the history arrays to see if the user has executed a double-tap of the
// same button vector combo.  If so, set up the SPARKLING effect.
//
int detectDoubleTap()
{
#if HISTORY_LENGTH < 4
#error Need more HISTORY_LENGTH to support detectDoubleTap()
#endif

  // The vector history arrays need to match this pattern:
  //
  //                   < most recent           least recent >
  // history_vector:   [ NOBODY, x, NOBODY, x, ... ]
  // history_time:     [      ?, ?,  short, ?, ... ]
  //
  if (history_vector[0] == NOBODY && history_vector[2] == NOBODY)
  {
    if (history_vector[1] == NOBODY)
      return NONE;

    if (history_vector[1] == history_vector[3])
    {
      if (history_time[2] > HISTORY_CYCLES*2)
        return NONE;

      return SPARKLING;
    }
  }

  return NONE;
}

// Check out the history arrays to see if the user has executed a long-tap of the
// same button vector combo.  If so, set up the SHUTDOWN effect.
//
int detectLongTap()
{
#if HISTORY_LENGTH < 2
#error Need more HISTORY_LENGTH to support detectLongTap()
#endif

  // The vector history arrays need to match this pattern:
  //
  //                   < most recent   least recent >
  // history_vector:   [ x,    ... ]
  // history_time:     [ long, ... ]
  //
  if (history_vector[0] != NOBODY)
  {
    if (history_time[0] > HOLD_MODE_CYCLES)
      return SHUTDOWN;
  }

  return NONE;
}

// This routine records the recent history of user actions, and then checks if
// any of the special effect commands can be detected in the history data.
//
int detectEffectCommand(int vector)
{
  unsigned long now = millis();

  // If the vector of input buttons has changed at all, we record
  // this change in the history.  The last HISTORY_LENGTH vector
  // states are kept.
  //
  if (vector != history_vector[0])
  {
    // push the new vector onto the stack
    memmove(&history_vector[1], &history_vector[0],
            sizeof(*history_vector)*(HISTORY_LENGTH-1));
    memmove(&history_time[1], &history_time[0],
            sizeof(*history_time)*(HISTORY_LENGTH-1));
    history_vector[0] = vector;
    history_millis = now;
  }

  // The latest entry in the history is always counting up from the last change.
  //
  int since = (now - history_millis);
  history_time[0] = since;

  int effect = NONE;
  if (effect == NONE)
    effect = detectTripleTap();
  if (effect == NONE)
    effect = detectDoubleTap();
  if (effect == NONE)
    effect = detectLongTap();

  return effect;
}

//-------------------------------------------------------------------------------------

// Make any strand lighting updates desired.  This is based on a given major character
// mode and also a special effect.  In our case, we rely heavily on the NeoStrand's
// "scrollForward" function, to allow new modes to appear at the top of the strand and
// wash down the strand like a waterfall.
// 
void updateStrand(int mode, int effect)
{
  unsigned long now = millis();

  // Scroll the current mode down the strand at the appropriate speed.
  static int heldScroll = 0;
  heldScroll++;
  if (heldScroll >= SCROLL_CYCLES)
  {
    heldScroll = 0;
    strand.scrollForward();
  }

  // Grab the base color for the current character mode.
  //
  uint32_t color = VocaloidColors[mode];

  // Apply the correct effect as temporal variations of the base color.
  //
  if (mode != EVERYONE && mode != NOBODY)
  {
    switch (effect)
    {
    default:
    case SOLID:     color = applySolidEffect(color, now); break;
    case PULSING:   color = applyPulsingEffect(color, now); break;
    case SPARKLING: color = applySparklingEffect(color, now); break;
    case SHUTDOWN:    color = applyShutdownEffect(color, now); break;
    }
  }

  // Master dimmer applied last.
  //
  if (dimmer < 255)
    color = NeoStrand::Bright(color, dimmer);

  // Give the final color to the top of the strand.
  //
  strand.setPixelColor(ACCESSORY_LENGTH, color);

#if ACCESSORY_LENGTH > 0

  // Now grab the accent color for the current character mode.
  //
  color = AccessoryColors[mode];

  // Apply different effects to the accessory color.
  //
  static unsigned int heldAccessory = 0;
  if (heldScroll == 0)
    heldAccessory++;
  int decay;
  switch (mode)
  {
  case NOBODY:
    color = 0;
    break;

  case MIKU: // black and red alternate
    if ((heldAccessory % ACCESSORY_LENGTH) < (ACCESSORY_LENGTH / 2))
      color = 0;
    break;

  case LUKA: // accessory color pulsates bright and dim
    decay = map(heldAccessory % ACCESSORY_LENGTH,
                0, ACCESSORY_LENGTH-1,
                255, 100);
    color = NeoStrand::Bright(color, decay);
    break;
  
  case EVERYONE:
    color = VocaloidColors[mode];
    break;
  
  default: break;
  }

  // Master dimmer applied last.
  //
  if (dimmer < 255)
    color = NeoStrand::Bright(color, dimmer);

  // Give the final color to the top of the strand.
  //
  strand.setPixelColor(0, color);

#endif
}

uint32_t applySolidEffect(uint32_t color, unsigned long now)
{
  // Fresh color change is bright; fades to resting brightness soon after.
  unsigned long since = now - history_millis;
  if (history_vector[0] != NOBODY)
    color = color;
  else if (since < 200)
    color = strand.Bright(color, map(since, 0, 200, 255, RESTING_BRIGHTNESS));
  else
    color = strand.Bright(color, RESTING_BRIGHTNESS);
  return color;
}

uint32_t applyPulsingEffect(uint32_t color, unsigned long now)
{
  // Right on the pulsing beat frequency is bright; fades to resting brightness.
  unsigned long since = now - history_millis + history_time[1];
  while (since > pulsingPeriod)
    since -= pulsingPeriod;
  if (history_vector[0] != NOBODY)
    color = color;
  else if (since < 200)
    color = strand.Bright(color, map(since, 0, 200, 255, RESTING_BRIGHTNESS));
  else
    color = strand.Bright(color, RESTING_BRIGHTNESS);
  return color;
}

uint32_t applySparklingEffect(uint32_t color, unsigned long now)
{
  // Jitter the brightness randomly.
  color = strand.Bright(color, random(150, 255));
}

uint32_t applyShutdownEffect(uint32_t color, unsigned long now)
{
  // Nothing to do here.
}

// This function keeps track of a cycling hue that is used to generate a prismatic
// rainbow effect.  It uses the NeoStrand "wheel" function to calculate a rainbow
// color.
//
void updateRainbow()
{
  static uint8_t wheel = 0;
  static int heldWheel = 0;
  
  heldWheel++;
  if (heldWheel >= RAINBOW_CYCLES)
  {
    wheel++;
    heldWheel = 0;
    VocaloidColors[EVERYONE] = strand.Wheel(wheel);
  }
}

// Get the total combination of button presses at the current instant.
// This function performs two important features.
//
// (1) combine all three buttons' pressed/unpressed status into one number
//     called an input vector
//
// (2) don't accept a change in the input vector until a certain confirmation
//     time has elapsed; this makes it easier for the user to go from zero
//     buttons pressed to two buttons pressed even if they don't get pressed
//     at the exact same instant
//
// Lastly, we flicker the onboard LED on the Arduino, just to show that there
// is some activity on the buttons.  This is very useful during the breadboard
// stage when you're not sure if the circuit is wrong or the software has
// crashed.
//
int getConfirmedInputVector()
{
  static int lastConfirmedVector = NOBODY;
  static int lastVector = -1;
  static int heldVector = 0;

  // Combine the inputs.
  int rawVector =
    isButtonPressed(LUKA_BUTTON) << 2 |
    isButtonPressed(TWIN_BUTTON) << 1 |
    isButtonPressed(MIKU_BUTTON) << 0;

  // On a change in vector, don't return the new one!
  // Instead, flicker the light and return the OLD one.
  //
  if (rawVector != lastVector)
  {
    heldVector = 0;
    lastVector = rawVector;
    digitalWrite(13, HIGH);
    return lastConfirmedVector;
  }
  digitalWrite(13, LOW);
  
  // We only update the confirmed vector after it has
  // been held steady for long enough to rule out any
  // accidental/sloppy half-presses or electric bounces.
  //
  heldVector++;
  if (heldVector >= CONFIRMATION_CYCLES)
  {
    lastConfirmedVector = rawVector;
  }

  return lastConfirmedVector;
}

