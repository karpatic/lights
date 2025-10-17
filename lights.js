//todo: handle: lights.js:66 Uncaught (in promise) DOMException: GATT operation failed for unknown reason.

// Initialize variables
window.charac = false;
window.notificationsActive = false;
window.queued = false;
window.processing = false;

// Define data configuration with all supported keys  
window.myData = {
  lightMode: 'static',  // Speed control  
  const speed = document.getElementById('speed');
  if (speed) {
    speed.addEventListener('input', function() {
      console.log('Speed changed to:', this.value);
      window.myData.speed = parseInt(this.value);
      window.myData.updated = true;
      debouncedBleWrite();
    });
    console.log('Added listener for speed');
  } else {
    console.warn('speed element not found');
  }

  // Intensity control
  const intensity = document.getElementById('intensity');
  if (intensity) {
    intensity.addEventListener('input', function() {
      console.log('Intensity changed to:', this.value);
      window.myData.intensity = parseInt(this.value);
      window.myData.updated = true;
      debouncedBleWrite();
    });
    console.log('Added listener for intensity');
  } else {
    console.warn('intensity element not found');
  }

  // Count control
  const count = document.getElementById('count');
  if (count) {
    count.addEventListener('input', function() {
      console.log('Count changed to:', this.value);
      window.myData.count = parseInt(this.value);
      window.myData.updated = true;
      debouncedBleWrite();
    });
    console.log('Added listener for count');
  } else {
    console.warn('count element not found');
  }

  // Direction control
  const direction = document.getElementById('direction');
  if (direction) {
    direction.addEventListener('input', function() {
      console.log('Direction changed to:', this.value);
      window.myData.direction = parseInt(this.value);
      window.myData.updated = true;
      debouncedBleWrite();
    });
    console.log('Added listener for direction');
  } else {
    console.warn('direction element not found');
  }: 0xFF00CB,    // Hot pink as hex integer
  colorTwo: 0xFF0000,    // Red as hex integer  
  colorThree: 0x0000FF,  // Blue as hex integer
  brightness: 20,        // Match main.cpp default
  speed: 50,
  intensity: 75,
  count: 2,              // Match main.cpp default
  direction: 0,
  ledCount: 300,         // Match main.cpp default
  pixelPin: 15,
  maxCurrent: 8000,
  colorOrder: 'NEO_GRB', // Match main.cpp default
  updated: true
};

