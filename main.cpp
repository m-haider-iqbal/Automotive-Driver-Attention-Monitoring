#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <FastLED.h>
#include "Mindwave.h"

// OLED Configuration
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// LED Configuration
#define LED_PIN 48
#define NUM_LEDS 6
#define LED_TYPE WS2812B
#define COLOR_ORDER GRB
CRGB leds[NUM_LEDS];

// Buzzer Configuration
#define BUZZER_PIN 45

// Mindwave Configuration
HardwareSerial mindwaveSerial(2); // UART2 (RX=6, TX=5)
Mindwave mindwave;

// System Constants
const unsigned long BEEP_INTERVAL = 5000;
const unsigned long FEEDBACK_INTERVAL = 100;
const unsigned long MAX_ALERT_DURATION = 30000;
const unsigned long STATE_CONFIRMATION_TIME = 1000; // 1s confirmation
const float HYSTERESIS_THRESHOLD = 0.15; // Hysteresis value

// State History Buffer
#define STATE_HISTORY_SIZE 10
struct StateHistory {
  float drowsinessScores[STATE_HISTORY_SIZE];
  float attentionScores[STATE_HISTORY_SIZE];
  float meditationScores[STATE_HISTORY_SIZE];
  uint8_t index = 0;
};

// System State Enum
enum DriverState {
  STATE_ALERT,
  STATE_DROWSY,
  STATE_ATTENTION_LOST,
  STATE_DISTRACTED
};

// Forward declarations
void displayStartupScreen();
float calculateDrowsinessScore(int attention, int meditation, int* eeg, bool blinkDetected);
void updateDriverState(float drowsinessScore);
void handleStateActions();
void setLeds(CRGB color, uint8_t brightness, bool pulse);
void triggerBuzzer(int beepCount);
void updateDisplay();
void detectBlinks();
void onMindwaveData();
void updateStateHistory(float drowsiness, float attention, float meditation);
bool confirmStateTransition(DriverState newState);

struct SystemState {
  DriverState currentState;
  DriverState previousState;
  DriverState pendingState;
  unsigned long stateStartTime;
  unsigned long stateConfirmationStart;
  float drowsinessScore;
  float attentionScore;
  float meditationScore;
  unsigned long lastBeepTime;
  unsigned long lastUpdateTime;
  unsigned long lastBlinkTime;
  bool blinkDetected;
  unsigned long blinkDuration;
  StateHistory history;
};

SystemState systemState;

void displayStartupScreen() {
  display.clearDisplay();
  
  // Title Screen
  display.setTextSize(2);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(10, 0);
  display.println("DRIVER");
  display.setCursor(10, 16);
  display.println("ATTENTION");
  display.setCursor(10, 32);
  display.println("SYSTEM");
  display.display();
  delay(1500);
  
  // Initialization message
  display.clearDisplay();
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.println("System Initializing...");
  display.display();
}

void initializeSystem() {
  // Initialize hardware
  pinMode(BUZZER_PIN, OUTPUT);
  digitalWrite(BUZZER_PIN, LOW);
  
  FastLED.addLeds<LED_TYPE, LED_PIN, COLOR_ORDER>(leds, NUM_LEDS);
  FastLED.setBrightness(60);
  
  Wire.begin(8, 9); // SDA, SCL
  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println(F("SSD1306 allocation failed"));
    while(true);
  }
  
  // Initialize state
  systemState.currentState = STATE_ALERT;
  systemState.previousState = STATE_ALERT;
  systemState.pendingState = STATE_ALERT;
  systemState.stateStartTime = millis();
  systemState.stateConfirmationStart = 0;
  systemState.drowsinessScore = 0.0;
  systemState.attentionScore = 0.0;
  systemState.meditationScore = 0.0;
  systemState.lastBeepTime = 0;
  systemState.lastUpdateTime = 0;
  systemState.lastBlinkTime = 0;
  systemState.blinkDetected = false;
  systemState.blinkDuration = 0;
  
  // Initialize history buffer
  for(int i=0; i<STATE_HISTORY_SIZE; i++) {
    systemState.history.drowsinessScores[i] = 0;
    systemState.history.attentionScores[i] = 0;
    systemState.history.meditationScores[i] = 0;
  }
  
  // Show startup screen
  displayStartupScreen();
}

