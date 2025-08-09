// vim: ts=4:sw=4:et
#include <Arduino.h>
#include <BinaryString.h>
#include <Broadcast.h>
#include <VersionCheck.h>
#include <HTTPClient.h>
#include <Update.h>
#include <ESP32OTAPull.h>
#include <CANStream.h>
#include <OBD2Responder.h>
#include <WiFi.h>

const char* ota_version = "0.2.128";
const char* ota_url = "http://192.168.101.1:23001/emulator.json";

const char* broadcast_address = "192.168.101.255";
const uint broadcast_port = 23000;

Broadcast broadcast = Broadcast(broadcast_address, broadcast_port);

// CAN Configuration for OBD-II interface
CANConfig can_config = {
    .instance_id = 0,
    .cs_pin = 5,
    .irq_pin = 4,
    .baud_rate = 500000,
    .clock_frequency = 8000000,
    .name = "CAN0",
    .spi_sck_pin = 18,
    .spi_miso_pin = 19,
    .spi_mosi_pin = 23,
    .spi_frequency = 1000000,
    .spi_bit_order = MSBFIRST,
    .spi_mode = SPI_MODE0
};

// CAN Stream - using Broadcast as Stream* for debug output
CANStream can_stream = CANStream(can_config, &broadcast);

// OBD-II Responder - using Broadcast as Stream* for debug output
OBD2Responder obd2_responder = OBD2Responder(
    can_stream, &broadcast
);

void connectWifi() {
    Serial.print("Connecting to WiFi\n");
    WiFi.mode(WIFI_STA);
    WiFi.begin("Wifi", "Wifi");

    while (!WiFi.isConnected()) {
        Serial.print(".");
        delay(250);
    }

    Serial.print(" connected.\n");
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());
}

void checkForOtaUpdate() {
    char debug_message[128];
    memset(debug_message, 0x0, 128);
    sprintf(debug_message, "Current version: %s, checking for update\n", ota_version);
    broadcast.send(debug_message, strlen(debug_message));

    ESP32OTAPull ota;
    int ret = ota.CheckForOTAUpdate(ota_url, ota_version);
    String otaVersion = ota.GetVersion();

    memset(debug_message, 0x0, 128);
    sprintf(debug_message, "Update version: %s\n", otaVersion.c_str());
    broadcast.send(debug_message, strlen(debug_message));

    if (ret == ESP32OTAPull::UPDATE_AVAILABLE) {
        broadcast.send("Installing OTA update\n");
        broadcast.flush();
        delay(2000);

        ota.CheckForOTAUpdate(ota_url, ota_version, ESP32OTAPull::UPDATE_AND_BOOT);
    } else {
        broadcast.send("Already up-to-date.\n");
        broadcast.flush();
    }
}

unsigned long last_odb2_status = 0;

void printOdb2Status() {
    unsigned long now = millis();

    if ((last_odb2_status == 0) || ((now - last_odb2_status) >= 5000)) {
        can_stream.printStats();
        last_odb2_status = now;
    }
}

void runTests() {
    test_byte_to_bits();
    test_byte_array_to_bits();
}

void setup() {
    setCpuFrequencyMhz(160);
    Serial.begin (115200);
    runTests();
    connectWifi();
    delay(2000);
    broadcast.send("Starting up OBD-II Emulator...\n");
    
    checkForOtaUpdate();
    
    // Initialize CAN Stream
    int can_init_status = can_stream.begin();
    if (can_init_status == 1) {
        broadcast.send("CAN Stream Ready.\n");
    } else {
        char msg[50];
        snprintf(msg, 50, "Failed to initialize CAN Stream with status %i.\n", can_init_status);
        broadcast.send(msg);
    }
    
    // Initialize OBD-II Responder
    int obd2_init_status = obd2_responder.init();
    if (obd2_init_status == 1) {
        broadcast.send("OBD-II Responder Ready.\n");
    } else {
        char msg[50];
        snprintf(msg, 50, "Failed to initialize OBD-II Responder with status %i.\n", obd2_init_status);
        broadcast.send(msg);
    }
}

void loop() {
    broadcast.flush();
    if (obd2_responder.handleNextFrame() > 0) {
        can_stream.printStats();
    } else {
        // A negative status code means the frame wasn't processed
        // Since we're not doing anything else with it, clear it out
        can_stream.clearBuffer();
    }
    broadcast.flush();
}
