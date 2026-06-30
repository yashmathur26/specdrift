/*
  JUCE WebView frontend library (SliderState, getSliderState, etc.)
  Copied from JUCE modules/juce_gui_extra/native/javascript/index.js
  for use in Specdrift WebView UI.
*/
import "./check_native_interop.js";

class PromiseHandler {
  constructor() {
    this.lastPromiseId = 0;
    this.promises = new Map();

    window.__JUCE__.backend.addEventListener(
      "__juce__complete",
      ({ promiseId, result }) => {
        if (this.promises.has(promiseId)) {
          this.promises.get(promiseId).resolve(result);
          this.promises.delete(promiseId);
        }
      }
    );
  }

  createPromise() {
    const promiseId = this.lastPromiseId++;
    const result = new Promise((resolve, reject) => {
      this.promises.set(promiseId, { resolve: resolve, reject: reject });
    });
    return [promiseId, result];
  }
}

const promiseHandler = new PromiseHandler();

function getNativeFunction(name) {
  if (!window.__JUCE__.initialisationData.__juce__functions.includes(name))
    console.warn(
      `Creating native function binding for '${name}', which is unknown to the backend`
    );

  const f = function () {
    const [promiseId, result] = promiseHandler.createPromise();

    window.__JUCE__.backend.emitEvent("__juce__invoke", {
      name: name,
      params: Array.prototype.slice.call(arguments),
      resultId: promiseId,
    });

    return result;
  };

  return f;
}

class ListenerList {
  constructor() {
    this.listeners = new Map();
    this.listenerId = 0;
  }

  addListener(fn) {
    const newListenerId = this.listenerId++;
    this.listeners.set(newListenerId, fn);
    return newListenerId;
  }

  removeListener(id) {
    if (this.listeners.has(id)) {
      this.listeners.delete(id);
    }
  }

  callListeners(payload) {
    for (const [, value] of this.listeners) {
      value(payload);
    }
  }
}

const BasicControl_valueChangedEventId = "valueChanged";
const BasicControl_propertiesChangedId = "propertiesChanged";
const SliderControl_sliderDragStartedEventId = "sliderDragStarted";
const SliderControl_sliderDragEndedEventId = "sliderDragEnded";

class SliderState {
  constructor(name) {
    if (!window.__JUCE__.initialisationData.__juce__sliders.includes(name))
      console.warn(
        "Creating SliderState for '" +
          name +
          "', which is unknown to the backend"
      );

    this.name = name;
    this.identifier = "__juce__slider" + this.name;
    this.scaledValue = 0;
    this.properties = {
      start: 0,
      end: 1,
      skew: 1,
      name: "",
      label: "",
      numSteps: 100,
      interval: 0,
      parameterIndex: -1,
    };
    this.valueChangedEvent = new ListenerList();
    this.propertiesChangedEvent = new ListenerList();

    window.__JUCE__.backend.addEventListener(this.identifier, (event) =>
      this.handleEvent(event)
    );

    window.__JUCE__.backend.emitEvent(this.identifier, {
      eventType: "requestInitialUpdate",
    });
  }

  setNormalisedValue(newValue) {
    this.scaledValue = this.snapToLegalValue(
      this.normalisedToScaledValue(newValue)
    );

    window.__JUCE__.backend.emitEvent(this.identifier, {
      eventType: BasicControl_valueChangedEventId,
      value: this.scaledValue,
    });
  }

  sliderDragStarted() {
    window.__JUCE__.backend.emitEvent(this.identifier, {
      eventType: SliderControl_sliderDragStartedEventId,
    });
  }

  sliderDragEnded() {
    window.__JUCE__.backend.emitEvent(this.identifier, {
      eventType: SliderControl_sliderDragEndedEventId,
    });
  }

  handleEvent(event) {
    if (event.eventType == BasicControl_valueChangedEventId) {
      this.scaledValue = event.value;
      this.valueChangedEvent.callListeners();
    }
    if (event.eventType == BasicControl_propertiesChangedId) {
      let { eventType: _, ...rest } = event;
      this.properties = rest;
      this.propertiesChangedEvent.callListeners();
    }
  }

  getScaledValue() {
    return this.scaledValue;
  }

  getNormalisedValue() {
    return Math.pow(
      (this.scaledValue - this.properties.start) /
        (this.properties.end - this.properties.start),
      this.properties.skew
    );
  }

  normalisedToScaledValue(normalisedValue) {
    return (
      Math.pow(normalisedValue, 1 / this.properties.skew) *
        (this.properties.end - this.properties.start) +
      this.properties.start
    );
  }

  snapToLegalValue(value) {
    const interval = this.properties.interval;

    if (interval == 0) return value;

    const start = this.properties.start;
    const clamp = (val, min = 0, max = 1) => Math.max(min, Math.min(max, val));

    return clamp(
      start + interval * Math.floor((value - start) / interval + 0.5),
      this.properties.start,
      this.properties.end
    );
  }
}

const sliderStates = new Map();

for (const sliderName of window.__JUCE__.initialisationData.__juce__sliders)
  sliderStates.set(sliderName, new SliderState(sliderName));

function getSliderState(name) {
  if (!sliderStates.has(name)) sliderStates.set(name, new SliderState(name));

  return sliderStates.get(name);
}

function getBackendResourceAddress(path) {
  const platform =
    window.__JUCE__.initialisationData.__juce__platform.length > 0
      ? window.__JUCE__.initialisationData.__juce__platform[0]
      : "";

  if (platform == "windows" || platform == "android")
    return "https://juce.backend/" + path;

  if (platform == "macos" || platform == "ios" || platform == "linux")
    return "juce://juce.backend/" + path;

  return path;
}

export {
  getNativeFunction,
  getSliderState,
  getBackendResourceAddress,
};
