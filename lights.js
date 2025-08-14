//todo: handle: lights.js:66 Uncaught (in promise) DOMException: GATT operation failed for unknown reason.

// Initialize variables
window.charac = false;
window.notificationsActive = false;
window.queued = false;
window.processing = false;

// Define data configuration with all supported keys
window.myData = {
  lightMode: 'swipe',
  colorOne: '255,0,203',
  colorTwo: '255,0,0',
  colorThree: '0,0,255',
  loopInterval: 100,
  brightness: 40,
  ledCount: 50,
  pixelPin: 15,
  maxCurrent: 8000
};

window.getRandomColor = () => {
  const r = Math.floor(Math.random() * 256);
  const g = Math.floor(Math.random() * 256);
  const b = Math.floor(Math.random() * 256);
  return `${r},${g},${b}`;
}

// Local storage functions
window.saveSettings = () => {
  localStorage.setItem('ledControllerSettings', JSON.stringify(window.myData));
  console.log('Settings saved to local storage');
};

window.loadSettings = () => {
  const saved = localStorage.getItem('ledControllerSettings');
  if (saved) {
    try {
      const settings = JSON.parse(saved);
      window.myData = { ...window.myData, ...settings };
      console.log('Settings loaded from local storage:', window.myData);
      return true;
    } catch (e) {
      console.error('Failed to parse saved settings:', e);
    }
  }
  return false;
};

window.updateUIFromData = () => {
  // Update color pickers
  if (window.myData.colorOne) updateColorPicker('colorOne', window.myData.colorOne);
  if (window.myData.colorTwo) updateColorPicker('colorTwo', window.myData.colorTwo);
  if (window.myData.colorThree) updateColorPicker('colorThree', window.myData.colorThree);
  
  // Update number inputs with correct ID mapping
  const inputMappings = {
    'loopInterval': 'loopInterval',
    'brightness': 'brightness',
    'ledCount': 'ledCount',
    'pixelPin': 'pixelPin',
    'maxCurrent': 'maxCurrent'
  };
  
  Object.entries(inputMappings).forEach(([elementId, dataKey]) => {
    const element = document.getElementById(elementId);
    if (element && window.myData[dataKey] !== undefined) {
      element.value = window.myData[dataKey];
    }
  });
};

// Asynchronous function for Bluetooth write
window.bleWrite = async () => {
  if (window.processing) {
    window.queued = true;
  } else {
    window.queued = false;
    window.processing = true;
    // Save settings to local storage before sending
    window.saveSettings();
    // json stringify and encode
    const encoded = new TextEncoder().encode(JSON.stringify(window.myData));
    console.log(`Ble Write`, window.myData, { encoded });
    await window.charac.writeValue(encoded).then(() => {
      window.processing = false;
      window.queued && window.bleWrite();
    }).catch(error => {
      console.error('Error during BLE write:', error);
      window.processing = false;
    });
  }
};

// Asynchronous function for Bluetooth read
window.bleRead = async ({ target: { value } }) => {
  let message = new TextDecoder().decode(await value);
  try {
    const statusData = JSON.parse(message);
    console.log('Status from BLE:', statusData);
    
    // Update UI elements if they exist
    if (statusData.mode) {
      const modeSelect = document.getElementById('lightMode');
      if (modeSelect) modeSelect.value = statusData.mode;
    }
    if (statusData.brightness !== undefined) {
      const brightnessInput = document.getElementById('brightness');
      if (brightnessInput) brightnessInput.value = statusData.brightness;
    }
    if (statusData.loopInterval !== undefined) {
      const delayInput = document.getElementById('loopInterval');
      if (delayInput) delayInput.value = statusData.loopInterval;
    }
    
    // Update color pickers if they exist
    if (statusData.colorOne) updateColorPicker('colorOne', statusData.colorOne);
    if (statusData.colorTwo) updateColorPicker('colorTwo', statusData.colorTwo);
    if (statusData.colorThree) updateColorPicker('colorThree', statusData.colorThree);
    
    // Log system info
    if (statusData.temperature) console.log('ESP32 Temperature:', statusData.temperature);
    if (statusData.firmware) console.log('Firmware:', statusData.firmware);
    if (statusData.freeHeap) console.log('Free Heap:', statusData.freeHeap);
    
  } catch (e) {
    console.log('Non-JSON message from BLE:', message);
  }
};

