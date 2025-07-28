/*
 Name:		PDUWifiFixRefreshWithPreferences.ino
 Created:	5/7/2025 12:59:48 PM
 Author:	ahmed ellamie
 Modified:  5/8/2025 - Added Preferences storage for settings
*/

#include <WiFi.h>
#include <WebServer.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <Preferences.h>

// Access Point credentials
const char* apSSID = "TAT_PDU_AP";
const char* apPassword = "password123";

// Basic authentication credentials
const char* http_username = "admin";
const char* http_password = "password";

// Pin definitions
const int F1_PIN = 14;     // Fuse ind CH1 (input)
const int F2_PIN = 35;     // Fuse ind CH2 (input)
const int F3_PIN = 18;     // Fuse ind CH3 (input)
const int F4_PIN = 25;     // Fuse ind CH4 (input)
const int IGN_PIN = 23;    // IGN (ON/OFF) (input)
const int VP_PIN = 36;     // Edge problem detection from middle chip (input)
const int TEMP_PIN = 21;   // Temperature sensor (OneWire, DS18B20)
const int BAT_PIN = 34;    // Battery measure (analog input)
const int CH1_PIN = 26;    // CH1 Edge Control (output)
const int CH2_PIN = 27;    // CH2 Control (output)
const int CH3_PIN = 32;    // CH3 Control (output)
const int CH4_PIN = 33;    // CH4 Control (output)
const int RELAY_PIN = 13;  // Relay Control (output)

// Preferences object for storing settings
Preferences preferences;

// OneWire setup for DS18B20
OneWire oneWire(TEMP_PIN);
DallasTemperature sensors(&oneWire);

// Global variables
float tempThreshold = 65.0;                     // Default temperature threshold (�C)
unsigned long timeThreshold = 300000;           // Default time threshold (ms)
const unsigned long CH1_PULSE = 100;            // 100ms pulse for CH1
const unsigned long CH1_OFF_TIME = 50;          // 50ms off time for CH1
const unsigned long CH_ACTIVATE_DELAY = 10000;  // 3000ms before other channels
const unsigned long RELAY_DELAY = 50;           // 50ms after relay on
// State variables
bool systemOn = false;
bool channelStates[4] = { true, true, true, true };  // CH1, CH2, CH3, CH4
bool relayState = false;                                 // Relay state
float currentTemp = 0.0;                                 // Current temperature
int vpResetAttempts = 0;                                // Counter for VP_PIN reset attempts
unsigned long lastVpResetTime = 0;                       // Time of last VP reset attempt
const int MAX_VP_RESETS = 4;                            // Maximum number of VP reset attempts
float batteryVoltage = 0.0;                              // Battery voltage
unsigned long ignLowStartTime = 0;                       // Time when IGN went LOW
bool RelayFlag = true;                                   // Relay control flag
unsigned long lastTempCheckTime = 0;                     // Last temperature check time
const unsigned long tempCheckInterval = 1000;            // 1 second
unsigned long lastStableTime = 0;                        // Last stable state change time
int lastStableState = LOW;                               // Last stable IGN state
const unsigned long debounceDelay = 150;                 // Debounce period (ms)

// Web server
WebServer server(80);
// Add static variable to track first run
static bool firstRun = true;
void on_sequance() {
    // Start with all channels off
    digitalWrite(RELAY_PIN, HIGH);  // Turn on relay
    delay(RELAY_DELAY);             // Wait 50ms

    if (firstRun) {
        // On first run, we still need CH1 edge control sequence if it's meant to be ON
        if (channelStates[0]) {  // Only if CH1 should be ON
            digitalWrite(CH1_PIN, LOW);   // Turn on CH1
            delay(CH1_PULSE);             // Wait 100ms
            digitalWrite(CH1_PIN, HIGH);  // Turn off CH1
            delay(CH1_OFF_TIME);          // Wait 50ms
            digitalWrite(CH1_PIN, LOW);   // Turn on CH1 again
        }

        delay(CH_ACTIVATE_DELAY);  // Wait 3000ms

        // Turn on other channels according to their saved states
        if (channelStates[1]) digitalWrite(CH2_PIN, LOW);
        if (channelStates[2]) digitalWrite(CH3_PIN, LOW);
        if (channelStates[3]) digitalWrite(CH4_PIN, LOW);
        
        firstRun = false;  // Mark first run as complete
        saveSettings();    // Save firstRun state
  } else {
    // Subsequent runs - check channel states
    if (channelStates[0]) {  // If CH1 should be ON
      // CH1 edge control sequence
      digitalWrite(CH1_PIN, LOW);   // Turn on CH1
      delay(CH1_PULSE);             // Wait 100ms
      digitalWrite(CH1_PIN, HIGH);  // Turn off CH1
      delay(CH1_OFF_TIME);          // Wait 50ms
      digitalWrite(CH1_PIN, LOW);   // Turn on CH1 again
      
      delay(CH_ACTIVATE_DELAY);  // Wait 5 seconds before turning on other channels
    }
    
    // Turn on only the channels that should be ON
    if (channelStates[1]) digitalWrite(CH2_PIN, LOW);
    if (channelStates[2]) digitalWrite(CH3_PIN, LOW);
    if (channelStates[3]) digitalWrite(CH4_PIN, LOW);
  }
}

