#pragma once

#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>
#include <BLE2902.h>

class BLEHandler {
public:
    BLEHandler() = default;

    void begin(
        const char* deviceName,
        const char* serviceUuid,
        const char* actualUuid,
        const char* desiredUuid
    ) {
        BLEDevice::init(deviceName);

        pServer = BLEDevice::createServer();
        pServer->setCallbacks(new ServerCallbacks(this));

        BLEService *pService = pServer->createService(serviceUuid);

        pActualCharacteristic = pService->createCharacteristic(actualUuid, BLECharacteristic::PROPERTY_NOTIFY);
        auto* pActualDescriptor = new BLE2902();
        pActualDescriptor->setNotifications(true);
        pActualCharacteristic->addDescriptor(pActualDescriptor);
        pActualCharacteristic->setCallbacks(new ActualCallbacks(this));

        pDesiredCharacteristic = pService->createCharacteristic(desiredUuid, BLECharacteristic::PROPERTY_WRITE);
        auto* pDesiredDescriptor = new BLE2902();
        pDesiredCharacteristic->addDescriptor(pDesiredDescriptor);
        pDesiredCharacteristic->setCallbacks(new DesiredCallbacks(this));

        pService->start();

        BLEAdvertising *pAdvertising = pServer->getAdvertising();
        pAdvertising->addServiceUUID(serviceUuid);
//        pAdvertising->setScanResponse(true);
        pAdvertising->start();
    }

    std::function<void()> onConnectCallback;
    std::function<void()> onDisconnectCallback;
    std::function<void(BLECharacteristic* pCharacteristic)> onActualCallback;
    std::function<void(BLECharacteristic* pCharacteristic)> onDesiredCallback;

    bool isConnected() const {
        return _isConnected;
    }

    void writeToActual(uint8_t* data, const size_t len) const {
        if (pActualCharacteristic) {
            pActualCharacteristic->setValue(data, len);
            pActualCharacteristic->notify();
        }
    }

private:
    BLEServer* pServer = nullptr;
    BLECharacteristic* pActualCharacteristic = nullptr;
    BLECharacteristic* pDesiredCharacteristic = nullptr;
    bool _isConnected = false;

    class ServerCallbacks final : public BLEServerCallbacks {
    public:
        explicit ServerCallbacks(BLEHandler* handler) : _handler(handler) {}
        void onConnect(BLEServer* _) override {
            _handler->_isConnected = true;
            if (_handler->onConnectCallback) {
                _handler->onConnectCallback();
            }
        }
        void onDisconnect(BLEServer* _) override {
            _handler->_isConnected = false;
            if (_handler->onDisconnectCallback) {
                _handler->onDisconnectCallback();
            }
        }
    private:
        BLEHandler* _handler;
    };

    class ActualCallbacks final : public BLECharacteristicCallbacks {
    public:
        explicit ActualCallbacks(BLEHandler* handler) : _handler(handler) {}
        // not working?
        void onWrite(BLECharacteristic* pCharacteristic) override {
            if (_handler->onActualCallback) {
                _handler->onActualCallback(pCharacteristic);
            }
        }
    private:
        BLEHandler* _handler;
    };

    class DesiredCallbacks final : public BLECharacteristicCallbacks {
    public:
        explicit DesiredCallbacks(BLEHandler* handler) : _handler(handler) {}
        void onWrite(BLECharacteristic* pCharacteristic) override {
            if (_handler->onDesiredCallback) {
                _handler->onDesiredCallback(pCharacteristic);
            }
        }
    private:
        BLEHandler* _handler;
    };

};
