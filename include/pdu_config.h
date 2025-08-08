/*
 * PDU Configuration Header
 * 
 * This header contains all configuration constants and pin definitions
 * for the PDU system. It centralizes all configurable parameters in
 * one location for easy maintenance.
 * 
 * Author: Ahmed Ellamie
 * Email: ahmed.ellamiee@gmail.com
 * Created: 5/7/2025
 * Modified: 7/29/2025
 */

#ifndef PDU_CONFIG_H
#define PDU_CONFIG_H

// Network Configuration
#define AP_SSID "TAT_PDU_AP"
#define AP_PASSWORD "password123"
#define HTTP_USERNAME "admin"
#define HTTP_PASSWORD "password"

// Pin Definitions
#define F1_PIN 14     // Fuse ind CH1 (input)
#define F2_PIN 35     // Fuse ind CH2 (input)
#define F3_PIN 18     // Fuse ind CH3 (input)
#define F4_PIN 25     // Fuse ind CH4 (input)
#define IGN_PIN 23    // IGN (ON/OFF) (input)
#define VP_PIN 36     // Edge problem detection from middle chip (input)
#define TEMP_PIN 21   // Temperature sensor (OneWire, DS18B20)
#define BAT_PIN 34    // Battery measure (analog input)
#define CH1_PIN 26    // CH1 Edge Control (output)
#define CH2_PIN 27    // CH2 Control (output)
#define CH3_PIN 32    // CH3 Control (output)
#define CH4_PIN 33    // CH4 Control (output)
#define RELAY_PIN 13  // Relay Control (output)

// Timing Constants
#define CH1_PULSE 100           // 100ms pulse for CH1
#define CH1_OFF_TIME 50         // 50ms off time for CH1
#define CH_ACTIVATE_DELAY 10000 // 10s before other channels
#define RELAY_DELAY 50          // 50ms after relay on
#define TEMP_CHECK_INTERVAL 1000 // Temperature check interval (1 second)
#define DEBOUNCE_DELAY 150      // Debounce period (ms)
#define MAX_VP_RESETS 3        // Maximum number of VP reset attempts

// Default Values
#define DEFAULT_TEMP_THRESHOLD 65.0f  // Default temperature threshold (°C)
#define DEFAULT_TIME_THRESHOLD 300000  // Default time threshold (ms)

#endif // PDU_CONFIG_H
