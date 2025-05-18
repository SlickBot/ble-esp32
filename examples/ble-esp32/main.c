#include <Arduino.h>
#include <BleHandler.h>

#define BUILTIN_LED_PIN 2

inline String bytesToHexString(const uint8_t* data, const size_t len) {
    String hexString = "";
    for (size_t i = 0; i < len; ++i) {
        if (data[i] < 16) hexString += '0';  // leading zero for single-digit hex values
        hexString += String(data[i], HEX);
        hexString += ' ';  // space-separated hex values
    }
    return hexString;
}

inline String bytesToBinaryString(const uint8_t* data, const size_t len) {
    String binaryString = "";
    for (size_t i = 0; i < len; ++i) {
        for (int j = 7; j >= 0; --j) {
            binaryString += ((data[i] >> j) & 1) ? '1' : '0';
        }
        binaryString += ' ';  // space-separated binary values
    }
    return binaryString;
}

BLEHandler ble{};

void setup() {
    Serial.begin(115200);

    pinMode(BUILTIN_LED_PIN, OUTPUT);
    digitalWrite(BUILTIN_LED_PIN, LOW);

    ble.begin(
        /* deviceName  */ "BLE Device",
        /* serviceUuid */ "4ec11f8c-b68f-4f3a-a6d8-8df32f3a35e0",
        /* actualUuid  */ "f702b345-8d1b-43df-b6fa-130d77bf30bc",
        /* desiredUuid */ "bfe4d428-8d28-4427-9478-e6b649233482"
    );

    ble.onConnectCallback = [] {
        Serial.println("Device connected");
        digitalWrite(BUILTIN_LED_PIN, HIGH);
    };

    ble.onDisconnectCallback = [] {
        Serial.println("Device disconnected");
        digitalWrite(BUILTIN_LED_PIN, LOW);
        BLEDevice::startAdvertising();
    };

    // not working?
    ble.onActualCallback = [](BLECharacteristic* pCharacteristic) {
        Serial.println("---> " + bytesToHexString(pCharacteristic->getData(), pCharacteristic->getLength()));
        Serial.println("---> " + String(pCharacteristic->getValue().c_str()));
    };

    ble.onDesiredCallback = [](BLECharacteristic* pCharacteristic) {
        Serial.println("<--- " + bytesToHexString(pCharacteristic->getData(), pCharacteristic->getLength()));
        Serial.println("<--- " + String(pCharacteristic->getValue().c_str()));
    };
}

void loop() {
    delay(10);
    if (ble.isConnected()) {
        const auto message = "Hello, BLE! " + String(millis());
        ble.writeToActual((uint8_t*) message.c_str(), message.length());
    }
}