// Helper function to update color picker from RGB string
function updateColorPicker(elementId, rgbString) {
  const [r, g, b] = rgbString.split(',').map(Number);
  const hex = '#' + ((1 << 24) + (r << 16) + (g << 8) + b).toString(16).slice(1);
  const element = document.getElementById(elementId);
  if (element) element.value = hex;
}

// Function to set up Bluetooth connection
window.ble = async () => {
  const dashboard = document.getElementById('btDash');
  const connectCard = document.getElementById('connectCard');
  const statusIndicator = document.getElementById('statusIndicator');
  
  console.log('BLE connection starting...', { dashboard, connectCard, statusIndicator });
  
  const SERVICE_UUID = '4fafc201-1fb5-459e-8fcc-c5c9c3319123';
  const CHARACTERISTIC_UUID = 'beb5483e-36e1-4688-b7f5-ea07361b2623';
  const ESPNAME = 'Music Strip';

  if (window.charac && window.charac.service && window.charac.service.device && window.charac.service.device.gatt.connected) {
    console.log('Already connected, showing dashboard');
    // If we already have a valid connection, show the dashboard
    if (dashboard) {
      dashboard.classList.remove('hidden');
      dashboard.style.display = 'grid';
    }
    if (connectCard) {
      connectCard.classList.add('hidden');
    }
    if (statusIndicator) {
      statusIndicator.classList.add('connected');
    }
    // Try to read to verify connection is working
    try {
      console.log('Read:', (await window.charac.readValue()).getUint8(0));
    } catch (error) {
      console.log('Connection seems broken, resetting...');
      window.charac = false;
      window.notificationsActive = false;
      // Retry connection
      return window.ble();
    }
  } else {
    console.log("Setting up new connection!");
    try {
      const device = await navigator.bluetooth.requestDevice({
        filters: [{ name: ESPNAME }],
        optionalServices: [SERVICE_UUID]
      });
      const server = await device.gatt.connect();
      const service = await server.getPrimaryService(SERVICE_UUID);
      window.charac = await service.getCharacteristic(CHARACTERISTIC_UUID);
      console.log('Characteristic obtained successfully');
      
      if (!window.notificationsActive) {
        await window.charac.startNotifications();
        window.notificationsActive = true;
        window.charac.addEventListener('characteristicvaluechanged', window.bleRead);
      }
      
      // Connection successful - update UI
      if (dashboard) {
        dashboard.classList.remove('hidden');
        dashboard.style.display = 'grid';
        console.log('Dashboard shown');
      }
      if (connectCard) {
        connectCard.classList.add('hidden');
        console.log('Connect card hidden');
      }
      if (statusIndicator) {
        statusIndicator.classList.add('connected');
        console.log('Status indicator updated');
      }
      
      // Dispatch custom event for successful connection
      window.dispatchEvent(new CustomEvent('bleConnected'));
      
      const testButton = document.querySelector('button[onclick*="TestEvent"]');
      if (!testButton) {
        document.getElementById('btConnect').insertAdjacentHTML('afterend',
          "<button onclick=\"window.bleWrite('TestEvent')\">TestEvent</button>");
      }
          } catch (error) {
      console.error('Bluetooth connection failed:', error);
      
      // Reset UI to initial state on connection failure
      if (dashboard) {
        dashboard.classList.add('hidden');
      }
      if (connectCard) {
        connectCard.classList.remove('hidden');
      }
      if (statusIndicator) {
        statusIndicator.classList.remove('connected');
      }
        // Dispatch custom event for failed connection
      window.dispatchEvent(new CustomEvent('bleConnectionFailed', { detail: error }));
      throw error;
    }
  }
}

