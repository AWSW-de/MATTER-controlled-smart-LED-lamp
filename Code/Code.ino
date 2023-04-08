// ########################################################################################################################################################################################################
// #
// # Code for the printables "MATTER controlled smart LED lamp" project: https://www.printables.com/de/model/445391-matter-controlled-smart-led-lamp
// #
// # Code by https://github.com/AWSW-de
// #
// # Released under license: GNU General Public License v3.0: https://github.com/AWSW-de/MATTER-controlled-smart-LED-lamp/blob/main/LICENSE
// #
// ########################################################################################################################################################################################################
/*
      ___           ___           ___           ___           ___           ___                    ___       ___           ___                    ___       ___           ___           ___     
     /\__\         /\  \         /\  \         /\  \         /\  \         /\  \                  /\__\     /\  \         /\  \                  /\__\     /\  \         /\__\         /\  \    
    /::|  |       /::\  \        \:\  \        \:\  \       /::\  \       /::\  \                /:/  /    /::\  \       /::\  \                /:/  /    /::\  \       /::|  |       /::\  \   
   /:|:|  |      /:/\:\  \        \:\  \        \:\  \     /:/\:\  \     /:/\:\  \              /:/  /    /:/\:\  \     /:/\:\  \              /:/  /    /:/\:\  \     /:|:|  |      /:/\:\  \  
  /:/|:|__|__   /::\~\:\  \       /::\  \       /::\  \   /::\~\:\  \   /::\~\:\  \            /:/  /    /::\~\:\  \   /:/  \:\__\            /:/  /    /::\~\:\  \   /:/|:|__|__   /::\~\:\  \ 
 /:/ |::::\__\ /:/\:\ \:\__\     /:/\:\__\     /:/\:\__\ /:/\:\ \:\__\ /:/\:\ \:\__\          /:/__/    /:/\:\ \:\__\ /:/__/ \:|__|          /:/__/    /:/\:\ \:\__\ /:/ |::::\__\ /:/\:\ \:\__\
 \/__/~~/:/  / \/__\:\/:/  /    /:/  \/__/    /:/  \/__/ \:\~\:\ \/__/ \/_|::\/:/  /          \:\  \    \:\~\:\ \/__/ \:\  \ /:/  /          \:\  \    \/__\:\/:/  / \/__/~~/:/  / \/__\:\/:/  /
       /:/  /       \::/  /    /:/  /        /:/  /       \:\ \:\__\      |:|::/  /            \:\  \    \:\ \:\__\    \:\  /:/  /            \:\  \        \::/  /        /:/  /       \::/  / 
      /:/  /        /:/  /     \/__/         \/__/         \:\ \/__/      |:|\/__/              \:\  \    \:\ \/__/     \:\/:/  /              \:\  \       /:/  /        /:/  /         \/__/  
     /:/  /        /:/  /                                   \:\__\        |:|  |                 \:\__\    \:\__\        \::/__/                \:\__\     /:/  /        /:/  /                 
     \/__/         \/__/                                     \/__/         \|__|                  \/__/     \/__/         ~~                     \/__/     \/__/         \/__/                  
*/

// Version V1.0.0

// ###########################################################################################################################################
// # Includes:
// #
// # You will need to add the following libraries to your Arduino IDE to use the project:
// # - Adafruit NeoPixel      // by Adafruit:                     https://github.com/adafruit/Adafruit_NeoPixel
// # - ESP32 Matter           // by datjan                        https://github.com/datjan/esp32-arduino-matter
// ###########################################################################################################################################
#include "Matter.h"
#include <app/server/OnboardingCodesUtil.h>
#include <Adafruit_NeoPixel.h>
using namespace chip;
using namespace chip::app::Clusters;
using namespace esp_matter;
using namespace esp_matter::endpoint;

// LED settings:
#define NUMPIXELS 64
#define PIN 33
#define Brightness 128  // of 255 --> !!! BE REALLY CAREFULL WITH THIS SETTING !!! A proper 5V/4A power supply is mandatory !!!
Adafruit_NeoPixel pixels = Adafruit_NeoPixel(NUMPIXELS, PIN, NEO_GRB + NEO_KHZ800);

// MATTER settings:
#define LogLevel ESP_LOG_INFO
const uint32_t CLUSTER_ID = OnOff::Id;                       // Cluster ID used by Matter light device
const uint32_t ATTRIBUTE_ID = OnOff::Attributes::OnOff::Id;  // Attribute ID used by Matter light device
uint16_t light_endpoint_id = 0;                              // Endpoint ref that will be assigned to Matter device
attribute_t *attribute_ref;                                  // Attribute ref that will be assigned to Matter device

