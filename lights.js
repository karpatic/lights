const SERVICE_UUID = '4fafc201-1fb5-459e-8fcc-c5c9c3319123';
const CHARACTERISTIC_UUID = 'beb5483e-36e1-4688-b7f5-ea07361b2623';
const ESP_NAME = 'Music Strip';
const STORAGE_KEYS = ['ledControllerDefaults', 'ledControllerSettings'];
const LAST_DEVICE_ID_KEY = 'ledControllerLastDeviceId';

const DEFAULT_VALUES = {
  colorOne: 0xff0000,
  colorTwo: 0x00ff00,
  colorThree: 0x0000ff,
  brightness: 20,
  speed: 50,
  intensity: 75,
  count: 2,
  direction: 0,
  ledCount: 300,
  pixelCount: 300,
  pixelPin: 15,
  maxCurrent: 8000,
  colorOrder: 'GRB',
  lightMode: 'static',
  updated: true
};

const COLOR_FIELDS = ['colorOne', 'colorTwo', 'colorThree'];
const NUMBER_FIELDS = ['brightness', 'speed', 'intensity', 'count', 'ledCount', 'pixelPin', 'maxCurrent'];
const SELECT_FIELDS = ['animationMode', 'colorOrder', 'direction'];
const COLOR_ORDERS = new Set(['RGB', 'RBG', 'GRB', 'GBR', 'BRG', 'BGR']);
const FIELD_MAP = {
  animationMode: 'lightMode',
  colorOrder: 'colorOrder',
  colorOne: 'colorOne',
  colorTwo: 'colorTwo',
  colorThree: 'colorThree',
  brightness: 'brightness',
  speed: 'speed',
  intensity: 'intensity',
  count: 'count',
  direction: 'direction',
  ledCount: 'ledCount',
  pixelPin: 'pixelPin',
  maxCurrent: 'maxCurrent'
};

window.charac = window.charac || null;
window.notificationsActive = window.notificationsActive || false;
window.queued = window.queued || false;
window.processing = window.processing || false;
window.bleConnecting = window.bleConnecting || null;

function clampNumber(value, fallback, min, max) {
  const parsed = Number.parseInt(value, 10);
  if (Number.isNaN(parsed)) {
    return fallback;
  }
  return Math.min(max, Math.max(min, parsed));
}

function rgbStringToHexInt(value) {
  const parts = String(value).split(',').map((part) => clampNumber(part.trim(), 0, 0, 255));
  if (parts.length !== 3) {
    return null;
  }
  return (parts[0] << 16) | (parts[1] << 8) | parts[2];
}

function normalizeColorValue(value, fallback = 0) {
  if (typeof value === 'string') {
    if (value.startsWith('#')) {
      const parsed = Number.parseInt(value.slice(1), 16);
      return Number.isNaN(parsed) ? fallback : parsed & 0xffffff;
    }

    if (value.includes(',')) {
      const parsed = rgbStringToHexInt(value);
      return parsed === null ? fallback : parsed;
    }
  }

  const parsed = Number(value);
  return Number.isFinite(parsed) ? parsed & 0xffffff : fallback;
}

function colorToHexInt(colorString) {
  return normalizeColorValue(colorString);
}

function hexIntToColor(value) {
  return `#${normalizeColorValue(value).toString(16).padStart(6, '0')}`;
}

function hexIntToRgbString(value) {
  const color = normalizeColorValue(value);
  const red = (color >> 16) & 0xff;
  const green = (color >> 8) & 0xff;
  const blue = color & 0xff;
  return `${red},${green},${blue}`;
}

function normalizeColorOrder(value) {
  const order = String(value || DEFAULT_VALUES.colorOrder).replace(/^NEO_/, '').toUpperCase();
  return COLOR_ORDERS.has(order) ? order : DEFAULT_VALUES.colorOrder;
}

