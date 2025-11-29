#include <FastLED.h>
#include <vector>
#include <cstring>

#define NUM_LEDS_BODY 50 // Number of LEDs on body
#define NUM_LEDS_ORNAMENT 50 // Number of LEDs on ornament
#define DATA_PIN_BODY 4  // Teensy connection for body
#define DATA_PIN_ORNAMENT 3  // Teensy connection for ornament
#define BRIGHTNESS 255   // Brightness of LEDs (0-255)
#define LED_TYPE WS2811  // LED type
#define COLOR_ORDER RGB  // Color order for WS2811 LEDs

typedef enum
{
  BODY,
  ORNAMENT
} LED_ARRAY;

typedef struct
{
  LED_ARRAY arrayIndex;
  int start_index;
  int end_index;
  CRGB defaultColor;
  CRGB *led_array;
  const char *name;
} LED_SEGMENT_RANGE;

LED_SEGMENT_RANGE ledSegments[] = {
  // array, start index, end index, default color, led array ptr, name
    {BODY, 24, 24, CRGB::Blue, nullptr, "LEFT_EYE"},       // left eye
    {BODY, 23, 23, CRGB::Blue, nullptr, "RIGHT_EYE"},      // right eye
    {BODY, 22, 22, CRGB(191, 52, 0), nullptr, "NOSE"},      // nose (25% dimmer than OrangeRed)
    {BODY, 19, 21, CRGB::DarkRed, nullptr, "MOUTH"},       // mouth
    {BODY, 25, 30, CRGB::Yellow, nullptr, "HAIR_0"},       // hair 0
    {BODY, 33, 36, CRGB::Yellow, nullptr, "HAIR_1"},       // hair 1
    {BODY, 43, 44, CRGB::Red, nullptr, "LEFT_BOW"},        // left bow
    {BODY, 31, 32, CRGB::Red, nullptr, "RIGHT_BOW"},       // right bow
    {BODY, 45, 49, CRGB::Purple, nullptr, "LEFT_ANTENNA"}, // left antenna
    {BODY, 42, 37, CRGB::Purple, nullptr, "RIGHT_ANTENNA"}, // right antenna
    {BODY, 18, 0, CRGB::DeepPink1, nullptr, "DRESS"},       // dress
    {ORNAMENT, 49, 49, CRGB::Red, nullptr, "ORNAMENT_0"}, // ornament ring 0
    {ORNAMENT, 44, 48, CRGB::Red, nullptr, "ORNAMENT_1"}, // ornament ring 1
    {ORNAMENT, 34, 43, CRGB::Red, nullptr, "ORNAMENT_2"}, // ornament ring 2
    {ORNAMENT, 19, 33, CRGB::Red, nullptr, "ORNAMENT_3"}, // ornament ring 3
    {ORNAMENT, 0, 18, CRGB::Red, nullptr, "ORNAMENT_4"},  // ornament ring 4
};

CRGB leds_body[NUM_LEDS_BODY];
CRGB leds_ornament[NUM_LEDS_ORNAMENT];

// Find the index of a segment by its name string; returns -1 if not found
int getSegmentIndexByName(const char *segmentName)
{
  int segmentCount = sizeof(ledSegments) / sizeof(ledSegments[0]);
  for (int i = 0; i < segmentCount; ++i)
  {
    if (ledSegments[i].name && std::strcmp(ledSegments[i].name, segmentName) == 0)
    {
      return i;
    }
  }
  return -1;
}

void selfTest()
{
  FastLED.clear(true); // Ensure all LEDs start off

  // Fade from black to white
  for (int level = 0; level <= 255; level += 5)
  {
    CRGB shade(level, level, level);
    fill_solid(leds_body, NUM_LEDS_BODY, shade);
    fill_solid(leds_ornament, NUM_LEDS_ORNAMENT, shade);
    FastLED.show();
    delay(10);
  }

  // Fade from white back to black
  for (int level = 255; level >= 0; level -= 5)
  {
    CRGB shade(level, level, level);
    fill_solid(leds_body, NUM_LEDS_BODY, shade);
    fill_solid(leds_ornament, NUM_LEDS_ORNAMENT, shade);
    FastLED.show();
    delay(10);
  }

  FastLED.clear(true); // Ensure all LEDs end off
  delay(1000);
}