void updateStateHistory(float drowsiness, float attention, float meditation) {
  systemState.history.drowsinessScores[systemState.history.index] = drowsiness;
  systemState.history.attentionScores[systemState.history.index] = attention;
  systemState.history.meditationScores[systemState.history.index] = meditation;
  
  systemState.history.index = (systemState.history.index + 1) % STATE_HISTORY_SIZE;
}

bool confirmStateTransition(DriverState newState) {
  unsigned long now = millis();
  
  if(newState != systemState.pendingState) {
    systemState.pendingState = newState;
    systemState.stateConfirmationStart = now;
    return false;
  }
  
  if(now - systemState.stateConfirmationStart > STATE_CONFIRMATION_TIME) {
    return true;
  }
  
  return false;
}

float calculateDrowsinessScore(int attention, int meditation, int* eeg, bool blinkDetected) {
  // AI Model Parameters
  const float THETA_WEIGHT = 0.3;
  const float ALPHA_WEIGHT = 0.3;
  const float BETA_PENALTY = -0.15;
  const float MEDITATION_WEIGHT = 0.25;
  const float ATTENTION_PENALTY = -0.2;
  const float BLINK_PENALTY = -0.3;
  const float HISTORY_WEIGHT = 0.2;

  // Normalize inputs
  float theta = eeg[1] / 250000.0;
  float lowAlpha = eeg[2] / 60000.0;
  float highAlpha = eeg[3] / 40000.0;
  float lowBeta = eeg[4] / 15000.0;
  float med = meditation / 100.0;
  float att = attention / 100.0;
  
  // Calculate historical averages
  float histDrowsiness = 0, histAttention = 0, histMeditation = 0;
  for(int i=0; i<STATE_HISTORY_SIZE; i++) {
    histDrowsiness += systemState.history.drowsinessScores[i];
    histAttention += systemState.history.attentionScores[i];
    histMeditation += systemState.history.meditationScores[i];
  }
  histDrowsiness /= STATE_HISTORY_SIZE;
  histAttention /= STATE_HISTORY_SIZE;
  histMeditation /= STATE_HISTORY_SIZE;
  
  // Calculate base score with historical context
  float score = THETA_WEIGHT * theta + 
                ALPHA_WEIGHT * (lowAlpha + highAlpha) +
                MEDITATION_WEIGHT * med +
                BETA_PENALTY * lowBeta +
                ATTENTION_PENALTY * att +
                HISTORY_WEIGHT * (histDrowsiness - histAttention);
                
  if (blinkDetected) {
    score += BLINK_PENALTY;
    systemState.blinkDetected = false;
  }
  
  return constrain(score, 0.0, 1.0);
}

void updateDriverState(float drowsinessScore) {
  systemState.previousState = systemState.currentState;
  DriverState newState = systemState.currentState;
  
  // State transition logic with improved hysteresis
  switch(systemState.currentState) {
    case STATE_ALERT:
      if (drowsinessScore > 0.4 + HYSTERESIS_THRESHOLD) {
        newState = STATE_DROWSY;
      }
      break;
      
    case STATE_DROWSY:
      if (drowsinessScore > 0.7 + HYSTERESIS_THRESHOLD) {
        newState = STATE_ATTENTION_LOST;
      } else if (drowsinessScore < 0.4 - HYSTERESIS_THRESHOLD) {
        newState = STATE_ALERT;
      }
      break;
      
    case STATE_ATTENTION_LOST:
      if (drowsinessScore < 0.7 - HYSTERESIS_THRESHOLD) {
        newState = STATE_DROWSY;
      }
      break;
      
    case STATE_DISTRACTED:
      // Existing logic
      break;
  }
  
  if(newState != systemState.currentState) {
    if(confirmStateTransition(newState)) {
      systemState.currentState = newState;
      systemState.stateStartTime = millis();
    }
  } else {
    systemState.pendingState = newState;
  }
}

void setLeds(CRGB color, uint8_t brightness, bool pulse) {
  static float pulsePhase = 0.0;
  
  if (pulse) {
    brightness = brightness * (0.5 + 0.5 * sin(pulsePhase));
    pulsePhase += 0.1;
    if (pulsePhase > TWO_PI) pulsePhase -= TWO_PI;
  }
  
  for (int i = 0; i < NUM_LEDS; i++) {
    leds[i] = color;
    leds[i].fadeToBlackBy(255 - brightness);
  }
  FastLED.show();
}

void triggerBuzzer(int beepCount) {
  for (int i = 0; i < beepCount; i++) {
    digitalWrite(BUZZER_PIN, HIGH);
    delay(80);
    digitalWrite(BUZZER_PIN, LOW);
    if (i < beepCount - 1) delay(120);
  }
}