function normalizeSettings(settings = {}) {
  const hasPixelCount = settings.pixelCount !== undefined;
  const normalized = { ...DEFAULT_VALUES, ...settings };

  normalized.colorOne = normalizeColorValue(normalized.colorOne, DEFAULT_VALUES.colorOne);
  normalized.colorTwo = normalizeColorValue(normalized.colorTwo, DEFAULT_VALUES.colorTwo);
  normalized.colorThree = normalizeColorValue(normalized.colorThree, DEFAULT_VALUES.colorThree);
  normalized.brightness = clampNumber(normalized.brightness, DEFAULT_VALUES.brightness, 0, 100);
  normalized.speed = clampNumber(normalized.speed, DEFAULT_VALUES.speed, 0, 100);
  normalized.intensity = clampNumber(normalized.intensity, DEFAULT_VALUES.intensity, 0, 100);
  normalized.count = clampNumber(normalized.count, DEFAULT_VALUES.count, 0, 100);
  normalized.direction = clampNumber(normalized.direction, DEFAULT_VALUES.direction, 0, 2);
  normalized.ledCount = clampNumber(normalized.ledCount, DEFAULT_VALUES.ledCount, 1, 1000);
  normalized.pixelCount = clampNumber(hasPixelCount ? normalized.pixelCount : normalized.ledCount, normalized.ledCount, 1, 1000);
  normalized.pixelPin = clampNumber(normalized.pixelPin, DEFAULT_VALUES.pixelPin, 0, 48);
  normalized.maxCurrent = clampNumber(normalized.maxCurrent, DEFAULT_VALUES.maxCurrent, 0, 50000);
  normalized.colorOrder = normalizeColorOrder(normalized.colorOrder);
  normalized.lightMode = normalized.lightMode || DEFAULT_VALUES.lightMode;
  normalized.updated = true;

  return normalized;
}

function buildEsp32Payload(data) {
  const settings = normalizeSettings(data);

  return {
    brightness: settings.brightness,
    lightMode: String(settings.lightMode).slice(0, 31),
    colorOne: hexIntToRgbString(settings.colorOne),
    colorTwo: hexIntToRgbString(settings.colorTwo),
    colorThree: hexIntToRgbString(settings.colorThree),
    ledCount: settings.ledCount,
    colorOrder: settings.colorOrder,
    maxCurrent: settings.maxCurrent,
    pixelPin: settings.pixelPin,
    pixelCount: settings.pixelCount,
    speed: settings.speed,
    intensity: settings.intensity,
    count: settings.count,
    direction: settings.direction
  };
}

function mergeStoredSettings() {
  const merged = {};

  for (const storageKey of STORAGE_KEYS) {
    const raw = localStorage.getItem(storageKey);
    if (!raw) {
      continue;
    }

    try {
      Object.assign(merged, JSON.parse(raw));
    } catch (error) {
      console.error(`Failed to parse ${storageKey}:`, error);
    }
  }

  return normalizeSettings(merged);
}

function persistSettings() {
  const payload = JSON.stringify(window.myData);
  for (const storageKey of STORAGE_KEYS) {
    localStorage.setItem(storageKey, payload);
  }
}

function connectedGatt() {
  return window.charac?.service?.device?.gatt;
}

function isConnected() {
  return Boolean(connectedGatt()?.connected);
}

function applyUIState(connected) {
  const dashboard = document.getElementById('btDash');
  const connectCard = document.getElementById('connectCard');
  const statusIndicator = document.getElementById('statusIndicator');
  const connectButton = document.getElementById('btConnect');

  if (dashboard) {
    dashboard.classList.toggle('hidden', !connected);
    dashboard.style.display = connected ? 'grid' : '';
  }

  if (connectCard) {
    connectCard.classList.toggle('hidden', connected);
  }

  if (statusIndicator) {
    statusIndicator.classList.toggle('connected', connected);
  }

  if (connectButton) {
    connectButton.disabled = connected;
  }
}

function applyDataToUI() {
  COLOR_FIELDS.forEach((field) => {
    const element = document.getElementById(field);
    if (element && window.myData[field] !== undefined) {
      element.value = hexIntToColor(window.myData[field]);
    }
  });

  NUMBER_FIELDS.forEach((field) => {
    const element = document.getElementById(field);
    if (element && window.myData[field] !== undefined) {
      element.value = String(window.myData[field]);
    }
  });

  const direction = document.getElementById('direction');
  if (direction && window.myData.direction !== undefined) {
    direction.value = String(window.myData.direction);
  }

  const animationMode = document.getElementById('animationMode');
  if (animationMode && window.myData.lightMode !== undefined) {
    animationMode.value = window.myData.lightMode;
  }

  const colorOrder = document.getElementById('colorOrder');
  if (colorOrder && window.myData.colorOrder !== undefined) {
    colorOrder.value = window.myData.colorOrder;
  }
}