void walkEachLED()
{
  // Step through every LED individually so we can confirm wiring
  FastLED.clear(true);

  int segmentCount = sizeof(ledSegments) / sizeof(ledSegments[0]);

  for (int i = 0; i < segmentCount; ++i)
  {
    LED_SEGMENT_RANGE &segment = ledSegments[i];
    CRGB *ledArray = (segment.arrayIndex == BODY) ? leds_body : leds_ornament;
    CRGB activeColor = segment.defaultColor;

    int step = (segment.start_index <= segment.end_index) ? 1 : -1;

    for (int j = segment.start_index;; j += step)
    {
      ledArray[j] = activeColor;
      FastLED.show();
      delay(20);
//      ledArray[j] = CRGB::Black;

       // Break after hitting the end index (works in either direction)
      if (j == segment.end_index)
      {
        break;
      }
    }
  }

  delay(1000);
  FastLED.clear(true);
}

void setup()
{
  FastLED.addLeds<LED_TYPE, DATA_PIN_BODY, COLOR_ORDER>(leds_body, NUM_LEDS_BODY).setCorrection(TypicalLEDStrip);
  FastLED.addLeds<LED_TYPE, DATA_PIN_ORNAMENT, COLOR_ORDER>(leds_ornament, NUM_LEDS_ORNAMENT).setCorrection(TypicalLEDStrip);
  FastLED.setBrightness(BRIGHTNESS);
  FastLED.clear(true); // Ensure all LEDs start off
  FastLED.show();
  delay(100);
  selfTest();

  Serial.begin(9600); // Baud rate must match monitor_speed in platformio.ini

  // Loop over all ledSegments
  int segmentCount = sizeof(ledSegments) / sizeof(ledSegments[0]); // Calculate the number of segments
  for (int i = 0; i < segmentCount; ++i)
  {
    LED_SEGMENT_RANGE &segment = ledSegments[i];

    // Initialize LED array pointer based on the array index
    if (segment.arrayIndex == BODY)
    {
      segment.led_array = leds_body;
    }
    else if (segment.arrayIndex == ORNAMENT)
    {
      segment.led_array = leds_ornament;
    }
  }
}

void flashSegment(const char *segmentName, int time = 100, bool clear = false)
{
  int segmentIdx = getSegmentIndexByName(segmentName);
  if (segmentIdx < 0)
  {
    return;
  }

  LED_SEGMENT_RANGE &segment = ledSegments[segmentIdx];
  int i;
  int step = (segment.start_index <= segment.end_index) ? 1 : -1;

  for (i = segment.start_index;; i += step)
  {
    segment.led_array[i] = segment.defaultColor;

    if (i == segment.end_index)
    {
      break;
    }
  }
  FastLED.show();
  delay(time);

  if (clear)
  {
    for (i = segment.start_index;; i += step)
    {
      segment.led_array[i] = CRGB::Black;

      if (i == segment.end_index)
      {
        break;
      }
    }
  }
  delay(time);
}

// Simple chase effect across a named segment
void chaseSegment(const char *segmentName, int timePerLED = 100, int cycles = 2)
{
  int segmentIdx = getSegmentIndexByName(segmentName);
  if (segmentIdx < 0)
  {
    return;
  }

  LED_SEGMENT_RANGE &segment = ledSegments[segmentIdx];
  int step = (segment.start_index <= segment.end_index) ? 1 : -1;

  for (int c = 0; c < cycles; ++c)
  {
    for (int j = segment.start_index;; j += step)
    {
      segment.led_array[j] = segment.defaultColor;
      FastLED.show();
      delay(timePerLED);
      segment.led_array[j] = CRGB::Black;

      if (j == segment.end_index)
      {
        break;
      }
    }
  }
}

