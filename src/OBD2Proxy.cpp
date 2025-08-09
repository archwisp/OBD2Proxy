// vim: ts=4:sw=4:et
#include <Arduino.h>
#include <BinaryString.h>
#include <Broadcast.h>
#include <VersionCheck.h>
#include <HTTPClient.h>
#include <Update.h>
#include <ESP32OTAPull.h>
#include <CANProxy.h>
#include <DebugWebserver.h>

const bool wifi_enabled = true;
const char* ota_version = "0.2.97";
const char* ota_url = "http://192.168.101.1:23001/proxy.json";

const char* broadcast_address = "192.168.101.255";
const uint broadcast_port = 23000;
Broadcast debug = Broadcast(broadcast_address, broadcast_port);

const uint webserver_port = 23002;
DebugWebserver webserver = DebugWebserver(webserver_port);

// CAN Configuration
// CAN1: Scanner interface (CS=GPIO5, IRQ=GPIO4)
// CAN2: ECU interface (CS=GPIO14, IRQ=GPIO13)
CANConfig can1_config = {
    .instance_id = 0,
    .cs_pin = 5,
    .irq_pin = 4,
    .baud_rate = 500000,
    .clock_frequency = 8000000,
    .name = "CAN1",
    .spi_sck_pin = 18,
    .spi_miso_pin = 19,
    .spi_mosi_pin = 23,
    .spi_frequency = 1000000,
    .spi_bit_order = MSBFIRST,
    .spi_mode = SPI_MODE0
};

CANConfig can2_config = {
    .instance_id = 1,
    .cs_pin = 14,
    .irq_pin = 13,
    .baud_rate = 500000,
    .clock_frequency = 8000000,
    .name = "CAN2",
    .spi_sck_pin = 18,
    .spi_miso_pin = 19,
    .spi_mosi_pin = 23,
    .spi_frequency = 1000000,
    .spi_bit_order = MSBFIRST,
    .spi_mode = SPI_MODE0
};

// CAN Proxy - using Broadcast as Stream* for debug output
CANProxy can_proxy = CANProxy(can1_config, can2_config, &debug);

// System state flags
bool can_proxy_initialized = false;
bool wifi_connected = false;
unsigned long last_error_report = 0;
const unsigned long ERROR_REPORT_INTERVAL = 30000; // 30 seconds

void connectWifi() {
    Serial.print("Connecting to WiFi\n");
    WiFi.mode(WIFI_STA); // Both AP and Station mode
    // WiFi.mode(WIFI_AP_STA); // Both AP and Station mode
    WiFi.begin("Wifi", "Wifi");

    int attempts = 0;
    const int max_attempts = 20; // 5 seconds max
    
    while (!WiFi.isConnected() && attempts < max_attempts) {
        Serial.print(".");
        delay(250);
        attempts++;
    }

    if (WiFi.isConnected()) {
        Serial.print(" connected.\n");
        Serial.print("IP address: ");
        Serial.println(WiFi.localIP());
        wifi_connected = true;
    } else {
        Serial.println(" WiFi connection failed!");
        wifi_connected = false;
    }
}

void checkForOtaUpdate() {
    if (!wifi_connected) {
        return; // Skip OTA if WiFi not connected
    }
    
    debug.printf("Current version: %s, checking for update\n", ota_version);

    ESP32OTAPull ota;
    int ret = ota.CheckForOTAUpdate(ota_url, ota_version);
    String otaVersion = ota.GetVersion();

    debug.printf("Update version: %s\n", otaVersion.c_str());

    if (ret == ESP32OTAPull::UPDATE_AVAILABLE) {
        debug.print("Installing OTA update\n");
        debug.flush();
        delay(100);

        ota.CheckForOTAUpdate(ota_url, ota_version, ESP32OTAPull::UPDATE_AND_BOOT);
    } else {
        debug.print("Already up-to-date.\n");
        debug.flush();
    }
}

unsigned long last_status_print = 0;
unsigned long status_print_interval = 10000; // 10 seconds

void printStatus() {
    unsigned long now = millis();

    if ((last_status_print == 0) || ((now - last_status_print) >= status_print_interval)) {
        debug.print("=== OBD-II CAN Proxy Status ===\n");
        
        if (can_proxy_initialized) {
            // Only call these methods if CAN proxy is initialized
            can_proxy.printStats();
        } else {
            debug.print("CAN Proxy: NOT INITIALIZED\n");
        }
        
        debug.print("WiFi: ");
        debug.print(wifi_connected ? "CONNECTED" : "DISCONNECTED");
        debug.print("\n");
        
        debug.print("===============================\n");
        last_status_print = now;
    }
}

void reportError(const char* error_msg) {
    unsigned long now = millis();
    
    // Limit error reporting frequency to avoid spam
    if ((last_error_report == 0) || ((now - last_error_report) >= ERROR_REPORT_INTERVAL)) {
        debug.printf("ERROR: %s (uptime: %lu ms)\n", error_msg, now);
        debug.flush();
        last_error_report = now;
    }
}

void runTests() {
    test_byte_to_bits();
    test_byte_array_to_bits();
}

void setup() {
    setCpuFrequencyMhz(160);
    // Serial2.begin(115200, SERIAL_8N1, 16, 17);
    Serial2.begin(115200);
    
    // Wait for serial with timeout to prevent hanging
    unsigned long start_time = millis();
    while (!Serial && (millis() - start_time) < 3000) {
        delay(10);
    }
    
    runTests();
    
    // Initialize broadcast first for error reporting
    Serial2.print("Starting up OBD-II CAN Proxy...\n");
  
    if (wifi_enabled) {
        // Connect to WiFi
        connectWifi();
       
        // Wait for wifi connection to complete
        start_time = millis();
        while (!wifi_connected && (millis() - start_time) < 3000) {
            delay(10);
        }

        // Check for OTA updates
        checkForOtaUpdate();

        if (wifi_connected) {
            debug.print("\nStarting up OBD-II CAN Proxy...\n");
            if (!webserver.start()) {
                debug.println("Failed to start webserver");
            } else {
                debug.print("Webserver started on port ");
                debug.println(webserver_port);
                webserver.println("Hello");
                webserver.flush();
            }
        } else {
            Serial2.println("Skipping webserver startup - WiFi failed to connect");
        }
    } else {
        Serial2.println("WiFi disabled");
    }
    
    // Initialize CAN Proxy
    int can_proxy_status = can_proxy.begin();
    
    if (can_proxy_status == 1) {
        can_proxy_initialized = true;
        can_proxy.activateOBD2Responder(34); // GPIO34 for OBD2 responder enable/disable
        debug.print("CAN Proxy initialized successfully.\n");
    } else {
        char error_msg[100];
        snprintf(error_msg, sizeof(error_msg), 
                "Failed to initialize CAN Proxy with status %i.\n", can_proxy_status);
        reportError(error_msg);
        can_proxy_initialized = false;
    }
    
    debug.print("Setup complete.\n");
}

void loop() {
    // Handle CAN proxy frames (forwarding between CAN1 and CAN2)
    if (can_proxy_initialized) {
        can_proxy.handleFrames();
    }
    
    // Flush debug buffer periodically (not every loop)
    static unsigned long last_flush = 0;
    if (millis() - last_flush > 1000) { // Flush every second
        debug.flush();
        last_flush = millis();
    }
    
    // Small delay to prevent overwhelming the system
    delay(1);
} 
