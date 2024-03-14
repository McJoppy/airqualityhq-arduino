#include "PMS5003T.h"
#include "Arduino.h"

#if defined(ESP8266)
#include <SoftwareSerial.h>
/**
 * @brief Init sensor
 *
 * @param _debugStream Serial use for print debug log
 * @return true Success
 * @return false Failure
 */
bool PMS5003T::begin(Stream *_debugStream) {
  this->_debugStream = _debugStream;
  return this->begin();
}
#else

/**
 * @brief Init Sensor
 *
 * @param serial Serial communication with sensor
 * @return true Success
 * @return false Failure
 */
bool PMS5003T::begin(HardwareSerial &serial) {
  this->_serial = &serial;
  return this->begin();
}
#endif

/**
 * @brief Construct a new PMS5003T::PMS5003T object
 *
 * @param def Board type @ref BoardType
 */
PMS5003T::PMS5003T(BoardType def) : _boardDef(def) {}

/**
 * @brief Init sensor
 *
 * @return true Success
 * @return false Failure
 */
bool PMS5003T::begin(void) {
  if (this->_isBegin) {
    return true;
  }

#if defined(ESP32)
  // if (this->_serial != &Serial) {
  //   AgLog("Hardware serial must be Serial(0)");
  //   return false;
  // }
#endif

  this->bsp = getBoardDef(this->_boardDef);
  if (bsp == NULL) {
    AgLog("Board [%d] not supported", this->_boardDef);
    return false;
  }

  if (bsp->Pms5003.supported == false) {
    AgLog("Board [%d] PMS5003 not supported", this->_boardDef);
    return false;
  }

#if defined(ESP8266)
  bsp->Pms5003.uart_tx_pin;
  SoftwareSerial *uart =
      new SoftwareSerial(bsp->Pms5003.uart_tx_pin, bsp->Pms5003.uart_rx_pin);
  uart->begin(9600);
  if (pms.begin(uart) == false) {
    AgLog("PMS failed");
    return false;
  }
#else

#if ARDUINO_USB_CDC_ON_BOOT // Serial used for USB CDC
  if (this->_serial == &Serial0) {
    AgLog("Init Serial0");
    _serial->begin(9600, SERIAL_8N1);
#else
  if (this->_serial == &Serial) {
    AgLog("Init Serial");
    this->_serial->begin(9600, SERIAL_8N1, bsp->Pms5003.uart_rx_pin,
                         bsp->Pms5003.uart_tx_pin);
#endif
  } else {
    /** Share with sensor air s8*/
    if (bsp->SenseAirS8.supported == false) {
      AgLog("Board [%d] PMS5003T_2 not supported", this->_boardDef);
      return false;
    }

    AgLog("Init Serialx");
    this->_serial->begin(9600, SERIAL_8N1, bsp->SenseAirS8.uart_rx_pin,
                         bsp->SenseAirS8.uart_tx_pin);
  }
  if (pms.begin(this->_serial) == false) {
    AgLog("PMS failed");
    return false;
  }
#endif

  this->_isBegin = true;
  return true;
}

/**
 * @brief Read PM1.0 must call this function after @ref readData success
 *
 * @return int PM1.0 index
 */
int PMS5003T::getPm01Ae(void) { return pms.getPM0_1(); }

/**
 * @brief Read PM2.5 must call this function after @ref readData success
 *
 * @return int PM2.5 index
 */
int PMS5003T::getPm25Ae(void) { return pms.getPM2_5(); }

/**
 * @brief Read PM10.0 must call this function after @ref readData success
 *
 * @return int PM10.0 index
 */
int PMS5003T::getPm10Ae(void) { return pms.getPM10(); }

/**
 * @brief Read PM 0.3 Count must call this function after @ref readData success
 *
 * @return int PM 0.3 Count index
 */
int PMS5003T::getPm03ParticleCount(void) { return pms.getCount0_3(); }

/**
 * @brief Convert PM2.5 to US AQI
 *
 * @param pm25 PM2.5 index
 * @return int PM2.5 US AQI
 */
int PMS5003T::convertPm25ToUsAqi(int pm25) { return pms.pm25ToAQI(pm25); }

/**
 * @brief Get temperature, Must call this method after @ref readData() success
 *
 * @return float Degree Celcius
 */
float PMS5003T::getTemperature(void) {
  float temp = pms.getTemp();
  return correctionTemperature(temp / 10.0f);
}

/**
 * @brief Get humidity, Must call this method after  @ref readData() success
 *
 * @return float Percent (%)
 */
float PMS5003T::getRelativeHumidity(void) {
  float hum = pms.getHum();
  return correctionRelativeHumidity(hum / 10.0f);
}

/**
 * @brief Check device initialized or not
 *
 * @return true Initialized
 * @return false No-initialized
 */
bool PMS5003T::isBegin(void) {
  if (this->_isBegin == false) {
    AgLog("Not-initialized");
    return false;
  }
  return true;
}

float PMS5003T::correctionTemperature(float inTemp) {
  if (inTemp < 10.0f) {
    return inTemp * 1.327f - 6.738f;
  }
  return inTemp * 1.181f - 5.113f;
}

void PMS5003T::end(void) {
  if (_isBegin == false) {
    return;
  }
  _isBegin = false;
#if defined(ESP8266)
  _debugStream = NULL;
#else
  delete _serial;
#endif
  AgLog("De-initialize");
}

/**
 * @brief Read PMS on loop
 *
 */
void PMS5003T::handle(void) { pms.handle(); }

/**
 * @brief Get PMS status
 * @return true Failed
 * @return false No Problem
 */
bool PMS5003T::isFailed(void) { return pms.isFailed(); }

float PMS5003T::correctionRelativeHumidity(float inHum) {
  float hum = inHum * 1.259 + 7.34;
  if (hum > 100.0f) {
    hum = 100.0f;
  }
  return hum;
}
