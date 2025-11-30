#include <FastLED.h>
#include <vector>
#include <cstring>

#define NUM_LEDS_BODY 50 // Number of LEDs on body
#define NUM_LEDS_ORNAMENT 50 // Number of LEDs on ornament
#define DATA_PIN_BODY 4  // Teensy connection for body
#define DATA_PIN_ORNAMENT 3  // Teensy connection for ornament
#define BRIGHTNESS 63   // Brightness of LEDs (0-255)
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
struct SimpleChaseState
{
  LED_SEGMENT_RANGE *segment = nullptr;
  int step = 1;
  int length = 0;
  int offset = 0;
  int prevPos = -1;
  bool hasPrev = false;
};

bool initChaseState(const char *segmentName, SimpleChaseState &state)
{
  int idx = getSegmentIndexByName(segmentName);
  if (idx < 0)
  {
    return false;
  }

  LED_SEGMENT_RANGE &segment = ledSegments[idx];
  state.segment = &segment;
  state.step = (segment.start_index <= segment.end_index) ? 1 : -1;
  state.length = (state.step == 1) ? (segment.end_index - segment.start_index + 1) : (segment.start_index - segment.end_index + 1);
  state.offset = 0;
  state.prevPos = -1;
  state.hasPrev = false;
  return true;
}

// Advance one step of a chase; returns true if LEDs changed and sets wrapped when we loop
bool advanceChase(SimpleChaseState &state, bool &wrapped)
{
  wrapped = false;
  if (!state.segment || state.length <= 0)
  {
    return false;
  }

  LED_SEGMENT_RANGE &segment = *state.segment;

  int pos = segment.start_index + state.offset * state.step;
  segment.led_array[pos] = segment.defaultColor;

  if (state.hasPrev && state.prevPos != pos)
  {
    segment.led_array[state.prevPos] = CRGB::Black;
  }

  state.prevPos = pos;
  state.hasPrev = true;
  state.offset = (state.offset + 1) % state.length;
  wrapped = (state.offset == 0);
  return true;
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
  Serial.print("warmUp\\n");
  FastLED.clear(true);

  // Initialize antenna chase state
  SimpleChaseState leftChase{};
  SimpleChaseState rightChase{};
  initChaseState("LEFT_ANTENNA", leftChase);
  initChaseState("RIGHT_ANTENNA", rightChase);

  const char *sequence[] = {
      "LEFT_EYE",
      "RIGHT_EYE",
      "NOSE",
      "MOUTH",
      "HAIR_0",
      "HAIR_1",
      "LEFT_BOW",
      "RIGHT_BOW",
      "DRESS",
      "ORNAMENT_0",
      "ORNAMENT_1",
      "ORNAMENT_2",
      "ORNAMENT_3",
      "ORNAMENT_4"};

  const int sequenceCount = sizeof(sequence) / sizeof(sequence[0]);
  const unsigned long chaseInterval = 100; // ms per antenna step
  const unsigned long flashDuration = 250; // ms each segment stays on before moving on
  unsigned long lastChase = millis();
  bool leftFrozen = false;

  for (int s = 0; s < sequenceCount; ++s)
  {
    int idx = getSegmentIndexByName(sequence[s]);
    if (idx < 0)
    {
      continue;
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
    FastLED.show();

    unsigned long start = millis();
    while (millis() - start < flashDuration)
    {
      unsigned long now = millis();
      bool updated = false;
      bool rightWrapped = false;
      bool leftWrapped = false;
      if (now - lastChase >= chaseInterval)
      {
        if (!leftFrozen)
        {
          updated |= advanceChase(leftChase, leftWrapped);
          if (leftWrapped)
          {
            leftFrozen = true; // Hold on the last LED until the right antenna wraps
          }
        }
        updated |= advanceChase(rightChase, rightWrapped);

        // When the longer (right) antenna wraps, restart the left as well
        if (rightWrapped)
        {
          // Clear the left antenna before restarting it
          if (leftChase.segment)
          {
            LED_SEGMENT_RANGE &seg = *leftChase.segment;
            int stepClear = (seg.start_index <= seg.end_index) ? 1 : -1;
            for (int j = seg.start_index;; j += stepClear)
            {
              seg.led_array[j] = CRGB::Black;
              if (j == seg.end_index)
              {
                break;
              }
            }
          }

          leftChase.offset = 0;
          leftChase.prevPos = -1;
          leftChase.hasPrev = false;
          leftFrozen = false;
        }

        lastChase = now;
      }

      if (updated)
      {
        FastLED.show();
      }
      delay(5);
    }
  }

  // Leave antenna LEDs on after the chase
  setSegmentOn("LEFT_ANTENNA");
  setSegmentOn("RIGHT_ANTENNA");
  FastLED.show();
  delay(1000);
  FastLED.clear(true);
  delay(1000);
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
  // walkEachLED();
  warmUp();
  // allOn();
}