// Function to update color settings
function updateColor() {
  console.log('updateColor called on element:', this.id, 'with value:', this.value);
  let color = this.value.slice(1);
  let r = parseInt(color.substr(0, 2), 16);
  let g = parseInt(color.substr(2, 2), 16);
  let b = parseInt(color.substr(4, 2), 16);
  
  // Map UI element IDs to data keys
  const colorMap = {
    'colorOne': 'colorOne',
    'colorTwo': 'colorTwo', 
    'colorThree': 'colorThree'
  };
  
  const dataKey = colorMap[this.id];
  if (dataKey) {
    window.myData[dataKey] = `${r},${g},${b}`;
    console.log('Color updated:', dataKey, '=', window.myData[dataKey]);
    // Save settings and write to BLE immediately
    window.saveSettings();
    if (window.charac && window.charac.service && window.charac.service.device && window.charac.service.device.gatt.connected) {
      console.log('Writing color change to BLE...');
      window.bleWrite();
    } else {
      console.log('BLE not connected, color saved locally only');
    }
  } else {
    console.error('No data key found for element:', this.id);
  }
}

// Debounce function to prevent excessive Bluetooth writes
function debounce(func, wait) {
  let timeout;
  return function executedFunction(...args) {
    const later = () => {
      clearTimeout(timeout);
      func(...args);
    };
    clearTimeout(timeout);
    timeout = setTimeout(later, wait);
  };
}

// Debounced version of bleWrite for number inputs
const debouncedBleWrite = debounce(() => {
  window.saveSettings();
  if (window.charac && window.charac.service && window.charac.service.device && window.charac.service.device.gatt.connected) {
    window.bleWrite();
  }
}, 300);

// Add event listeners when DOM is loaded
document.addEventListener('DOMContentLoaded', () => {
  console.log('DOM Content Loaded - setting up event listeners');
  
  // Load settings from local storage first
  window.loadSettings();
  
  // Update UI with loaded settings
  setTimeout(() => {
    window.updateUIFromData();
  }, 100);

  // Skip color picker setup since it's handled in HTML
  // Color pickers are now handled by manual listeners in HTML
  
  // Animation delay control - fix the ID
  const loopInterval = document.getElementById('loopInterval');
  if (loopInterval) {
    loopInterval.addEventListener('input', function() {
      console.log('Animation delay changed to:', this.value);
      window.myData.loopInterval = parseInt(this.value);
      debouncedBleWrite();
    });
    console.log('Added listener for loopInterval');
  } else {
    console.warn('loopInterval element not found');
  }

  // Brightness control
  const brightness = document.getElementById('brightness');
  if (brightness) {
    brightness.addEventListener('input', function() {
      console.log('Brightness changed to:', this.value);
      window.myData.brightness = parseInt(this.value);
      debouncedBleWrite();
    });
    console.log('Added listener for brightness');
  } else {
    console.warn('brightness element not found');
  }

  // LED count control
  const ledCount = document.getElementById('ledCount');
  if (ledCount) {
    ledCount.addEventListener('input', function() {
      console.log('LED count changed to:', this.value);
      window.myData.ledCount = parseInt(this.value);
      debouncedBleWrite();
    });
    console.log('Added listener for ledCount');
  } else {
    console.warn('ledCount element not found');
  }

  // Pixel pin control
  const pixelPin = document.getElementById('pixelPin');
  if (pixelPin) {
    pixelPin.addEventListener('input', function() {
      console.log('Pixel pin changed to:', this.value);
      window.myData.pixelPin = parseInt(this.value);
      debouncedBleWrite();
    });
    console.log('Added listener for pixelPin');
  } else {
    console.warn('pixelPin element not found');
  }

  // Max current control
  const maxCurrent = document.getElementById('maxCurrent');
  if (maxCurrent) {
    maxCurrent.addEventListener('input', function() {
      console.log('Max current changed to:', this.value);
      window.myData.maxCurrent = parseInt(this.value);
      debouncedBleWrite();
    });
    console.log('Added listener for maxCurrent');
  } else {
    console.warn('maxCurrent element not found');
  }

  // Debug: List all input elements found
  console.log('All input elements on page:', {
    colorOne: !!document.getElementById('colorOne'),
    colorTwo: !!document.getElementById('colorTwo'),
    colorThree: !!document.getElementById('colorThree'),
    loopInterval: !!document.getElementById('loopInterval'),
    brightness: !!document.getElementById('brightness'),
    ledCount: !!document.getElementById('ledCount'),
    pixelPin: !!document.getElementById('pixelPin'),
    maxCurrent: !!document.getElementById('maxCurrent')
  });
});