// Device event handling:
static void on_device_event(const ChipDeviceEvent *event, intptr_t arg) {}
static esp_err_t on_identification(identification::callback_type_t type, uint16_t endpoint_id, uint8_t effect_id, void *priv_data) {
  return ESP_OK;
}

// Listen on attribute update requests:
static esp_err_t on_attribute_update(attribute::callback_type_t type, uint16_t endpoint_id, uint32_t cluster_id, uint32_t attribute_id, esp_matter_attr_val_t *val, void *priv_data) {
  if (type == attribute::PRE_UPDATE && endpoint_id == light_endpoint_id && cluster_id == CLUSTER_ID && attribute_id == ATTRIBUTE_ID) {
    boolean new_state = val->val.b;
    // Turn LEDs ON or OFF:
    if (new_state == true) {
      for (int i = 0; i < NUMPIXELS; i++) {
        pixels.setPixelColor(i, pixels.Color(255, 255, 255));
      }
      pixels.show();
      Serial.println("############################################################################## LAMP ON");
    } else {
      for (int i = 0; i < NUMPIXELS; i++) {
        pixels.setPixelColor(i, pixels.Color(0, 0, 0));
      }
      pixels.show();
      Serial.println("############################################################################## LAMP OFF");
    }
  }
  return ESP_OK;
}

// Setup the device:
void setup() {
  Serial.begin(115200);
  Serial.println("############################################################################## STARTUP BEGIN");
  pixels.begin();                    // Init the LEDs
  pixels.show();                     // Turn off LEDs
  pixels.setBrightness(Brightness);  // Set LED brightness
  esp_log_level_set("*", LogLevel);  // Enable MATTER debug logging

  // Setup MATTER node:
  node::config_t node_config;
  node_t *node = node::create(&node_config, on_attribute_update, on_identification);

  // Setup light endpoint / cluster / attributes and default values
  on_off_light::config_t light_config;
  light_config.on_off.on_off = false;
  light_config.on_off.lighting.start_up_on_off = false;
  endpoint_t *endpoint = on_off_light::create(node, &light_config, ENDPOINT_FLAG_NONE, NULL);

  // Save on/off attribute reference. It will be used to read attribute value later.
  attribute_ref = attribute::get(cluster::get(endpoint, CLUSTER_ID), ATTRIBUTE_ID);

  // Save generated endpoint id
  light_endpoint_id = endpoint::get_id(endpoint);

  // Start MATTER device
  esp_matter::start(on_device_event);

  // Print codes needed to setup Matter device
  PrintOnboardingCodes(chip::RendezvousInformationFlags(chip::RendezvousInformationFlag::kBLE));

  for (int y = 0; y < 4; y++) {  // Flash LEDs 3x on startup
    for (int i = 0; i < NUMPIXELS; i++) {
      pixels.setPixelColor(i, pixels.Color(255, 255, 255));
    }
    pixels.show();
    delay(1000);
    for (int i = 0; i < NUMPIXELS; i++) {
      pixels.setPixelColor(i, pixels.Color(0, 0, 0));
    }
    pixels.show();
    delay(1000);
  }
  Serial.println(" ");
  Serial.println(" ");
  Serial.println(" ");
  Serial.println(" ");
  Serial.println(" ");
  Serial.println("########################################################################################################################################################################################");
  Serial.println("############################################################################## STARTUP DONE");
  Serial.println("########################################################################################################################################################################################");
  Serial.println("############################################################################## CHECK AND USE REGISTRATION QR CODE OR AND MANUAL PAIRING CODE A FEW LINES ABOVE");
  Serial.println("########################################################################################################################################################################################");
  Serial.println("############################################################################## IF YOU HAVE CONNECTED THE LAMP ALREADY WAIT FOR IT TO CONNECT AND TO SHOW UP IN YOUR SMART HOME APP");
  Serial.println("########################################################################################################################################################################################");
  Serial.println(" ");
  Serial.println(" ");
  Serial.println(" ");
  Serial.println(" ");
  Serial.println(" ");
}

// Reads lamp on/off
esp_matter_attr_val_t get_onoff_attribute_value() {
  esp_matter_attr_val_t onoff_value = esp_matter_invalid(NULL);
  attribute::get_val(attribute_ref, &onoff_value);
  return onoff_value;
}

// Sets lamp on/off
void set_onoff_attribute_value(esp_matter_attr_val_t *onoff_value) {
  attribute::update(light_endpoint_id, CLUSTER_ID, ATTRIBUTE_ID, onoff_value);
}

void loop() {
  // NOT IN USE
}
