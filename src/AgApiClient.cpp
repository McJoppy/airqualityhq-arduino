#include "AgApiClient.h"
#include "AgConfigure.h"
#include "AirGradient.h"
#ifdef ESP8266
#include <ESP8266HTTPClient.h>
#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#else
#include <HTTPClient.h>
#endif

AgApiClient::AgApiClient(Stream &debug, AgConfigure &config)
    : PrintLog(debug, "ApiClient"), config(config) {}

AgApiClient::~AgApiClient() {}

/**
 * @brief Initialize the API client
 *
 */
void AgApiClient::begin(void) {
  getConfigFailed = false;
  postToServerFailed = false;
  logInfo("begin");
}

/**
 * @brief Get configuration from AirGradient cloud
 *
 * @param deviceId Device ID
 * @return true Success
 * @return false Failure
 */
bool AgApiClient::fetchServerConfiguration(void) {
  if (config.getConfigurationControl() ==
      ConfigurationControl::ConfigurationControlLocal) {
    logWarning("Ignore fetch server configuration");

    // Clear server configuration failed flag, cause it's ignore but not
    // really failed
    getConfigFailed = false;
    return false;
  }

  String uri =
      "http://hw.airgradient.com/sensors/airgradient:" + ag->deviceId() +
      "/one/config";

  /** Init http client */
#ifdef ESP8266
  HTTPClient client;
  WiFiClient wifiClient;
  if (client.begin(wifiClient, uri) == false) {
#else
  HTTPClient client;
  if (client.begin(uri) == false) {
#endif
    getConfigFailed = true;
    return false;
  }

  /** Get data */
  int retCode = client.GET();
  if (retCode != 200) {
    client.end();
    getConfigFailed = true;
    return false;
  }

  /** clear failed */
  getConfigFailed = false;

  /** Get response string */
  String respContent = client.getString();
  client.end();

  // logInfo("Get configuration: " + respContent);

  /** Parse configuration and return result */
  return config.parse(respContent, false);
}

/**
 * @brief Post data to AirGradient cloud
 *
 * @param deviceId Device Id
 * @param data String JSON
 * @return true Success
 * @return false Failure
 */
bool AgApiClient::postToServer(String data) {
  if (config.isPostDataToAirGradient() == false) {
    logWarning("Ignore post data to server");
    return true;
  }

  if (WiFi.isConnected() == false) {
    return false;
  }

  String uri =
      "http://hw.airgradient.com/sensors/airgradient:" + ag->deviceId() +
      "/measures";
  logInfo("Post uri: " + uri);
  logInfo("Post data: " + data);

  WiFiClient wifiClient;
  HTTPClient client;
  if (client.begin(wifiClient, uri.c_str()) == false) {
    return false;
  }
  client.addHeader("content-type", "application/json");
  int retCode = client.POST(data);
  client.end();

  if ((retCode == 200) || (retCode == 429)) {
    postToServerFailed = false;
    return true;
  } else {
    logError("Post response failed code: " + String(retCode));
  }
  postToServerFailed = true;
  return false;
}

/**
 * @brief Get failed status when get configuration from AirGradient cloud
 *
 * @return true Success
 * @return false Failure
 */
bool AgApiClient::isFetchConfigureFailed(void) { return getConfigFailed; }

/**
 * @brief Get failed status when post data to AirGradient cloud
 *
 * @return true Success
 * @return false Failure
 */
bool AgApiClient::isPostToServerFailed(void) { return postToServerFailed; }

void AgApiClient::setAirGradient(AirGradient *ag) { this->ag = ag; }