// Chase two segments in parallel
void chaseSegmentsTogether(const char *segmentNameA, const char *segmentNameB, int timePerLED = 100, int cycles = 2)
{
  int idxA = getSegmentIndexByName(segmentNameA);
  int idxB = getSegmentIndexByName(segmentNameB);
  if (idxA < 0 || idxB < 0)
  {
    return;
  }

  LED_SEGMENT_RANGE &segA = ledSegments[idxA];
  LED_SEGMENT_RANGE &segB = ledSegments[idxB];

  int stepA = (segA.start_index <= segA.end_index) ? 1 : -1;
  int stepB = (segB.start_index <= segB.end_index) ? 1 : -1;

  int lenA = (stepA == 1) ? (segA.end_index - segA.start_index + 1) : (segA.start_index - segA.end_index + 1);
  int lenB = (stepB == 1) ? (segB.end_index - segB.start_index + 1) : (segB.start_index - segB.end_index + 1);
  int maxLen = (lenA > lenB) ? lenA : lenB;

  for (int c = 0; c < cycles; ++c)
  {
    for (int n = 0; n < maxLen; ++n)
    {
      bool litA = false;
      bool litB = false;

      if (n < lenA)
      {
        int posA = segA.start_index + n * stepA;
        segA.led_array[posA] = segA.defaultColor;
        litA = true;
      }

      if (n < lenB)
      {
        int posB = segB.start_index + n * stepB;
        segB.led_array[posB] = segB.defaultColor;
        litB = true;
      }

      FastLED.show();
      delay(timePerLED);

      if (litA)
      {
        int posA = segA.start_index + n * stepA;
        segA.led_array[posA] = CRGB::Black;
      }

      if (litB)
      {
        int posB = segB.start_index + n * stepB;
        segB.led_array[posB] = CRGB::Black;
      }
    }
  }
}

// Leave an entire segment lit with its default color
void setSegmentOn(const char *segmentName)
{
  int idx = getSegmentIndexByName(segmentName);
  if (idx < 0)
  {
    return;
  }

  LED_SEGMENT_RANGE &segment = ledSegments[idx];
  int step = (segment.start_index <= segment.end_index) ? 1 : -1;
  for (int j = segment.start_index;; j += step)
  {
    segment.led_array[j] = segment.defaultColor;
    if (j == segment.end_index)
    {
      break;
    }
  }
}

void warmUp()
{
  Serial.print("warmUp\n");
  FastLED.clear(true);
  flashSegment("LEFT_EYE");
  flashSegment("RIGHT_EYE");
  flashSegment("NOSE");
  flashSegment("MOUTH");
  flashSegment("HAIR_0");
  flashSegment("HAIR_1");
  flashSegment("LEFT_BOW");
  flashSegment("RIGHT_BOW");
  flashSegment("LEFT_ANTENNA");
  flashSegment("RIGHT_ANTENNA");
  flashSegment("DRESS");
  flashSegment("ORNAMENT_0");
  flashSegment("ORNAMENT_1");
  flashSegment("ORNAMENT_2");
  flashSegment("ORNAMENT_3");
  flashSegment("ORNAMENT_4");
  // Chase both antenna segments in parallel at the end of warmup
  chaseSegmentsTogether("LEFT_ANTENNA", "RIGHT_ANTENNA", 150, 6);
  // Leave antenna LEDs on after the chase
  setSegmentOn("LEFT_ANTENNA");
  setSegmentOn("RIGHT_ANTENNA");
  FastLED.show();
  delay(1000);
  FastLED.clear(true);
  delay(100);
}

void allOn()
{
  Serial.print("allOn\n");
  // Loop over all ledSegments
  int segmentCount = sizeof(ledSegments) / sizeof(ledSegments[0]);
  for (int i = 0; i < segmentCount; ++i)
  {
    LED_SEGMENT_RANGE &segment = ledSegments[i];

    int step = (segment.start_index <= segment.end_index) ? 1 : -1;
    for (int j = segment.start_index;; j += step)
    {
      segment.led_array[j] = segment.defaultColor;

      if (j == segment.end_index)
      {
        break;
      }
    }
  }
  FastLED.show();
  delay(1000);
  FastLED.clear(true);
  delay(1000);
}

void loop()
{
  walkEachLED();
  warmUp();
  allOn();
}
