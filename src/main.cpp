#include <Arduino.h>
#include <Audio.h>
#define SLANG_DEBUG
#include "slang-lib.h"

#define BUFFER_SIZE 128   // MUSS 128 sein für Teensy Audio

// =====================================================
// Slang
// =====================================================
Token* tokens;
SlangInterpreter* interpreter;
SlangBufferCore* sbc;

// =====================================================
// Teensy Audio Objekte (GLOBAL!)
// =====================================================
AudioPlayQueue playQueue;
AudioOutputI2S  i2s1;

// Nur EIN Audio-Pfad
AudioConnection patchCord1(playQueue, 0, i2s1, 0);
AudioConnection patchCord2(playQueue, 0, i2s1, 1);

// =====================================================
const int LED_PIN = 13;
static int blockCount = 0;

// =====================================================
void setup() {

    pinMode(LED_PIN, OUTPUT);
    Serial.begin(115200);
    delay(500);

    Serial.println("Teensy Slang Audio startet...");

    // Audio Engine starten
    AudioMemory(80);

    // =================================================
    // Slang Setup
    // =================================================
    char* p = (char*)"x = sawtoothosc(110);";
    int length = 0;

    tokens = tokenize(p, &length);
    interpreter = createSlangInterpreter(tokens, length);

    sbc = createBufferCore(interpreter, 44100, BUFFER_SIZE);

    interpret(interpreter);
}

// =====================================================
void loop() {
    float* buf = renderBuffer(sbc);
    // Audio-Interrupt muss zuerst laufen
    // Daher erst prüfen ob Block verfügbar
    if (playQueue.available() > 0) {

        
        int16_t* audioBlock = playQueue.getBuffer();

        for (int i = 0; i < BUFFER_SIZE; i++) {

            float sample = buf[i];

            // Clipping Schutz
            if (sample > 1.0f)  sample = 1.0f;
            if (sample < -1.0f) sample = -1.0f;

            audioBlock[i] = (int16_t)(sample * 32767.0f);
        }

        playQueue.playBuffer();
    }

    // LED Debug
    blockCount++;
    if (blockCount >= 2048) {
        digitalWrite(LED_PIN, !digitalRead(LED_PIN));
        blockCount = 0;
    }
}