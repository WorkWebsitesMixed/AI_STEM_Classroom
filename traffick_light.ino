// ============================================================
//  Pedestrian Traffic Light — ESP32 (Wokwi simulation)
//
//  Normal vehicle cycle:
//    RED  (10 s) → GREEN (8 s) → YELLOW (3 s) → repeat
//
//  Pedestrian interrupt:
//    If the button is pressed while RED is active, a
//    PEDESTRIAN GREEN signal fires for 6 s before the
//    normal cycle continues with vehicle GREEN.
// ============================================================

// ---------- Pin definitions ----------
const int PIN_RED    = 26;   // Vehicle red    LED
const int PIN_YELLOW = 27;   // Vehicle yellow LED
const int PIN_GREEN  = 14;   // Vehicle green  LED
const int PIN_PED    = 12;   // Pedestrian green LED (WALK signal)
const int PIN_BTN    = 4;    // Pedestrian push-button (active-LOW)

// ---------- Timing constants (milliseconds) ----------
const unsigned long T_RED    = 10000UL;  // Vehicle red phase
const unsigned long T_GREEN  =  8000UL;  // Vehicle green phase
const unsigned long T_YELLOW =  3000UL;  // Vehicle yellow phase
const unsigned long T_PED    =  6000UL;  // Pedestrian walk signal

// ---------- State machine ----------
// Using an enum keeps the code readable and avoids "magic numbers".
enum TrafficState {
  STATE_RED,
  STATE_PED_GREEN,
  STATE_GREEN,
  STATE_YELLOW
};

TrafficState currentState = STATE_RED;   // Start with red
unsigned long stateStart  = 0;           // When the current state began
bool pedRequested         = false;       // Has the button been pressed?

// ============================================================
//  Helper — turn every LED off, then light only the ones
//  passed in.  This prevents accidental overlaps.
// ============================================================
void setLights(bool red, bool yellow, bool green, bool ped) {
  digitalWrite(PIN_RED,    red    ? HIGH : LOW);
  digitalWrite(PIN_YELLOW, yellow ? HIGH : LOW);
  digitalWrite(PIN_GREEN,  green  ? HIGH : LOW);
  digitalWrite(PIN_PED,    ped    ? HIGH : LOW);
}

// ============================================================
//  Transition to a new state:
//    1. Update currentState
//    2. Record the start time
//    3. Set the correct LEDs
// ============================================================
void enterState(TrafficState next) {
  currentState = next;
  stateStart   = millis();

  switch (next) {

    case STATE_RED:
      // Vehicle red ON — pedestrians must wait
      setLights(true, false, false, false);
      Serial.println("[RED]  Vehicle stopped — waiting 10 s");
      break;

    case STATE_PED_GREEN:
      // Pedestrian walk signal ON — all vehicle lights OFF
      setLights(false, false, false, true);
      Serial.println("[WALK] Pedestrian crossing — 6 s");
      break;

    case STATE_GREEN:
      // Vehicle green ON — normal traffic flow
      setLights(false, false, true, false);
      Serial.println("[GRN]  Vehicle moving  — 8 s");
      break;

    case STATE_YELLOW:
      // Vehicle yellow ON — prepare to stop
      setLights(false, true, false, false);
      Serial.println("[YLW]  Slowing down    — 3 s");
      break;
  }
}

// ============================================================
//  setup() — runs once at power-on / reset
// ============================================================
void setup() {
  Serial.begin(115200);
  Serial.println("=== Pedestrian Traffic Light booting ===");

  // Configure LED pins as outputs
  pinMode(PIN_RED,    OUTPUT);
  pinMode(PIN_YELLOW, OUTPUT);
  pinMode(PIN_GREEN,  OUTPUT);
  pinMode(PIN_PED,    OUTPUT);

  // Button input with the internal pull-up resistor enabled.
  // This means the pin reads LOW when the button is pressed.
  pinMode(PIN_BTN, INPUT_PULLUP);

  // Start in the RED state
  enterState(STATE_RED);
}

// ============================================================
//  loop() — runs repeatedly after setup()
// ============================================================
void loop() {

  // ---- 1. Read the pedestrian button ----
  // digitalRead returns LOW when button is pressed (pull-up logic).
  // We only care about button presses during the RED phase;
  // pressing at any other time is silently ignored.
  if (digitalRead(PIN_BTN) == LOW) {
    if (currentState == STATE_RED && !pedRequested) {
      pedRequested = true;
      Serial.println("       [BTN] Pedestrian request registered!");
    }
  }

  // ---- 2. Check if the current phase has timed out ----
  unsigned long elapsed = millis() - stateStart;

  switch (currentState) {

    // --- RED phase ---
    case STATE_RED:
      if (elapsed >= T_RED) {
        // Red phase is over.  If a pedestrian pressed the button,
        // give them the walk signal; otherwise go straight to green.
        if (pedRequested) {
          pedRequested = false;       // Clear the request flag
          enterState(STATE_PED_GREEN);
        } else {
          enterState(STATE_GREEN);
        }
      }
      break;

    // --- Pedestrian WALK phase ---
    case STATE_PED_GREEN:
      if (elapsed >= T_PED) {
        // Walk time is up — resume normal vehicle flow
        enterState(STATE_GREEN);
      }
      break;

    // --- Vehicle GREEN phase ---
    case STATE_GREEN:
      if (elapsed >= T_GREEN) {
        enterState(STATE_YELLOW);
      }
      break;

    // --- Vehicle YELLOW phase ---
    case STATE_YELLOW:
      if (elapsed >= T_YELLOW) {
        // Yellow done — back to the top of the normal cycle
        enterState(STATE_RED);
      }
      break;
  }

  // ---- 3. Small delay to avoid hammering the CPU ----
  // 10 ms is short enough not to miss button presses but
  // reduces unnecessary CPU cycles in the simulation.
  delay(10);
}
