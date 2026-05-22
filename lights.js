const SERVICE_UUID = '4fafc201-1fb5-459e-8fcc-c5c9c3319123';
const CHARACTERISTIC_UUID = 'beb5483e-36e1-4688-b7f5-ea07361b2623';
const ESP_NAME = 'Music Strip';
const STORAGE_KEYS = ['ledControllerDefaults', 'ledControllerSettings'];
const LAST_DEVICE_ID_KEY = 'ledControllerLastDeviceId';
const PROFILE_STORAGE_KEY = 'ledControllerProfiles';
const ACTIVE_PROFILE_KEY = 'ledControllerActiveProfileId';

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
const MODE_TUNABLE_FIELDS = ['colorOne', 'colorTwo', 'colorThree', 'brightness', 'speed', 'intensity', 'count', 'direction'];
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

const MODE_SETTINGS_CONFIG = {
  static: {
    settings: ['colorOne', 'brightness'],
    defaults: { colorOne: '#ff0000', brightness: 20 }
  },
  statictri: {
    settings: ['colorOne', 'colorTwo', 'colorThree', 'brightness'],
    defaults: { colorOne: '#ff0000', colorTwo: '#00ff00', colorThree: '#0000ff', brightness: 20 }
  },
  perlinmove: {
    settings: ['colorOne', 'colorTwo', 'colorThree', 'brightness', 'speed', 'intensity', 'direction'],
    defaults: { colorOne: '#4f86f7', colorTwo: '#7bdff2', colorThree: '#b2f7ef', brightness: 35, speed: 45, intensity: 70, direction: 2 }
  },
  stream: {
    settings: ['colorOne', 'colorTwo', 'colorThree', 'brightness', 'speed', 'intensity', 'direction'],
    defaults: { colorOne: '#00d4ff', colorTwo: '#ffffff', colorThree: '#0088ff', brightness: 30, speed: 60, intensity: 65, direction: 0 }
  },
  palette: {
    settings: ['colorOne', 'colorTwo', 'colorThree', 'brightness', 'speed', 'intensity'],
    defaults: { colorOne: '#ff006e', colorTwo: '#fb5607', colorThree: '#ffbe0b', brightness: 30, speed: 50, intensity: 75 }
  },
  plasma: {
    settings: ['colorOne', 'colorTwo', 'colorThree', 'brightness', 'speed', 'intensity'],
    defaults: { colorOne: '#6a00f4', colorTwo: '#00b4d8', colorThree: '#90e0ef', brightness: 35, speed: 55, intensity: 80 }
  },
  pacifica: {
    settings: ['colorOne', 'colorTwo', 'colorThree', 'brightness', 'speed', 'intensity'],
    defaults: { colorOne: '#003049', colorTwo: '#2a9d8f', colorThree: '#8ecae6', brightness: 25, speed: 35, intensity: 70 }
  },
  sunrise: {
    settings: ['colorOne', 'colorTwo', 'colorThree', 'brightness', 'speed', 'intensity'],
    defaults: { colorOne: '#ff4d00', colorTwo: '#ffb703', colorThree: '#ffd166', brightness: 40, speed: 20, intensity: 65 }
  },
  aurora: {
    settings: ['colorOne', 'colorTwo', 'colorThree', 'brightness', 'speed', 'intensity'],
    defaults: { colorOne: '#00f5d4', colorTwo: '#9b5de5', colorThree: '#f15bb5', brightness: 30, speed: 40, intensity: 70 }
  },
  percent: {
    settings: ['colorOne', 'brightness', 'count'],
    defaults: { colorOne: '#00ff00', brightness: 25, count: 2 }
  },
  percenttri: {
    settings: ['colorOne', 'colorTwo', 'colorThree', 'brightness', 'count'],
    defaults: { colorOne: '#00ff00', colorTwo: '#ffff00', colorThree: '#ff0000', brightness: 25, count: 3 }
  },
  shift: {
    settings: ['colorOne', 'colorTwo', 'colorThree', 'brightness', 'speed', 'intensity', 'direction'],
    defaults: { colorOne: '#ff0054', colorTwo: '#00bbf9', colorThree: '#fee440', brightness: 30, speed: 55, intensity: 70, direction: 0 }
  },
  washingmachine: {
    settings: ['colorOne', 'colorTwo', 'colorThree', 'brightness', 'speed', 'intensity', 'direction'],
    defaults: { colorOne: '#3a86ff', colorTwo: '#8338ec', colorThree: '#ff006e', brightness: 30, speed: 60, intensity: 75, direction: 2 }
  },
  blink: {
    settings: ['colorOne', 'brightness', 'speed'],
    defaults: { colorOne: '#ffffff', brightness: 30, speed: 70 }
  },
  blinktoggle: {
    settings: ['colorOne', 'colorTwo', 'brightness', 'speed'],
    defaults: { colorOne: '#ffffff', colorTwo: '#ff0000', brightness: 30, speed: 65 }
  },
  blinkrandom: {
    settings: ['brightness', 'speed', 'intensity'],
    defaults: { brightness: 30, speed: 65, intensity: 80 }
  },
  heartbeat: {
    settings: ['colorOne', 'brightness', 'speed', 'intensity'],
    defaults: { colorOne: '#ff0033', brightness: 30, speed: 45, intensity: 70 }
  },
  twinkles: {
    settings: ['colorOne', 'colorTwo', 'colorThree', 'brightness', 'speed', 'intensity', 'count'],
    defaults: { colorOne: '#ffffff', colorTwo: '#caffbf', colorThree: '#9bf6ff', brightness: 25, speed: 50, intensity: 75, count: 4 }
  },
  swipe: {
    settings: ['colorOne', 'colorTwo', 'brightness', 'speed', 'direction'],
    defaults: { colorOne: '#00f5ff', colorTwo: '#006dff', brightness: 30, speed: 55, direction: 0 }
  },
  swiperandom: {
    settings: ['brightness', 'speed', 'intensity', 'direction'],
    defaults: { brightness: 30, speed: 60, intensity: 80, direction: 0 }
  },
  colorloop: {
    settings: ['colorOne', 'colorTwo', 'colorThree', 'brightness', 'speed'],
    defaults: { colorOne: '#ff0000', colorTwo: '#00ff00', colorThree: '#0000ff', brightness: 30, speed: 50 }
  },
  breath: {
    settings: ['colorOne', 'colorTwo', 'brightness', 'speed', 'intensity'],
    defaults: { colorOne: '#00e5ff', colorTwo: '#0055ff', brightness: 25, speed: 35, intensity: 70 }
  },
  sweep: {
    settings: ['colorOne', 'brightness', 'speed', 'direction'],
    defaults: { colorOne: '#ffd60a', brightness: 30, speed: 60, direction: 0 }
  },
  sweepdual: {
    settings: ['colorOne', 'colorTwo', 'brightness', 'speed', 'direction'],
    defaults: { colorOne: '#ffd60a', colorTwo: '#00b4d8', brightness: 30, speed: 60, direction: 2 }
  },
  theater: {
    settings: ['colorOne', 'colorTwo', 'colorThree', 'brightness', 'speed', 'count', 'direction'],
    defaults: { colorOne: '#ffffff', colorTwo: '#ff006e', colorThree: '#8338ec', brightness: 30, speed: 70, count: 3, direction: 0 }
  },
  fireworks: {
    settings: ['colorOne', 'colorTwo', 'colorThree', 'brightness', 'speed', 'intensity', 'count'],
    defaults: { colorOne: '#ffbe0b', colorTwo: '#fb5607', colorThree: '#ff006e', brightness: 35, speed: 60, intensity: 80, count: 4 }
  },
  juggle: {
    settings: ['colorOne', 'colorTwo', 'colorThree', 'brightness', 'speed', 'count', 'direction'],
    defaults: { colorOne: '#38b000', colorTwo: '#70e000', colorThree: '#ccff33', brightness: 30, speed: 55, count: 4, direction: 0 }
  },
  bouncingballs: {
    settings: ['colorOne', 'colorTwo', 'colorThree', 'brightness', 'speed', 'intensity', 'count', 'direction'],
    defaults: { colorOne: '#caf0f8', colorTwo: '#90e0ef', colorThree: '#00b4d8', brightness: 30, speed: 45, intensity: 75, count: 3, direction: 2 }
  },
  meteor: {
    settings: ['colorOne', 'colorTwo', 'colorThree', 'brightness', 'speed', 'intensity', 'direction'],
    defaults: { colorOne: '#f72585', colorTwo: '#7209b7', colorThree: '#3a0ca3', brightness: 30, speed: 65, intensity: 75, direction: 0 }
  },
  tetrix: {
    settings: ['colorOne', 'colorTwo', 'colorThree', 'brightness', 'speed', 'intensity', 'count', 'direction'],
    defaults: { colorOne: '#f77f00', colorTwo: '#fcbf49', colorThree: '#eae2b7', brightness: 30, speed: 55, intensity: 70, count: 4, direction: 0 }
  },
  candle: {
    settings: ['colorOne', 'colorTwo', 'colorThree', 'brightness', 'speed', 'intensity'],
    defaults: { colorOne: '#ff6d00', colorTwo: '#ff8500', colorThree: '#ffd166', brightness: 20, speed: 25, intensity: 85 }
  }
};

