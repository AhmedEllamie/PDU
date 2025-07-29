/*
 * HTML Content Header
 * 
 * This header contains the HTML, CSS, and JavaScript code for the
 * web interface of the PDU system. It provides a responsive and
 * interactive control panel for PDU management.
 * 
 * Author: Ahmed Ellamie
 * Email: ahmed.ellamiee@gmail.com
 * Created: 5/7/2025
 * Modified: 7/29/2025
 */

#ifndef HTML_CONTENT_H
#define HTML_CONTENT_H

const char INDEX_HTML[] PROGMEM = R"=====(
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

#endif // HTML_CONTENT_H