void off_sequance() {
  // Turn off all channels
  digitalWrite(CH1_PIN, HIGH);
  digitalWrite(CH2_PIN, HIGH);
  digitalWrite(CH3_PIN, HIGH);
  digitalWrite(CH4_PIN, HIGH);
  digitalWrite(RELAY_PIN, LOW);  // Turn off relay
}
// Function to save settings to flash using Preferences
void saveSettings() {
  // Open the preferences namespace
  preferences.begin("pdu-settings", false);  // false = read/write mode

  // Save temperature threshold
  preferences.putFloat("tempThresh", tempThreshold);

  // Save time threshold
  preferences.putULong("timeThresh", timeThreshold);

  // Save channel states
  preferences.putBool("ch1", channelStates[0]);
  preferences.putBool("ch2", channelStates[1]);
  preferences.putBool("ch3", channelStates[2]);
  preferences.putBool("ch4", channelStates[3]);

  // Save relay flag
  preferences.putBool("relayFlag", RelayFlag);

  // Save first run state
  preferences.putBool("firstRun", firstRun);

  // Close the preferences namespace
  preferences.end();

  Serial.println("Settings saved to flash memory");
}

// Function to load settings from flash
void loadSettings() {
  // Open the preferences namespace
  preferences.begin("pdu-settings", true);  // true = read-only mode

  // Check if settings exist (using presence of tempThresh as indication)
  if (preferences.isKey("tempThresh")) {
    // Load temperature threshold
    tempThreshold = preferences.getFloat("tempThresh", 65.0);

    // Load time threshold
    timeThreshold = preferences.getULong("timeThresh", 300000);

    // Load channel states
    channelStates[0] = preferences.getBool("ch1", false);
    channelStates[1] = preferences.getBool("ch2", false);
    channelStates[2] = preferences.getBool("ch3", false);
    channelStates[3] = preferences.getBool("ch4", false);

    // Load relay flag
    RelayFlag = preferences.getBool("relayFlag", true);

    // Load first run state
    firstRun = preferences.getBool("firstRun", true);

    Serial.println("Settings loaded from flash memory");
    Serial.println("Temperature threshold: " + String(tempThreshold));
    Serial.println("Time threshold: " + String(timeThreshold / 60000.0) + " minutes");
    Serial.println("RelayFlag: " + String(RelayFlag ? "ON" : "OFF"));
    Serial.print("Channel states: ");
    for (int i = 0; i < 4; i++) {
      Serial.print(channelStates[i] ? "ON " : "OFF ");
    }
    Serial.println();
  } else {
    // First time use, save default values
    Serial.println("No saved settings found, using default values");
    preferences.end();  // Close read-only mode first
    saveSettings();     // Save default values
    return;
  }

  // Close the preferences namespace
  preferences.end();
}

// Debounce function for IGN pin
int debounce(int pin) {
  int reading = digitalRead(pin);
  if (reading != lastStableState) {
    delay(debounceDelay);
    reading = digitalRead(pin);
    if (reading != lastStableState) {
      lastStableState = reading;
      lastStableTime = millis();
      return reading;
    }
  }
  return lastStableState;
}