function updateData(fieldId, rawValue) {
  const dataKey = FIELD_MAP[fieldId];
  if (!dataKey) {
    return;
  }

  if (COLOR_FIELDS.includes(fieldId)) {
    window.myData[dataKey] = colorToHexInt(rawValue);
  } else if (NUMBER_FIELDS.includes(fieldId) || fieldId === 'direction') {
    window.myData[dataKey] = Number.parseInt(rawValue, 10);
  } else {
    window.myData[dataKey] = rawValue;
  }

  if (dataKey === 'ledCount') {
    window.myData.pixelCount = window.myData.ledCount;
  }

  window.myData.updated = true;
  persistSettings();

  if (isConnected()) {
    window.bleWrite();
  }
}

function bindElement(id, eventName) {
  const element = document.getElementById(id);
  if (!element || element.dataset.lightsBound === 'true') {
    return;
  }

  element.addEventListener(eventName, function handleControlChange() {
    updateData(id, this.value);
  });
  element.dataset.lightsBound = 'true';
}

function bindControls() {
  COLOR_FIELDS.forEach((field) => bindElement(field, 'input'));
  NUMBER_FIELDS.forEach((field) => bindElement(field, 'input'));
  SELECT_FIELDS.forEach((field) => bindElement(field, 'change'));
}

function handleDisconnect() {
  window.notificationsActive = false;
  window.charac = null;
  applyUIState(false);
  window.dispatchEvent(new CustomEvent('bleDisconnected'));
}

function attachDisconnectHandler(device) {
  if (!device || device.datasetLightsDisconnectBound) {
    return;
  }

  device.addEventListener('gattserverdisconnected', handleDisconnect);
  device.datasetLightsDisconnectBound = true;
}

function rememberDevice(device) {
  if (!device?.id) {
    return;
  }

  localStorage.setItem(LAST_DEVICE_ID_KEY, device.id);
}

function loadRememberedDeviceId() {
  return localStorage.getItem(LAST_DEVICE_ID_KEY);
}

async function connectToDevice(device) {
  attachDisconnectHandler(device);

  const server = device.gatt.connected ? device.gatt : await device.gatt.connect();
  const service = await server.getPrimaryService(SERVICE_UUID);
  window.charac = await service.getCharacteristic(CHARACTERISTIC_UUID);

  if (!window.notificationsActive) {
    await window.charac.startNotifications();
    window.charac.addEventListener('characteristicvaluechanged', window.bleRead);
    window.notificationsActive = true;
  }

  rememberDevice(device);
  applyUIState(true);
  await window.bleWrite();
  window.dispatchEvent(new CustomEvent('bleConnected'));
  return true;
}

function sortReconnectCandidates(devices) {
  const rememberedId = loadRememberedDeviceId();
  const matchesName = (device) => device?.name === ESP_NAME;
  const byRemembered = [];
  const byName = [];

  devices.forEach((device) => {
    if (rememberedId && device.id === rememberedId) {
      byRemembered.push(device);
      return;
    }

    if (matchesName(device)) {
      byName.push(device);
    }
  });

  return [...byRemembered, ...byName];
}

async function withConnectionLock(connectAction) {
  if (window.bleConnecting) {
    return window.bleConnecting;
  }

  window.bleConnecting = (async () => {
    try {
      return await connectAction();
    } finally {
      window.bleConnecting = null;
    }
  })();

  return window.bleConnecting;
}

window.getRandomColor = () => Math.floor(Math.random() * 0xffffff);

window.saveSettings = persistSettings;
window.buildEsp32Payload = buildEsp32Payload;

window.loadSettings = () => {
  window.myData = mergeStoredSettings();
  return window.myData;
};

window.updateUIFromData = applyDataToUI;

window.updateConnectionStatus = (connected) => {
  applyUIState(Boolean(connected));
};