window.getRandomColor = () => {
  return Math.floor(Math.random() * 0xFFFFFF);
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
  // Update color pickers - now expecting hex integers
  if (window.myData.colorOne !== undefined) updateColorPicker('colorOne', window.myData.colorOne);
  if (window.myData.colorTwo !== undefined) updateColorPicker('colorTwo', window.myData.colorTwo);
  if (window.myData.colorThree !== undefined) updateColorPicker('colorThree', window.myData.colorThree);
  
  // Update number inputs with correct ID mapping
  const inputMappings = {
    'brightness': 'brightness',
    'speed': 'speed',
    'intensity': 'intensity', 
    'count': 'count',
    'direction': 'direction',
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

  // Update select inputs
  const selectMappings = {
    'animationMode': 'lightMode',
    'colorOrder': 'colorOrder'
  };
  
  Object.entries(selectMappings).forEach(([elementId, dataKey]) => {
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
    if (statusData.lightMode) {
      const modeSelect = document.getElementById('animationMode');
      if (modeSelect) modeSelect.value = statusData.lightMode;
    }
    if (statusData.brightness !== undefined) {
      const brightnessInput = document.getElementById('brightness');
      if (brightnessInput) brightnessInput.value = statusData.brightness;
    }
    if (statusData.speed !== undefined) {
      const speedInput = document.getElementById('speed');
      if (speedInput) speedInput.value = statusData.speed;
    }
    if (statusData.intensity !== undefined) {
      const intensityInput = document.getElementById('intensity');
      if (intensityInput) intensityInput.value = statusData.intensity;
    }
    if (statusData.count !== undefined) {
      const countInput = document.getElementById('count');
      if (countInput) countInput.value = statusData.count;
    }
    if (statusData.direction !== undefined) {
      const directionSelect = document.getElementById('direction');
      if (directionSelect) directionSelect.value = statusData.direction;
    }
    if (statusData.ledCount !== undefined) {
      const ledCountInput = document.getElementById('ledCount');
      if (ledCountInput) ledCountInput.value = statusData.ledCount;
    }
    if (statusData.pixelPin !== undefined) {
      const pixelPinInput = document.getElementById('pixelPin');
      if (pixelPinInput) pixelPinInput.value = statusData.pixelPin;
    }
    if (statusData.maxCurrent !== undefined) {
      const maxCurrentInput = document.getElementById('maxCurrent');
      if (maxCurrentInput) maxCurrentInput.value = statusData.maxCurrent;
    }
    if (statusData.colorOrder) {
      const colorOrderSelect = document.getElementById('colorOrder');
      if (colorOrderSelect) colorOrderSelect.value = statusData.colorOrder;
    }
    
    // Update color pickers - now expecting hex integers
    if (statusData.colorOne !== undefined) updateColorPicker('colorOne', statusData.colorOne);
    if (statusData.colorTwo !== undefined) updateColorPicker('colorTwo', statusData.colorTwo);
    if (statusData.colorThree !== undefined) updateColorPicker('colorThree', statusData.colorThree);
    
    // Log system info
    if (statusData.temperature) console.log('ESP32 Temperature:', statusData.temperature);
    if (statusData.firmware) console.log('Firmware:', statusData.firmware);
    if (statusData.freeHeap) console.log('Free Heap:', statusData.freeHeap);
    
  } catch (e) {
    console.log('Non-JSON message from BLE:', message);
  }
};

// Helper function to convert hex integer to color picker format
function updateColorPicker(elementId, hexInt) {
  const hex = '#' + hexInt.toString(16).padStart(6, '0');
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
  // Convert color string to hex integer
  const hexInt = parseInt(this.value.slice(1), 16);
  
  // Map UI element IDs to data keys
  const colorMap = {
    'colorOne': 'colorOne',
    'colorTwo': 'colorTwo', 
    'colorThree': 'colorThree'
  };
  
  const dataKey = colorMap[this.id];
  if (dataKey) {
    window.myData[dataKey] = hexInt;
    window.myData.updated = true;
    console.log('Color updated:', dataKey, '=', hexInt, '(0x' + hexInt.toString(16) + ')');
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
  
  // Animation mode control - replace the old loopInterval
  const animationMode = document.getElementById('animationMode');
  if (animationMode) {
    animationMode.addEventListener('change', function() {
      console.log('Animation mode changed to:', this.value);
      window.myData.lightMode = this.value;
      window.myData.updated = true;
      debouncedBleWrite();
    });
    console.log('Added listener for animationMode');
  } else {
    console.warn('animationMode element not found');
  }

  // Speed control  
  const speed = document.getElementById('speed');
  if (speed) {
    speed.addEventListener('input', function() {
      console.log('Speed changed to:', this.value);
      window.myData.speed = parseInt(this.value);
      window.myData.updated = true;
      debouncedBleWrite();
    });
    console.log('Added listener for speed');
  } else {
    console.warn('speed element not found');
  }

  // Intensity control
  const intensity = document.getElementById('intensity');
  if (intensity) {
    intensity.addEventListener('input', function() {
      console.log('Intensity changed to:', this.value);
      window.myData.intensity = parseInt(this.value);
      window.myData.updated = true;
      debouncedBleWrite();
    });
    console.log('Added listener for intensity');
  } else {
    console.warn('intensity element not found');
  }

  // Count control
  const count = document.getElementById('count');
  if (count) {
    count.addEventListener('input', function() {
      console.log('Count changed to:', this.value);
      window.myData.count = parseInt(this.value);
      window.myData.updated = true;
      debouncedBleWrite();
    });
    console.log('Added listener for count');
  } else {
    console.warn('count element not found');
  }

  // Direction control
  const direction = document.getElementById('direction');
  if (direction) {
    direction.addEventListener('change', function() {
      console.log('Direction changed to:', this.value);
      window.myData.direction = parseInt(this.value);
      window.myData.updated = true;
      debouncedBleWrite();
    });
    console.log('Added listener for direction');
  } else {
    console.warn('direction element not found');
  }

  // Color order control
  const colorOrder = document.getElementById('colorOrder');
  if (colorOrder) {
    colorOrder.addEventListener('change', function() {
      console.log('Color order changed to:', this.value);
      window.myData.colorOrder = this.value;
      window.myData.updated = true;
      debouncedBleWrite();
    });
    console.log('Added listener for colorOrder');
  } else {
    console.warn('colorOrder element not found');
  }

  // Brightness control
  const brightness = document.getElementById('brightness');
  if (brightness) {
    brightness.addEventListener('input', function() {
      console.log('Brightness changed to:', this.value);
      window.myData.brightness = parseInt(this.value);
      window.myData.updated = true;
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
      window.myData.updated = true;
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
      window.myData.updated = true;
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
      window.myData.updated = true;
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
    speed: !!document.getElementById('speed'),
    intensity: !!document.getElementById('intensity'),
    count: !!document.getElementById('count'),
    direction: !!document.getElementById('direction'),
    brightness: !!document.getElementById('brightness'),
    ledCount: !!document.getElementById('ledCount'),
    pixelPin: !!document.getElementById('pixelPin'),
    maxCurrent: !!document.getElementById('maxCurrent'),
    animationMode: !!document.getElementById('animationMode'),
    colorOrder: !!document.getElementById('colorOrder')
  });
});

// Start connection monitoring
setInterval(() => {
  if (window.checkConnectionStatus) {
    window.checkConnectionStatus();
  }
}, 1000);