window.charac = window.charac || null;
window.notificationsActive = window.notificationsActive || false;
window.queued = window.queued || false;
window.processing = window.processing || false;
window.bleConnecting = window.bleConnecting || null;
window.settingsProfiles = window.settingsProfiles || [];
window.activeProfileId = window.activeProfileId || null;

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

function nextPresetName(profiles) {
  let index = 1;
  const taken = new Set(profiles.map((profile) => profile.name));
  while (taken.has(`Preset ${index}`)) {
    index += 1;
  }
  return `Preset ${index}`;
}

function buildProfileData(data) {
  return normalizeSettings({ ...data });
}

function saveProfiles() {
  const payload = {
    profiles: window.settingsProfiles,
    activeProfileId: window.activeProfileId
  };
  localStorage.setItem(PROFILE_STORAGE_KEY, JSON.stringify(payload));

  if (window.activeProfileId) {
    localStorage.setItem(ACTIVE_PROFILE_KEY, window.activeProfileId);
  }
}

function parseStoredProfiles() {
  const raw = localStorage.getItem(PROFILE_STORAGE_KEY);
  if (!raw) {
    return { profiles: [], activeProfileId: localStorage.getItem(ACTIVE_PROFILE_KEY) };
  }

  try {
    const parsed = JSON.parse(raw);
    return {
      profiles: Array.isArray(parsed?.profiles) ? parsed.profiles : [],
      activeProfileId: parsed?.activeProfileId || localStorage.getItem(ACTIVE_PROFILE_KEY)
    };
  } catch (error) {
    console.error('Failed to parse profile storage:', error);
    return { profiles: [], activeProfileId: localStorage.getItem(ACTIVE_PROFILE_KEY) };
  }
}