window.checkConnectionStatus = () => {
  applyUIState(isConnected());
};

window.bleWrite = async () => {
  if (!isConnected()) {
    return;
  }

  if (window.processing) {
    window.queued = true;
    return;
  }

  window.processing = true;
  window.queued = false;
  persistSettings();

  try {
    const payload = buildEsp32Payload(window.myData);
    const encoded = new TextEncoder().encode(JSON.stringify(payload));
    if (window.charac.writeValueWithResponse) {
      await window.charac.writeValueWithResponse(encoded);
    } else {
      await window.charac.writeValue(encoded);
    }
  } catch (error) {
    console.error('Error during BLE write:', error);
  } finally {
    window.processing = false;
    if (window.queued) {
      window.queued = false;
      window.bleWrite();
    }
  }
};

window.bleRead = async ({ target: { value } }) => {
  const message = new TextDecoder().decode(value);

  try {
    const statusData = JSON.parse(message);
    window.myData = normalizeSettings({
      ...window.myData,
      ...statusData,
      updated: true
    });
    persistSettings();
    applyDataToUI();
  } catch (error) {
    if (!['OK', 'TRIGGERED', 'Ready'].includes(message)) {
      console.log('Non-JSON message from BLE:', message, error);
    }
  }
};

window.ble = async () => {
  if (!('bluetooth' in navigator)) {
    const error = new Error('Web Bluetooth is not supported in this browser.');
    console.error(error.message);
    window.dispatchEvent(new CustomEvent('bleConnectionFailed', { detail: error }));
    return false;
  }

  if (isConnected()) {
    applyUIState(true);
    return true;
  }

  return withConnectionLock(async () => {
    try {
      const device = await navigator.bluetooth.requestDevice({
        filters: [{ name: ESP_NAME }],
        optionalServices: [SERVICE_UUID]
      });
      return await connectToDevice(device);
    } catch (error) {
      applyUIState(false);
      console.error('Bluetooth connection failed:', error);
      window.dispatchEvent(new CustomEvent('bleConnectionFailed', { detail: error }));
      return false;
    }
  });
};

window.bleAutoReconnect = async () => {
  if (!('bluetooth' in navigator) || typeof navigator.bluetooth.getDevices !== 'function') {
    return false;
  }

  if (isConnected()) {
    applyUIState(true);
    return true;
  }

  return withConnectionLock(async () => {
    try {
      const authorizedDevices = await navigator.bluetooth.getDevices();
      const candidates = sortReconnectCandidates(authorizedDevices);

      for (const device of candidates) {
        try {
          const connected = await connectToDevice(device);
          if (connected) {
            return true;
          }
        } catch (error) {
          console.warn('Auto-reconnect candidate failed:', device?.name || device?.id, error);
        }
      }
    } catch (error) {
      console.warn('Auto-reconnect unavailable:', error);
    }

    applyUIState(false);
    return false;
  });
};

window.toggleAnimationMode = (direction) => {
  const animationMode = document.getElementById('animationMode');
  if (!animationMode) {
    return;
  }

  const optionCount = animationMode.options.length;
  const nextIndex = (animationMode.selectedIndex + direction + optionCount) % optionCount;
  animationMode.selectedIndex = nextIndex;
  updateData('animationMode', animationMode.value);
};

window.randomizeColors = () => {
  COLOR_FIELDS.forEach((field) => {
    const element = document.getElementById(field);
    const nextColor = window.getRandomColor();

    window.myData[field] = nextColor;
    if (element) {
      element.value = hexIntToColor(nextColor);
    }
  });

  window.myData.updated = true;
  persistSettings();

  if (isConnected()) {
    window.bleWrite();
  }
};

window.initLightsController = () => {
  window.myData = mergeStoredSettings();
  bindControls();
  applyDataToUI();
  applyUIState(isConnected());

  // Try reconnecting silently if the browser already has permission for the ESP32.
  window.bleAutoReconnect();
};

if (!window.__lightsDisableAutoBind) {
  if (document.readyState === 'loading') {
    document.addEventListener('DOMContentLoaded', () => {
      window.initLightsController();
    }, { once: true });
  } else {
    window.initLightsController();
  }
}
