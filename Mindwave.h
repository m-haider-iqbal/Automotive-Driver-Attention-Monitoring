#ifndef Mindwave_h
#define Mindwave_h

#include <Arduino.h>

#define MINDWAVE_BAUDRATE               57600
#define MINDWAVE_WAIT_FOR_FIRST_A        0
#define MINDWAVE_WAIT_FOR_SECOND_A       1
#define MINDWAVE_WAIT_FOR_PAYLOAD_LENGTH 2
#define MINDWAVE_WAIT_FOR_PAYLOAD        3
#define MINDWAVE_WAIT_FOR_CHECKSUM       4
#define MINDWAVE_EEG_SIZE                8
#define MINDWAVE_PACKET_MAX_SIZE        173
#define MINDWAVE_PAYLOAD_MAX_SIZE       169

typedef void (*MindwaveDataCallback) ();

class Mindwave {
  public:
    Mindwave();

    void update(Stream &stream, MindwaveDataCallback onData);
    int attention();
    int meditation();
    int quality();
    long lastUpdate();
    int* eeg();
    int delta();
    int theta();
    int lowAlpha();
    int highAlpha();
    int lowBeta();
    int highBeta();
    int lowGamma();
    int midGamma();
    int blinkStrength();

  private:
    byte _generatedChecksum = 0;
    byte _checksum = 0;
    int _payloadLength = 0;
    int _payloadIndex = 0;
    byte _payloadData[MINDWAVE_PACKET_MAX_SIZE] = {0};

    byte _poorQuality = 0;
    byte _attention = 0;
    byte _meditation = 0;
    byte _blinkStrength = 0;
    int _eeg[MINDWAVE_EEG_SIZE] = {0};

    long _lastReceivedPacket = 0, _lastUpdate = 0;
    bool _bigPacket = false;
    int _state = MINDWAVE_WAIT_FOR_FIRST_A;

    void parsePayload(MindwaveDataCallback onData);
};

#endif