function buildNewProfile(name, data) {
  return {
    id: `profile_${Date.now()}_${Math.random().toString(36).slice(2, 8)}`,
    name,
    data: buildProfileData(data),
    updatedAt: Date.now()
  };
}

function profileModeLabel(mode) {
  const animationMode = document.getElementById('animationMode');
  if (!animationMode) {
    return mode;
  }

  const option = Array.from(animationMode.options).find((item) => item.value === mode);
  return option ? option.text : mode;
}

function profileSettingsSummary(data) {
  const normalized = normalizeSettings(data || {});
  return `Br ${normalized.brightness}% · Sp ${normalized.speed}% · In ${normalized.intensity}% · Ct ${normalized.count}`;
}

function activeProfile() {
  return window.settingsProfiles.find((profile) => profile.id === window.activeProfileId) || null;
}

function renderProfileCards() {
  const container = document.getElementById('profileCards');
  if (!container) {
    return;
  }

  container.innerHTML = '';

  window.settingsProfiles.forEach((profile) => {
    const card = document.createElement('div');
    card.className = `profile-card${profile.id === window.activeProfileId ? ' active' : ''}`;
    card.setAttribute('role', 'button');
    card.setAttribute('tabindex', '0');

    const header = document.createElement('div');
    header.className = 'profile-card-header';

    const name = document.createElement('div');
    name.className = 'profile-name';
    name.textContent = profile.name;

    const actions = document.createElement('div');
    actions.className = 'profile-actions';

    const edit = document.createElement('button');
    edit.className = 'profile-edit-btn';
    edit.type = 'button';
    edit.textContent = 'Edit';
    edit.addEventListener('click', (event) => {
      event.stopPropagation();
      const nextName = window.prompt('Rename saved setting', profile.name);
      if (!nextName) {
        return;
      }

      const cleaned = nextName.trim();
      if (!cleaned) {
        return;
      }

      profile.name = cleaned;
      profile.updatedAt = Date.now();
      saveProfiles();
      renderProfileCards();
    });

    const remove = document.createElement('button');
    remove.className = 'profile-remove-btn';
    remove.type = 'button';
    remove.textContent = 'Remove';
    remove.addEventListener('click', (event) => {
      event.stopPropagation();

      if (window.settingsProfiles.length <= 1) {
        window.alert('At least one preset is required.');
        return;
      }

      const confirmed = window.confirm(`Remove saved setting "${profile.name}"?`);
      if (!confirmed) {
        return;
      }

      const removedIndex = window.settingsProfiles.findIndex((item) => item.id === profile.id);
      if (removedIndex < 0) {
        return;
      }

      const wasActive = profile.id === window.activeProfileId;
      window.settingsProfiles.splice(removedIndex, 1);

      if (wasActive) {
        const fallbackIndex = Math.max(0, removedIndex - 1);
        const fallbackProfile = window.settingsProfiles[fallbackIndex] || window.settingsProfiles[0];
        if (fallbackProfile) {
          window.activeProfileId = fallbackProfile.id;
          window.myData = buildProfileData(fallbackProfile.data);
          persistSettings();
          applyDataToUI();
          if (isConnected()) {
            window.bleWrite();
          }
        }
      }

      saveProfiles();
      renderProfileCards();
    });

    actions.append(edit, remove);
    header.append(name, actions);

    const sourceData = profile.id === window.activeProfileId ? window.myData : profile.data;

    const mode = document.createElement('div');
    mode.className = 'profile-mode';
    mode.textContent = profileModeLabel(sourceData.lightMode || DEFAULT_VALUES.lightMode);

    const settings = document.createElement('div');
    settings.className = 'profile-settings';
    settings.textContent = profileSettingsSummary(sourceData);

    card.append(header, mode, settings);

    const activate = () => {
      if (profile.id === window.activeProfileId) {
        return;
      }

      window.activeProfileId = profile.id;
      window.myData = buildProfileData(profile.data);
      persistSettings();
      applyDataToUI();
      saveProfiles();
      renderProfileCards();

      if (isConnected()) {
        window.bleWrite();
      }
    };

    card.addEventListener('click', activate);
    card.addEventListener('keydown', (event) => {
      if (event.key === 'Enter' || event.key === ' ') {
        event.preventDefault();
        activate();
      }
    });

    container.appendChild(card);
  });

  const createCard = document.createElement('div');
  createCard.className = 'profile-card create-new';
  createCard.setAttribute('role', 'button');
  createCard.setAttribute('tabindex', '0');
  createCard.textContent = '+ Create New';

  const createNew = () => {
    const newProfile = buildNewProfile(nextPresetName(window.settingsProfiles), window.myData);
    window.settingsProfiles.push(newProfile);
    window.activeProfileId = newProfile.id;
    saveProfiles();
    renderProfileCards();
  };

  createCard.addEventListener('click', createNew);
  createCard.addEventListener('keydown', (event) => {
    if (event.key === 'Enter' || event.key === ' ') {
      event.preventDefault();
      createNew();
    }
  });

  container.appendChild(createCard);
}