void handleRoot() {
  if (!server.authenticate(http_username, http_password)) {
    return server.requestAuthentication();
  }

  String html = R"=====(
<!DOCTYPE html>
<html>
<head>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <style>
    body { 
      font-family: Arial, sans-serif; 
      margin: 20px;
      background: #f0f0f0;
    }
    .grid-container {
      display: grid;
      grid-template-columns: repeat(auto-fit, minmax(300px, 1fr));
      gap: 15px;
    }
    .status-card {
      background: white;
      padding: 20px;
      border-radius: 10px;
      box-shadow: 0 2px 5px rgba(0,0,0,0.1);
    }
    .button {
      display: inline-block;
      padding: 12px 24px;
      margin: 5px;
      border: none;
      border-radius: 5px;
      font-size: 16px;
      cursor: pointer;
      transition: all 0.3s;
    }
    .btn-on { background: #4CAF50; color: white; }
    .btn-off { background: #f44336; color: white; }
    .btn-relay { background: #2196F3; color: white; }
    .button:hover { opacity: 0.8; }
    .threshold-form {
      background: white;
      padding: 20px;
      border-radius: 10px;
      margin-top: 20px;
    }
    .status-item {
      margin: 10px 0;
      padding: 10px;
      background: #f8f8f8;
      border-radius: 5px;
    }
    .status-on { color: #4CAF50; }
    .status-off { color: #f44336; }
  </style>
</head>
<body>
  <h1>PDU Control Panel</h1>
  
  <div class="grid-container">
    <!-- Status Column -->
    <div class="status-card">
      <h2>System Status</h2>
      <div id="status-data">
        <div class="status-item">
          Fuses: 
          <span id="f1-status" class="status-off">F1</span> | 
          <span id="f2-status" class="status-off">F2</span> | 
          <span id="f3-status" class="status-off">F3</span> | 
          <span id="f4-status" class="status-off">F4</span>
        </div>
        <div class="status-item">
          IGN: <span id="ign-status" class="status-off">OFF</span>
        </div>
        <div class="status-item">
          Temperature: <strong id="temp-value">0.0 C</strong> (Threshold: <span id="temp-threshold">40.0 C</span>)
        </div>
        <div class="status-item">
          Battery: <strong id="battery-value">0.00 V</strong>
        </div>
        <div class="status-item">
          Relay: <span id="relay-status" class="status-off">OFF</span>
        </div>
        <div class="status-item">
          Channels: 
          <span id="ch1-indicator" class="status-off">CH1: OFF</span> | 
          <span id="ch2-indicator" class="status-off">CH2: OFF</span> | 
          <span id="ch3-indicator" class="status-off">CH3: OFF</span> | 
          <span id="ch4-indicator" class="status-off">CH4: OFF</span>
        </div>
      </div>
    </div>

    <!-- Control Column -->
    <div class="status-card">
      <h2>Channel Controls</h2>
      <div id="channel-controls">
        <div style='margin:10px 0;'>
          <span>CH1: </span>
          <button class='button btn-on' id="ch1-on" onclick='setChannel(1, 1)'>ON</button>
          <button class='button btn-off' id="ch1-off" onclick='setChannel(1, 0)'>OFF</button>
          <span id="ch1-status"></span>
        </div>
        <div style='margin:10px 0;'>
          <span>CH2: </span>
          <button class='button btn-on' id="ch2-on" onclick='setChannel(2, 1)'>ON</button>
          <button class='button btn-off' id="ch2-off" onclick='setChannel(2, 0)'>OFF</button>
          <span id="ch2-status"></span>
        </div>
        <div style='margin:10px 0;'>
          <span>CH3: </span>
          <button class='button btn-on' id="ch3-on" onclick='setChannel(3, 1)'>ON</button>
          <button class='button btn-off' id="ch3-off" onclick='setChannel(3, 0)'>OFF</button>
          <span id="ch3-status"></span>
        </div>
        <div style='margin:10px 0;'>
          <span>CH4: </span>
          <button class='button btn-on' id="ch4-on" onclick='setChannel(4, 1)'>ON</button>
          <button class='button btn-off' id="ch4-off" onclick='setChannel(4, 0)'>OFF</button>
          <span id="ch4-status"></span>
        </div>
      </div>
      
      <h2 style="margin-top:20px;">Relay Control</h2>
      <button class='button btn-relay' onclick='setRelay(1)'>RELAY ON</button>
      <button class='button btn-relay' onclick='setRelay(0)'>RELAY OFF</button>
    </div>
  </div>

  <!-- Threshold Controls -->
  <div class="threshold-form">
    <div>
      <h3>Temperature Threshold</h3>
      <input type='number' step='0.1' id='tempThreshold' style="padding:8px;width:200px;">
      <button onclick='setTempThreshold()' class="button btn-on">Set (C)</button>
    </div>
    
    <div style="margin-top:15px;">
      <h3>Time Threshold</h3>
      <input type='number' step='0.1' id='timeThreshold' style="padding:8px;width:200px;">
      <button onclick='setTimeThreshold()' class="button btn-on">Set (minutes)</button>
    </div>
  </div>

  <script>
    // Initial data fetch
    fetchStatus();
    
    // Refresh status every 2 seconds
    setInterval(fetchStatus, 2000);
    
    // Keep track of previous values to minimize DOM updates
    let prevData = {
      f1: null, f2: null, f3: null, f4: null,
      ign: null, temp: null, tempThreshold: null,
      battery: null, relay: null, timeThreshold: null,
      ch1: null, ch2: null, ch3: null, ch4: null
    };
    
    // Function to fetch system status
    function fetchStatus() {
      fetch('/api/status')
        .then(response => response.json())
        .then(data => {
          // Update only what changed in the status display
          updateStatusItem('f1', data.f1, prevData.f1);
          updateStatusItem('f2', data.f2, prevData.f2);
          updateStatusItem('f3', data.f3, prevData.f3);
          updateStatusItem('f4', data.f4, prevData.f4);
          
          // Update IGN status
          if (data.ign !== prevData.ign) {
            const ignElement = document.getElementById('ign-status');
            if (ignElement) {
              ignElement.textContent = data.ign ? 'ON' : 'OFF';
              ignElement.className = data.ign ? 'status-on' : 'status-off';
            }
            prevData.ign = data.ign;
          }
          
          // Update temperature
          if (data.temp !== prevData.temp || data.tempThreshold !== prevData.tempThreshold) {
            const tempElement = document.getElementById('temp-value');
            const thresholdElement = document.getElementById('temp-threshold');
            if (tempElement) tempElement.textContent = data.temp.toFixed(1) + ' C';
            if (thresholdElement) thresholdElement.textContent = data.tempThreshold.toFixed(1) + ' C';
            prevData.temp = data.temp;
            prevData.tempThreshold = data.tempThreshold;
          }
          
          // Update battery
          if (data.battery !== prevData.battery) {
            const batElement = document.getElementById('battery-value');
            if (batElement) batElement.textContent = data.battery.toFixed(2) + ' V';
            prevData.battery = data.battery;
          }
          
          // Update relay
          if (data.relay !== prevData.relay) {
            const relayElement = document.getElementById('relay-status');
            if (relayElement) {
              relayElement.textContent = data.relay ? 'ON' : 'OFF';
              relayElement.className = data.relay ? 'status-on' : 'status-off';
            }
            prevData.relay = data.relay;
          }
          
          // Update channel indicators
          updateChannelStatus('ch1', data.ch1, prevData.ch1);
          updateChannelStatus('ch2', data.ch2, prevData.ch2);
          updateChannelStatus('ch3', data.ch3, prevData.ch3);
          updateChannelStatus('ch4', data.ch4, prevData.ch4);
          
          // Update form values only if they changed
          if (data.tempThreshold !== prevData.tempThreshold) {
            document.getElementById('tempThreshold').value = data.tempThreshold.toFixed(1);
            prevData.tempThreshold = data.tempThreshold;
          }
          
          if (data.timeThreshold !== prevData.timeThreshold) {
            document.getElementById('timeThreshold').value = (data.timeThreshold / 60000).toFixed(1);
            prevData.timeThreshold = data.timeThreshold;
          }
        })
        .catch(error => {
          console.error('Error fetching status:', error);
          document.getElementById('status-data').innerHTML = '<div class="status-item status-off">Error connecting to device</div>';
        });
    }
    
    // Helper function to update fuse status items
    function updateStatusItem(name, value, prevValue) {
      if (value !== prevValue) {
        const element = document.getElementById(name + '-status');
        if (element) {
          element.className = value ? 'status-on' : 'status-off';
          prevData[name] = value;
        }
      }
    }
    
    // Helper function to update channel status
    function updateChannelStatus(channel, value, prevValue) {
      if (value !== prevValue) {
        // Update channel status text in controls section
        const statusElement = document.getElementById(channel + '-status');
        if (statusElement) {
          statusElement.textContent = value ? ' (ON)' : ' (OFF)';
        }
        
        // Update channel status in status section
        const statusIndicator = document.getElementById(channel + '-indicator');
        if (statusIndicator) {
          statusIndicator.textContent = channel.toUpperCase() + ': ' + (value ? 'ON' : 'OFF');
          statusIndicator.className = value ? 'status-on' : 'status-off';
        }
        
        prevData[channel] = value;
      }
    }
    
    // Function to control channels
    function setChannel(channel, state) {
      // State is 1 for ON and 0 for OFF in the UI
      // But for the ESP32 pins, we need 0 for ON and 1 for OFF
      // So we invert the state before sending
      const invertedState = state === 1 ? 0 : 1;
      fetch(`/api/control?device=CH${channel}&state=${invertedState}`, { method: 'POST' })
        .then(response => response.json())
        .then(data => {
          if (data.success) {
            fetchStatus(); // Refresh status after successful control
          } else {
            alert('Failed to set channel');
          }
        })
        .catch(error => {
          console.error('Error setting channel:', error);
          alert('Error communicating with device');
        });
    }
    
    // Function to control relay
    function setRelay(state) {
      fetch(`/api/control?device=RELAY&state=${state}`, { method: 'POST' })
        .then(response => response.json())
        .then(data => {
          if (data.success) {
            fetchStatus(); // Refresh status after successful control
          } else {
            alert('Failed to set relay');
          }
        })
        .catch(error => {
          console.error('Error setting relay:', error);
          alert('Error communicating with device');
        });
    }
    
    // Function to set temperature threshold
    function setTempThreshold() {
      const tempValue = document.getElementById('tempThreshold').value;
      fetch(`/api/setTemp?value=${tempValue}`, { method: 'POST' })
        .then(response => response.json())
        .then(data => {
          if (data.success) {
            fetchStatus(); // Refresh status after successful setting
          } else {
            alert('Failed to set temperature threshold');
          }
        })
        .catch(error => {
          console.error('Error setting temperature threshold:', error);
          alert('Error communicating with device');
        });
    }
    
    // Function to set time threshold
    function setTimeThreshold() {
      const timeValue = document.getElementById('timeThreshold').value;
      fetch(`/api/setTime?value=${timeValue}`, { method: 'POST' })
        .then(response => response.json())
        .then(data => {
          if (data.success) {
            fetchStatus(); // Refresh status after successful setting
          } else {
            alert('Failed to set time threshold');
          }
        })
        .catch(error => {
          console.error('Error setting time threshold:', error);
          alert('Error communicating with device');
        });
    }
  </script>
</body>
</html>
)=====";

  server.send(200, "text/html", html);
}

// API endpoint to get status
void handleApiStatus() {
  if (!server.authenticate(http_username, http_password)) {
    return server.requestAuthentication();
  }

  String jsonResponse = "{";
  jsonResponse += "\"f1\":" + String(digitalRead(F1_PIN)) + ",";
  jsonResponse += "\"f2\":" + String(digitalRead(F2_PIN)) + ",";
  jsonResponse += "\"f3\":" + String(digitalRead(F3_PIN)) + ",";
  jsonResponse += "\"f4\":" + String(digitalRead(F4_PIN)) + ",";
  jsonResponse += "\"ign\":" + String(digitalRead(IGN_PIN)) + ",";
  jsonResponse += "\"temp\":" + String(currentTemp) + ",";
  jsonResponse += "\"tempThreshold\":" + String(tempThreshold) + ",";
  jsonResponse += "\"battery\":" + String(batteryVoltage) + ",";
  jsonResponse += "\"relay\":" + String(relayState) + ",";
  jsonResponse += "\"timeThreshold\":" + String(timeThreshold) + ",";
  // Add channel states to the status - invert the pin values because LOW=ON, HIGH=OFF
  jsonResponse += "\"ch1\":" + String(!digitalRead(CH1_PIN)) + ",";
  jsonResponse += "\"ch2\":" + String(!digitalRead(CH2_PIN)) + ",";
  jsonResponse += "\"ch3\":" + String(!digitalRead(CH3_PIN)) + ",";
  jsonResponse += "\"ch4\":" + String(!digitalRead(CH4_PIN));
  jsonResponse += "}";

  server.send(200, "application/json", jsonResponse);
}

// API endpoint for control
void handleApiControl() {
  if (!server.authenticate(http_username, http_password)) {
    return server.requestAuthentication();
  }

  if (server.hasArg("device") && server.hasArg("state")) {
    String device = server.arg("device");
    int state = server.arg("state").toInt();

    bool success = false;
    String message = "";

    if (state == 0 || state == 1) {
      if (device == "CH1") {
        // State 0 means ON (LOW), state 1 means OFF (HIGH)
        channelStates[0] = (state == 0);
        digitalWrite(CH1_PIN, state);
        success = true;
        message = "CH1 set to " + String(state == 0 ? "ON" : "OFF");
        saveSettings();  // Save to flash
      } else if (device == "CH2") {
        channelStates[1] = (state == 0);
        digitalWrite(CH2_PIN, state);
        success = true;
        message = "CH2 set to " + String(state == 0 ? "ON" : "OFF");
        saveSettings();  // Save to flash
      } else if (device == "CH3") {
        channelStates[2] = (state == 0);
        digitalWrite(CH3_PIN, state);
        success = true;
        message = "CH3 set to " + String(state == 0 ? "ON" : "OFF");
        saveSettings();  // Save to flash
      } else if (device == "CH4") {
        channelStates[3] = (state == 0);
        digitalWrite(CH4_PIN, state);
        success = true;
        message = "CH4 set to " + String(state == 0 ? "ON" : "OFF");
        saveSettings();  // Save to flash
      } else if (device == "RELAY") {
        RelayFlag = (state == 1);
        success = true;
        message = "RelayFlag set to " + String(RelayFlag);
        saveSettings();  // Save to flash
      } else {
        message = "Unknown device";
      }
    } else {
      message = "Invalid state. Use 0 or 1";
    }

    Serial.println(message);
    server.send(200, "application/json", "{\"success\":" + String(success ? "true" : "false") + ",\"message\":\"" + message + "\"}");
  } else {
    server.send(400, "application/json", "{\"success\":false,\"message\":\"Missing parameters\"}");
  }
}

// API endpoint to set temperature threshold
void handleApiSetTemp() {
  if (!server.authenticate(http_username, http_password)) {
    return server.requestAuthentication();
  }

  if (server.hasArg("value")) {
    tempThreshold = server.arg("value").toFloat();
    Serial.println("Temperature threshold set to " + String(tempThreshold));

    // Save to flash
    saveSettings();

    server.send(200, "application/json", "{\"success\":true,\"message\":\"Temperature threshold updated\"}");
  } else {
    server.send(400, "application/json", "{\"success\":false,\"message\":\"Missing value parameter\"}");
  }
}

// API endpoint to set time threshold
void handleApiSetTime() {
  if (!server.authenticate(http_username, http_password)) {
    return server.requestAuthentication();
  }

  if (server.hasArg("value")) {
    timeThreshold = (unsigned long)(server.arg("value").toFloat() * 60000);  // Convert minutes to milliseconds
    Serial.println("Time threshold set to " + String(timeThreshold / 60000.0) + " minutes");

    // Save to flash
    saveSettings();

    server.send(200, "application/json", "{\"success\":true,\"message\":\"Time threshold updated\"}");
  } else {
    server.send(400, "application/json", "{\"success\":false,\"message\":\"Missing value parameter\"}");
  }
}
void lastStatus();
void setup() {
  Serial.begin(9600);
  sensors.begin();

  pinMode(F1_PIN, INPUT);
  pinMode(F2_PIN, INPUT);
  pinMode(F3_PIN, INPUT);
  pinMode(F4_PIN, INPUT);
  pinMode(IGN_PIN, INPUT);
  pinMode(VP_PIN, INPUT);
  pinMode(CH1_PIN, OUTPUT);
  pinMode(CH2_PIN, OUTPUT);
  pinMode(CH3_PIN, OUTPUT);
  pinMode(CH4_PIN, OUTPUT);
  pinMode(RELAY_PIN, OUTPUT);

  // Initialize all channels to HIGH (OFF) temporarily
  digitalWrite(CH1_PIN, HIGH);
  digitalWrite(CH2_PIN, HIGH);
  digitalWrite(CH3_PIN, HIGH);
  digitalWrite(CH4_PIN, HIGH);
  digitalWrite(RELAY_PIN, LOW);
  /*
    delay(2000);
    digitalWrite(CH1_PIN, HIGH);
    digitalWrite(CH2_PIN, HIGH);
    digitalWrite(CH3_PIN, HIGH);
    digitalWrite(CH4_PIN, HIGH);
    digitalWrite(RELAY_PIN, LOW);
    delay(2000);
    digitalWrite(CH1_PIN, LOW);
    digitalWrite(CH2_PIN, LOW);
    digitalWrite(CH3_PIN, LOW);
    digitalWrite(CH4_PIN, LOW);
    digitalWrite(RELAY_PIN, HIGH);
    */
  // Load settings from flash
  loadSettings();

  // Start Access Point
  WiFi.softAP(apSSID, apPassword);
  IPAddress IP = WiFi.softAPIP();
  Serial.print("AP IP address: ");
  Serial.println(IP);

  // Set up server routes
  server.on("/", handleRoot);

  // API Routes
  server.on("/api/status", HTTP_GET, handleApiStatus);
  server.on("/api/control", HTTP_POST, handleApiControl);
  server.on("/api/setTemp", HTTP_POST, handleApiSetTemp);
  server.on("/api/setTime", HTTP_POST, handleApiSetTime);

  server.begin();
  Serial.println("HTTP server started");
}

void loop() {
  server.handleClient();

  // Read and debounce IGN state
  int ignState = debounce(IGN_PIN);
  
  // Debug: Print VP_PIN and CH1_PIN states every loop
  static int lastVpPinState = -1;
  static int lastCh1PinState = -1;
  int currentVpPin = digitalRead(VP_PIN);
  int currentCh1Pin = digitalRead(CH1_PIN);
  
  // Only print if states have changed
  if (currentVpPin != lastVpPinState || currentCh1Pin != lastCh1PinState) {
    Serial.println("Status Update:");
    Serial.println("VP_PIN State: " + String(currentVpPin ? "HIGH (Fault)" : "LOW (Normal)"));
    Serial.println("CH1_PIN State: " + String(currentCh1Pin ? "HIGH (OFF)" : "LOW (ON)"));
    Serial.println("Current Reset Attempts: " + String(vpResetAttempts));
    lastVpPinState = currentVpPin;
    lastCh1PinState = currentCh1Pin;
  }
  
  // VP_PIN fault handling for CH1 only
  if (!digitalRead(CH1_PIN)) {  // If CH1 is ON (LOW)
    if (digitalRead(VP_PIN) == HIGH) {  // Fault detected
      static bool faultHandlingInProgress = false;
      
      if (!faultHandlingInProgress && vpResetAttempts < MAX_VP_RESETS) {
        faultHandlingInProgress = true;
        Serial.println("FAULT DETECTED: VP_PIN is HIGH while CH1 is ON");
        
        // Immediately turn CH1 OFF
        digitalWrite(CH1_PIN, HIGH);
        Serial.println("CH1 turned OFF, waiting 30 seconds before reset...");
        
        // Wait 30 seconds
        delay(30000);
        
        // Turn CH1 back ON
        digitalWrite(CH1_PIN, LOW);
        Serial.println("CH1 turned back ON");
        
        // Increment reset counter
        vpResetAttempts++;
        Serial.println("Reset attempt " + String(vpResetAttempts) + " of " + String(MAX_VP_RESETS));
        
        faultHandlingInProgress = false;
      }
      
      // If we've reached max attempts, permanently disable CH1
      if (vpResetAttempts >= MAX_VP_RESETS) {
        Serial.println("CRITICAL: Maximum reset attempts reached!");
        // Permanent shutdown of CH1
        digitalWrite(CH1_PIN, HIGH);
        channelStates[0] = false;  // Update channel state
        saveSettings();  // Save to flash
        Serial.println("VP fault: Maximum reset attempts reached. CH1 locked and saved to flash memory.");
        Serial.println("Manual intervention required to reset CH1");
      }
    } else {
      // If CH1 is ON but VP_PIN is LOW (normal operation)
      static unsigned long lastNormalOpTime = 0;
      if (millis() - lastNormalOpTime >= 50000) {  // Print every 5 seconds during normal operation
        Serial.println("Normal Operation: CH1 is ON, VP_PIN is LOW (no fault)");
        lastNormalOpTime = millis();
      }
    }
  }

  // Update temperature and battery voltage every second
  if (millis() - lastTempCheckTime >= tempCheckInterval) {
    sensors.requestTemperatures();
    currentTemp = sensors.getTempCByIndex(0);

    // Read battery voltage (adjust scaling based on your voltage divider)
    int batValue = analogRead(BAT_PIN);
    batteryVoltage = (batValue * 3.3 / 1024.0) * 3.3;  // Example for 12V system

    lastTempCheckTime = millis();
  }

  // Relay control logic
  if (RelayFlag) {
    bool tempCondition = currentTemp >= tempThreshold;
    bool timeCondition = (ignState == LOW) && (millis() - lastStableTime >= timeThreshold);

    if (tempCondition || timeCondition) {
      if (relayState) {
        off_sequance();
      }
      relayState = false;
    } else if (ignState == HIGH) {  // Check specifically for HIGH state
      // Turn on if IGN is HIGH and conditions are good
      if (!relayState) {
        Serial.println("IGN is HIGH, running on_sequance");
        on_sequance();
      }
      relayState = true;
    }
  } else {
    if (relayState) {
      off_sequance();
    }
    relayState = false;
  }

  // Handle IGN state changes
  static int lastIgnState = HIGH;
  if (ignState != lastIgnState) {
    Serial.println("IGN state changed to: " + String(ignState ? "HIGH" : "LOW"));
    lastIgnState = ignState;

    if (ignState == LOW) {
      ignLowStartTime = millis();
    }
  }
  // Check for incoming serial commands
  if (Serial.available() > 0) {
    String command = Serial.readStringUntil('\n');
    command.trim();

    // Handle GET commands
    if (command == "GET_F1") {
      Serial.print("F1:");
      Serial.println(digitalRead(F1_PIN));
    } else if (command == "GET_F2") {
      Serial.print("F2:");
      Serial.println(digitalRead(F2_PIN));
    } else if (command == "GET_F3") {
      Serial.print("F3:");
      Serial.println(digitalRead(F3_PIN));
    } else if (command == "GET_F4") {
      Serial.print("F4:");
      Serial.println(digitalRead(F4_PIN));
    } else if (command == "GET_IGN") {
      Serial.print("IGN:");
      Serial.println(digitalRead(IGN_PIN));
    } else if (command == "GET_TEMP") {
      Serial.print("TEMP:");
      Serial.println(currentTemp, 2);
    } else if (command == "GET_BAT") {
      Serial.print("BAT:");
      Serial.println(batteryVoltage, 2);
    } else if (command == "GET_CH1") {
      Serial.print("CH1:");
      Serial.println(channelStates[0] ? "1" : "0");
    } else if (command == "GET_CH2") {
      Serial.print("CH2:");
      Serial.println(channelStates[1] ? "1" : "0");
    } else if (command == "GET_CH3") {
      Serial.print("CH3:");
      Serial.println(channelStates[2] ? "1" : "0");
    } else if (command == "GET_CH4") {
      Serial.print("CH4:");
      Serial.println(channelStates[3] ? "1" : "0");
    } else if (command == "GET_RELAY") {
      Serial.print("RELAY:");
      Serial.println(RelayFlag ? "1" : "0");
    } else if (command == "GET_STATUS") {
      lastStatus();
    }
    // Handle SET commands
    else {
      int spaceIndex = command.indexOf(' ');
      if (spaceIndex > 0) {
        String cmd = command.substring(0, spaceIndex);
        String valueStr = command.substring(spaceIndex + 1);
        int value = valueStr.toInt();

        if (cmd == "SET_CH1") {
          channelStates[0] = (value == 0);
          digitalWrite(CH1_PIN, value);
          saveSettings();
          Serial.print("CH1:");
          Serial.println(channelStates[0] ? "1" : "0");
        } else if (cmd == "SET_CH2") {
          channelStates[1] = (value == 0);
          digitalWrite(CH2_PIN, value);
          saveSettings();
          Serial.print("CH2:");
          Serial.println(channelStates[1] ? "1" : "0");
        } else if (cmd == "SET_CH3") {
          channelStates[2] = (value == 0);
          digitalWrite(CH3_PIN, value);
          saveSettings();
          Serial.print("CH3:");
          Serial.println(channelStates[2] ? "1" : "0");
        } else if (cmd == "SET_CH4") {
          channelStates[3] = (value == 0);
          digitalWrite(CH4_PIN, value);
          saveSettings();
          Serial.print("CH4:");
          Serial.println(channelStates[3] ? "1" : "0");
        } else if (cmd == "SET_RELAY") {
          RelayFlag = (value == 1);
          saveSettings();
          Serial.print("RELAY:");
          Serial.println(RelayFlag ? "1" : "0");
        } else if (cmd == "SET_TSTemp") {
          tempThreshold = value;
          saveSettings();
          Serial.print("TSTEMP:");
          Serial.println(tempThreshold);
        } else if (cmd == "SET_TSTime") {
          timeThreshold = value * 60000;  // Convert minutes to ms
          saveSettings();
          Serial.print("TSTOFF:");
          Serial.println(value);
        } else {
          Serial.println("Unknown command");
        }
      } else {
        Serial.println("Invalid command format");
      }
    }
  }
}
// Function to output all status parameters via Serial
void lastStatus() {
  Serial.println("System Status:");
  Serial.print("F1:");
  Serial.println(digitalRead(F1_PIN));
  Serial.print("F2:");
  Serial.println(digitalRead(F2_PIN));
  Serial.print("F3:");
  Serial.println(digitalRead(F3_PIN));
  Serial.print("F4:");
  Serial.println(digitalRead(F4_PIN));
  Serial.print("IGN:");
  Serial.println(digitalRead(IGN_PIN));
  Serial.print("TEMP:");
  Serial.println(currentTemp, 2);
  Serial.print("BAT:");
  Serial.println(batteryVoltage, 2);
  Serial.print("CH1:");
  Serial.println(channelStates[0] ? "1" : "0");
  Serial.print("CH2:");
  Serial.println(channelStates[1] ? "1" : "0");
  Serial.print("CH3:");
  Serial.println(channelStates[2] ? "1" : "0");
  Serial.print("CH4:");
  Serial.println(channelStates[3] ? "1" : "0");
  Serial.print("RELAY:");
  Serial.println(RelayFlag ? "1" : "0");
  Serial.print("TEMP_THRESH:");
  Serial.println(tempThreshold);
  Serial.print("TIME_THRESH_MIN:");
  Serial.println(timeThreshold / 60000.0);
}