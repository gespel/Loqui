#include <Arduino.h>
#include <Audio.h>
#define SLANG_DEBUG
#include "slang-lib.h"

#define BUFFER_SIZE 128
#define SAMPLE_RATE AUDIO_SAMPLE_RATE_EXACT

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

void initBlink() {
    pinMode(LED_PIN, OUTPUT);
    for (int i = 0; i < 3; i++) {
        digitalWrite(LED_PIN, HIGH);
        delay(200);
        digitalWrite(LED_PIN, LOW);
        delay(200);
    }
}

void runtimeDebug() {
    int triggerLimit = 128; // Anzahl der Blöcke, nach denen die Debug-Info ausgegeben wird
    blockCount++;
    if (blockCount >= triggerLimit) {
        Serial.print("[Loqui] ");
        Serial.print(triggerLimit);
        Serial.print(" blocks rendered -> ");
        Serial.print(triggerLimit * BUFFER_SIZE);
        Serial.print(" samples (");
        Serial.print((triggerLimit * BUFFER_SIZE) / SAMPLE_RATE);
        Serial.println(" seconds)");
        //Serial.print("[Loqui] Buffer[0]: ");
        //Serial.println(buf[0]);
        digitalWrite(LED_PIN, !digitalRead(LED_PIN));
        blockCount = 0;
    }
}

void setup() {
    pinMode(LED_PIN, OUTPUT);
    Serial.begin(115200);

    Serial.println("Teensy Slang Audio startet...");

    // Audio Engine starten
    AudioMemory(80);

    // =================================================
    // Slang Setup
    // =================================================
    char* p = (char*)"ss = stepsequencer([1, 1.2, 1.33333, 1.5,  1.6, 1.5, 1.3333, 1.6], 240); lin = linenvelope(ss, 0.001, 0.05, 0.001, 0.25); c = sawtoothosc(ss * 110); e = sawtoothosc(ss * 55); f = sawtoothosc(ss * 110.2); g = sawtoothosc(ss * 220); lowpassfilter(MAIN, 1500 * lin); springreverb(0.05, 0.8, 0.4, 0.5);";
    int length = 0;

    tokens = tokenize(p, &length);
    interpreter = createSlangInterpreter(tokens, length);

    sbc = createBufferCore(interpreter, AUDIO_SAMPLE_RATE_EXACT, BUFFER_SIZE);

    interpret(interpreter);
    
    Serial.print("Slang BufferCore initialisiert mit Sample-Rate: ");
    Serial.println(AUDIO_SAMPLE_RATE_EXACT);

    initBlink();
}

// =====================================================
void loop() {
    if (playQueue.available() >= 1) {       
        float* buf = renderBuffer(sbc);
        int16_t* audioBlock = playQueue.getBuffer();

        for (int i = 0; i < AUDIO_BLOCK_SAMPLES; i++) {
            float sample = buf[i];

            //if (sample > 1.0f)  sample = 1.0f;
            //if (sample < -1.0f) sample = -1.0f;

            audioBlock[i] = (int16_t)(sample * 16384.0f);
        }

        playQueue.playBuffer();
        runtimeDebug();
    }
}