function initializeProfiles() {
  const stored = parseStoredProfiles();
  const profiles = stored.profiles
    .map((profile) => {
      if (!profile || !profile.id || !profile.name) {
        return null;
      }

      return {
        id: profile.id,
        name: String(profile.name),
        data: buildProfileData(profile.data || {}),
        updatedAt: Number(profile.updatedAt) || Date.now()
      };
    })
    .filter(Boolean);

  if (!profiles.length) {
    const fallback = buildNewProfile('Preset 1', window.myData);
    profiles.push(fallback);
  }

  window.settingsProfiles = profiles;
  window.activeProfileId = profiles.some((profile) => profile.id === stored.activeProfileId)
    ? stored.activeProfileId
    : profiles[0].id;

  const selected = activeProfile();
  if (selected) {
    window.myData = buildProfileData(selected.data);
    persistSettings();
  }

  saveProfiles();
}

function bindProfileControls() {
  const saveButton = document.getElementById('saveProfileBtn');
  if (saveButton && saveButton.dataset.lightsBound !== 'true') {
    saveButton.addEventListener('click', () => {
      const profile = activeProfile();
      if (!profile) {
        return;
      }

      profile.data = buildProfileData(window.myData);
      profile.updatedAt = Date.now();
      saveProfiles();
      renderProfileCards();
    });
    saveButton.dataset.lightsBound = 'true';
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

  applyModeSettings(window.myData.lightMode, false);
}

function modeConfigFor(mode) {
  return MODE_SETTINGS_CONFIG[mode] || MODE_SETTINGS_CONFIG[DEFAULT_VALUES.lightMode];
}

function setModeFieldVisibility(fieldId, visible) {
  const element = document.getElementById(fieldId);
  if (!element) {
    return;
  }

  const group = element.closest('.control-group');
  if (group) {
    group.style.display = visible ? '' : 'none';
  } else {
    element.style.display = visible ? '' : 'none';
  }
}

function applyModeSettings(mode, applyDefaults) {
  const config = modeConfigFor(mode);
  const visibleSettings = new Set(config.settings || []);

  MODE_TUNABLE_FIELDS.forEach((fieldId) => {
    setModeFieldVisibility(fieldId, visibleSettings.has(fieldId));
  });

  if (!applyDefaults) {
    renderProfileCards();
    return;
  }

  const nextData = { ...window.myData, lightMode: mode };
  Object.entries(config.defaults || {}).forEach(([fieldId, value]) => {
    const dataKey = FIELD_MAP[fieldId];
    if (!dataKey) {
      return;
    }
    nextData[dataKey] = value;
  });

  window.myData = normalizeSettings(nextData);
  persistSettings();
  applyDataToUI();
  renderProfileCards();

  if (isConnected()) {
    window.bleWrite();
  }
}

function updateData(fieldId, rawValue) {
  const dataKey = FIELD_MAP[fieldId];
  if (!dataKey) {
    return;
  }

  if (fieldId === 'animationMode') {
    applyModeSettings(rawValue, true);
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
  renderProfileCards();

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
  renderProfileCards();

  if (isConnected()) {
    window.bleWrite();
  }
};

window.initLightsController = () => {
  window.myData = mergeStoredSettings();
  initializeProfiles();
  bindControls();
  bindProfileControls();
  applyDataToUI();
  renderProfileCards();
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
