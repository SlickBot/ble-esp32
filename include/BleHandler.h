#pragma once

#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>
#include <BLE2902.h>
#include <functional>

/**
 * @brief A helper class for setting up a BLE server with two characteristics:
 *        - one for sending notifications (actual)
 *        - one for receiving writes (desired)
 *
 * The user can register callbacks for connection events and characteristic writes.
 */
class BLEHandler {
public:
    BLEHandler() = default;

    /**
     * @brief Initializes the BLE server, service, and characteristics.
     *
     * @param deviceName       The advertised BLE device name
     * @param serviceUuid      UUID of the BLE service
     * @param actualUuid       UUID for the "actual" notify characteristic
     * @param desiredUuid      UUID for the "desired" write characteristic
     */
    void begin(
        const char* deviceName,
        const char* serviceUuid,
        const char* actualUuid,
        const char* desiredUuid
    ) {
        BLEDevice::init(deviceName);

        pServer = BLEDevice::createServer();
        pServer->setCallbacks(new ServerCallbacks(this));

        BLEService* pService = pServer->createService(serviceUuid);

        // Setup notify characteristic (actual)
        pActualCharacteristic = pService->createCharacteristic(
            actualUuid, BLECharacteristic::PROPERTY_NOTIFY
        );
        auto* pActualDescriptor = new BLE2902();
        pActualDescriptor->setNotifications(true);
        pActualCharacteristic->addDescriptor(pActualDescriptor);
        pActualCharacteristic->setCallbacks(new ActualCallbacks(this));

        // Setup write characteristic (desired)
        pDesiredCharacteristic = pService->createCharacteristic(
            desiredUuid, BLECharacteristic::PROPERTY_WRITE
        );
        auto* pDesiredDescriptor = new BLE2902();
        pDesiredCharacteristic->addDescriptor(pDesiredDescriptor);
        pDesiredCharacteristic->setCallbacks(new DesiredCallbacks(this));

        pService->start();

        BLEAdvertising* pAdvertising = pServer->getAdvertising();
        pAdvertising->addServiceUUID(serviceUuid);
//        pAdvertising->setScanResponse(true);
        pAdvertising->start();
    }

    /// Callback triggered on BLE connect
    std::function<void()> onConnectCallback;

    /// Callback triggered on BLE disconnect
    std::function<void()> onDisconnectCallback;

    /// Callback when the "actual" characteristic is written (usually shouldn't happen)
    std::function<void(BLECharacteristic* pCharacteristic)> onActualCallback;

    /// Callback when the "desired" characteristic is written
    std::function<void(BLECharacteristic* pCharacteristic)> onDesiredCallback;

    /**
     * @brief Check if a BLE device is currently connected.
     * @return true if connected, false otherwise
     */
    bool isConnected() const {
        return _isConnected;
    }

    /**
     * @brief Notify the connected client with data from the "actual" characteristic.
     * @param data Pointer to data
     * @param len  Length of data
     */
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

    // Server connection state callbacks
    class ServerCallbacks final : public BLEServerCallbacks {
    public:
        explicit ServerCallbacks(BLEHandler* handler) : _handler(handler) {}
        void onConnect(BLEServer*) override {
            _handler->_isConnected = true;
            if (_handler->onConnectCallback) _handler->onConnectCallback();
        }
        void onDisconnect(BLEServer*) override {
            _handler->_isConnected = false;
            if (_handler->onDisconnectCallback) _handler->onDisconnectCallback();
        }
    private:
        BLEHandler* _handler;
    };

    // Callback for unexpected writes to "actual"
    class ActualCallbacks final : public BLECharacteristicCallbacks {
    public:
        explicit ActualCallbacks(BLEHandler* handler) : _handler(handler) {}
        void onWrite(BLECharacteristic* pCharacteristic) override {
            if (_handler->onActualCallback) _handler->onActualCallback(pCharacteristic);
        }
    private:
        BLEHandler* _handler;
    };

    // Callback for desired value writes
    class DesiredCallbacks final : public BLECharacteristicCallbacks {
    public:
        explicit DesiredCallbacks(BLEHandler* handler) : _handler(handler) {}
        void onWrite(BLECharacteristic* pCharacteristic) override {
            if (_handler->onDesiredCallback) _handler->onDesiredCallback(pCharacteristic);
        }
    private:
        BLEHandler* _handler;
    };

};
