/*
    Video: https://www.youtube.com/watch?v=oCMOYS71NIU
    Based on Neil Kolban example for IDF: https://github.com/nkolban/esp32-snippets/blob/master/cpp_utils/tests/BLE%20Tests/SampleNotify.cpp
    Ported to Arduino ESP32 by Evandro Copercini

   Create a BLE server that, once we receive a connection, will send periodic notifications.
   The service advertises itself as: 6E400001-B5A3-F393-E0A9-E50E24DCCA9E
   Has a characteristic of: 6E400002-B5A3-F393-E0A9-E50E24DCCA9E - used for receiving data with "WRITE" 
   Has a characteristic of: 6E400003-B5A3-F393-E0A9-E50E24DCCA9E - used to send data with  "NOTIFY"

   The design of creating the BLE server is:
   1. Create a BLE Server
   2. Create a BLE Service
   3. Create a BLE Characteristic on the Service
   4. Create a BLE Descriptor on the characteristic
   5. Start the service.
   6. Start advertising.

   In this example rxValue is the data received (only accessible inside that function).
   And txValue is the data to be sent, in this example just a byte incremented every second. 
*/
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>

BLEServer *pServer = NULL;
BLECharacteristic * pTxCharacteristic;
bool deviceConnected = false;
bool oldDeviceConnected = false;
uint8_t txValue = 0;

// See the following for generating UUIDs:
// https://www.uuidgenerator.net/

#define SERVICE_UUID           "6E400001-B5A3-F393-E0A9-E50E24DCCA9E" // UART service UUID
#define CHARACTERISTIC_UUID_RX "6E400002-B5A3-F393-E0A9-E50E24DCCA9E"
#define CHARACTERISTIC_UUID_TX "6E400003-B5A3-F393-E0A9-E50E24DCCA9E"


const uint8_t LASER_PINS[] = {13, 12, 27, 33, 15, 32, 14, 26};
const uint8_t NUM_LASER_PINS = 8;

unsigned long time_now = 0;
unsigned long time_laser_on = 0;
unsigned long time_laser_off = 0;

unsigned long laser_off_period = 1000;
unsigned long laser_on_period = 1000;

int running_speed = 0;
int laser_on_off = 0;

void update_period()
{
    laser_off_period = 5000 - running_speed * 450;
    laser_on_period = 5000 - running_speed * 450;
}

void laser_running() {

  update_period();

  switch(running_speed)
  {
    case 0:
      laser_off_period = 5000;
      laser_on_period = 5000;
      break;
    case 1:
      laser_off_period = 5000;
      laser_on_period = 5000;
      break;
      
  }

  if (laser_on_off == 0 && millis() > time_now + laser_off_period)
  {
    // start laser
    time_now = millis();
    time_laser_on= time_now;
    laser_on_off = 1;
    
    digitalWrite(LASER_PINS[0],HIGH);
  }

  if (laser_on_off == 1 && millis() > time_laser_on + laser_on_period)
  {
    time_now = millis();
    time_laser_off = time_now;
    laser_on_off = 0;
    for(uint8_t i = 0; i < sizeof(LASER_PINS);i++)
    {
        digitalWrite(LASER_PINS[i], LOW);
    }
  }
}

class MyServerCallbacks: public BLEServerCallbacks {
    void onConnect(BLEServer* pServer) {
      deviceConnected = true;
    };

    void onDisconnect(BLEServer* pServer) {
      deviceConnected = false;
    }
};

class MyCallbacks: public BLECharacteristicCallbacks {
    void onWrite(BLECharacteristic *pCharacteristic) {
      std::string rxValue = pCharacteristic->getValue();

      if (rxValue.length() > 0) {
        Serial.println("*********");
        Serial.print("Received Value: ");

        if (rxValue.compare("+") == 0)
        {
            if (running_speed < 10)
            {
              running_speed++;
            }
        }
        else if (rxValue.compare("-") == 0)
        {
            if (running_speed > 0)
            {
              running_speed--;
            }
        }
        
        for (int i = 0; i < rxValue.length(); i++)
          Serial.print(rxValue[i]);

        Serial.println();
        Serial.println("*********");
      }
    }
};


void setup() {
  Serial.begin(115200);

  for(uint8_t i = 0; i < sizeof(LASER_PINS);i++)
  {
      pinMode(LASER_PINS[i], OUTPUT);
      digitalWrite(LASER_PINS[i], LOW);
  }
  // Create the BLE Device
  BLEDevice::init("Laser Trainer");

  // Create the BLE Server
  pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());

  // Create the BLE Service
  BLEService *pService = pServer->createService(SERVICE_UUID);

  // Create a BLE Characteristic
  pTxCharacteristic = pService->createCharacteristic(
										CHARACTERISTIC_UUID_TX,
										BLECharacteristic::PROPERTY_NOTIFY
									);
                      
  pTxCharacteristic->addDescriptor(new BLE2902());

  BLECharacteristic * pRxCharacteristic = pService->createCharacteristic(
											 CHARACTERISTIC_UUID_RX,
											BLECharacteristic::PROPERTY_WRITE
										);

  pRxCharacteristic->setCallbacks(new MyCallbacks());

  // Start the service
  pService->start();

  // Start advertising
  pServer->getAdvertising()->start();
  Serial.println("Waiting a client connection to notify...");
}

void loop() {

    if (deviceConnected) {
//        pTxCharacteristic->setValue(&txValue, 1);
//        pTxCharacteristic->notify();
//        txValue++;
//        
		 // delay(10); // bluetooth stack will go into congestion, if too many packets are sent
	}

    // disconnecting
    if (!deviceConnected && oldDeviceConnected) {
        delay(500); // give the bluetooth stack the chance to get things ready
        pServer->startAdvertising(); // restart advertising
        Serial.println("start advertising");
        oldDeviceConnected = deviceConnected;
    }
    // connecting
    if (deviceConnected && !oldDeviceConnected) {
		// do stuff here on connecting
        oldDeviceConnected = deviceConnected;
    }

    laser_running();
}