void handleStateActions() {
  unsigned long now = millis();
  
  switch(systemState.currentState) {
    case STATE_ALERT:
      setLeds(CRGB::Blue, 30, false);
      break;
      
    case STATE_DROWSY:
      setLeds(CRGB::Yellow, 50, true);
      if (now - systemState.lastBeepTime > BEEP_INTERVAL * 2) {
        triggerBuzzer(1);
        systemState.lastBeepTime = now;
      }
      break;
      
    case STATE_ATTENTION_LOST:
      setLeds(CRGB::Red, 100, true);
      if (now - systemState.lastBeepTime > BEEP_INTERVAL / 2) {
        triggerBuzzer(3);
        systemState.lastBeepTime = now;
      }
      break;
      
    case STATE_DISTRACTED:
      setLeds(CRGB::Purple, 70, true);
      break;
  }
  
  // Safety timeout
  if ((systemState.currentState == STATE_DROWSY || 
       systemState.currentState == STATE_ATTENTION_LOST) &&
      (now - systemState.stateStartTime > MAX_ALERT_DURATION)) {
    systemState.currentState = STATE_ALERT;
  }
}

void updateDisplay() {
  display.clearDisplay();
  
  // State information
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.print("State: ");
  
  switch(systemState.currentState) {
    case STATE_ALERT: 
      display.print("ALERT"); 
      break;
    case STATE_DROWSY: 
      display.print("DROWSY"); 
      break;
    case STATE_ATTENTION_LOST: 
      display.print("ATTN LOST!"); 
      break;
    case STATE_DISTRACTED: 
      display.print("DISTRACTED"); 
      break;
  }
  
  // Drowsiness meter
  display.setCursor(0, 16);
  display.print("Drowsiness:");
  
  // Bar graph
  int barWidth = map(systemState.drowsinessScore * 100, 0, 100, 0, SCREEN_WIDTH - 20);
  display.fillRect(0, 26, barWidth, 10, SSD1306_WHITE);
  display.drawRect(0, 26, SCREEN_WIDTH - 20, 10, SSD1306_WHITE);
  
  // Raw values
  display.setCursor(0, 40);
  display.print("Att:");
  display.print(int(systemState.attentionScore * 100));
  display.print("% Med:");
  display.print(int(systemState.meditationScore * 100));
  display.print("%");
  
  // Warnings
  if (systemState.currentState == STATE_DROWSY) {
    display.setCursor(0, 56);
    display.print("TAKE A BREAK!");
  } else if (systemState.currentState == STATE_ATTENTION_LOST) {
    display.setCursor(0, 56);
    display.print("PULL OVER NOW!");
  }
  
  display.display();
}

void detectBlinks() {
  static unsigned long lastBlinkCheck = 0;
  unsigned long now = millis();
  
  if (now - lastBlinkCheck > 100) {
    if (random(100) < 5) {
      systemState.blinkDetected = true;
      systemState.blinkDuration = random(200, 500);
      systemState.lastBlinkTime = now;
    }
    lastBlinkCheck = now;
  }
  
  if (systemState.blinkDetected && (now - systemState.lastBlinkTime) > 300) {
    systemState.drowsinessScore += 0.1;
  }
}

void onMindwaveData() {
  int* eeg = mindwave.eeg();
  detectBlinks();
  
  unsigned long now = millis();
  if (now - systemState.lastUpdateTime < FEEDBACK_INTERVAL) return;
  
  systemState.attentionScore = mindwave.attention() / 100.0;
  systemState.meditationScore = mindwave.meditation() / 100.0;
  systemState.drowsinessScore = calculateDrowsinessScore(
    mindwave.attention(), 
    mindwave.meditation(), 
    eeg,
    systemState.blinkDetected
  );
  
  updateStateHistory(systemState.drowsinessScore, 
                   systemState.attentionScore,
                   systemState.meditationScore);
  
  updateDriverState(systemState.drowsinessScore);
  handleStateActions();
  updateDisplay();
  
  systemState.lastUpdateTime = now;
}

void setup() {
  Serial.begin(115200);
  mindwaveSerial.begin(MINDWAVE_BAUDRATE, SERIAL_8N1, 6, 5);
  
  initializeSystem();
  Serial.println("Driver Attention System Ready");
}

void loop() {
  mindwave.update(mindwaveSerial, onMindwaveData);
}
