// ======================================================
// Rotary Encoder using ESP32 + Interrupts + Button Input
// CLK = GPIO36 (A), DT = GPIO39 (B), BTN = GPIO34 (SW)
// Requires external 10kΩ pull-up resistors on CLK and DT
// ======================================================

// === PIN DEFINITIONS ===
#define ENCODER_CLK 36   // Encoder CLK pin (A)
#define ENCODER_DT  39   // Encoder DT pin  (B)
#define ENCODER_BTN 34   // Encoder Button pin (SW)

// === ROTARY ENCODER VARIABLES ===
volatile double encoderPosition = 0.0;        // Fine position in steps of 0.5
int positionRounded = 0;                      // Rounded integer position
int lastReportedPosition = 0;                 // Last displayed position

volatile bool encoderRotated = false;         // Flag set in ISR
volatile bool lastCLKState = LOW;             // Previous CLK state for edge detection

// === BUTTON STATE VARIABLES ===
bool currentButtonState = HIGH;               // Current state of button (1 = not pressed)
bool lastButtonState    = HIGH;               // Previous state for edge detection

// === INTERRUPT: ROTARY ENCODER HANDLER ===
void IRAM_ATTR handleEncoderInterrupt() {
  bool currentCLK = digitalRead(ENCODER_CLK);
  bool currentDT  = digitalRead(ENCODER_DT);

  // Process only on CLK state change
  if (currentCLK != lastCLKState) {
    if (currentDT != currentCLK) {
      encoderPosition += 0.5;  // Clockwise
    } else {
      encoderPosition -= 0.5;  // Counter-clockwise
    }

    encoderRotated = true;  // Flag for main loop
  }

  lastCLKState = currentCLK;
}

// === SETUP ===
void setup() {
  Serial.begin(115200);

  // Set encoder and button pins as inputs
  pinMode(ENCODER_CLK, INPUT);    // External pull-up required
  pinMode(ENCODER_DT, INPUT);     // External pull-up required
  pinMode(ENCODER_BTN, INPUT);    // External pull-up recommended

  // Initialize last CLK state
  lastCLKState = digitalRead(ENCODER_CLK);

  // Attach interrupt to CLK pin
  attachInterrupt(digitalPinToInterrupt(ENCODER_CLK), handleEncoderInterrupt, CHANGE);

  Serial.println("Rotary Encoder Initialized");
}

// === MAIN LOOP ===
void loop() {
  // === Handle Encoder Rotation ===
  if (encoderRotated) {
    positionRounded = (int)encoderPosition;

    if (positionRounded != lastReportedPosition) {
      Serial.print("Encoder Position: ");
      Serial.println(positionRounded);
      lastReportedPosition = positionRounded;
    }

    encoderRotated = false;
  }

  // === Handle Button Press (Rising Edge Detection) ===
  currentButtonState = digitalRead(ENCODER_BTN);

  if ((currentButtonState != lastButtonState) && (currentButtonState == LOW)) {
    // Button was just pressed
    encoderPosition = 0.0;
    positionRounded = 0;
    Serial.println("Encoder Reset to 0");
  }

  lastButtonState = currentButtonState;  // Save for next loop
}
