#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <FS.h>
#include <ESP8266WebServer.h>
#include <WiFiUdp.h> //for syslog+ntp
#include <ArduinoJson.h>
#include "config.h"
#include "syslog.h"
#include "ntp.h"

const char* VERSION = "0.96";

ESP8266WebServer server(80);

// pin IDs
const int relayOne  = D0;
const int relayTwo  = D1;
const int switchOne = D2;
const int switchTwo = D3;
const int wifiSwitch = D5;

int switchOneState = LOW;
int switchTwoState = LOW;

/**
 * Set up the execution environment
 */
void setup() {
  Serial.begin(115200);
  Serial.println();
  Serial.println("Booting garage version " + String(VERSION));
  Serial.print("Last restart cause: ");
  Serial.println(ESP.getResetReason());

  pinMode(relayOne, OUTPUT); // door one button
  pinMode(relayTwo, OUTPUT); // door two button
  pinMode(switchOne, INPUT_PULLUP); // door one switch
  pinMode(switchTwo, INPUT_PULLUP); // door two switch
  pinMode(wifiSwitch, INPUT_PULLUP); // wifi reset switch

  digitalWrite(relayOne, HIGH); // relay off
  digitalWrite(relayTwo, HIGH); // relay off

  switchOneState = digitalRead(switchOne);
  switchTwoState = digitalRead(switchTwo);

  SPIFFS.begin();

  // hardware reset button?
  if(digitalRead(wifiSwitch) == LOW) {
    delay(1000);
    if(digitalRead(wifiSwitch) == LOW) {
      // if the reset button is still LOW, reset the config
      SPIFFS.remove(CONFIG_FILE); // erase config file
      Serial.println(F("Reset button press detected, resetting to factory config"));
    }
  }

  // set default config, read or initialize saved config
  initConfig();
  wifiSetup();
  webServerSetup();
  syslogSetup();
  ntpSetup();

  if(MDNS.begin(webCfg.hostname.c_str())) {
    Serial.print(F("MDNS responder started for name: "));
    Serial.println(webCfg.hostname);
    MDNS.addService("http", "tcp", 80);
  }

  Serial.print(F("Open http://"));
  if(wifiCfg.ssid != "") {
    Serial.print(WiFi.localIP());
  } else {
    Serial.print(WiFi.softAPIP());
  }
  Serial.println(F("/ in your browser to see it working"));

}

/**
 * Closes the relay on the given pin number for 250ms, simulating a button press
 */
void activateDoor(int relay) {
  digitalWrite(relay, LOW);  // push
  delay(350);                // hold
  digitalWrite(relay, HIGH); // release
}

/**
 * Keep track of door open/closed states
 */
void monitorDoors() {
  int newState = digitalRead(switchOne);
  if(switchOneState != newState) {
    syslog("DOOR1", newState == HIGH ? "open" : "closed");
    switchOneState = newState;
    delay(250); // lazy debounce
  }
  newState = digitalRead(switchTwo);
  if(switchTwoState != newState) {
    syslog("DOOR2", newState == HIGH ? "open" : "closed");
    switchTwoState = newState;
    delay(250); // lazy debounce
  }
}


/**
 * This is the main program loop.  Watches for door state changes
 * and handles web requests.
 */
void loop() {
  server.handleClient();
  monitorDoors();
}

/**
 * Set up wifi according to saved/default configuration
 */
void wifiSetup() {
  // start with all wifi modes off
  WiFi.mode(WIFI_OFF);

  if(wifiCfg.ssid != String("")) {
    if(wifiCfg.password != String("")) {
      WiFi.begin(wifiCfg.ssid.c_str(), wifiCfg.password.c_str());
    } else{
      WiFi.begin(wifiCfg.ssid.c_str());
    }

    if(WiFi.waitForConnectResult() != WL_CONNECTED) {
      Serial.println("Wifi station connect failed.");
      WiFi.printDiag(Serial);

      Serial.print(F("Starting Access Point (AP) with SSID: "));
      Serial.println(wifiCfg.apSsid);
      if(!WiFi.softAP(wifiCfg.apSsid.c_str())) {
        Serial.println(F("Failed to start AP!"));
        WiFi.printDiag(Serial);
        delay(1000);
        ESP.restart();
      }
    }
  } else { // no station config, start AP
    Serial.print(F("Starting Access Point (AP) with SSID: "));
    Serial.println(wifiCfg.apSsid);
    if(!WiFi.softAP(wifiCfg.apSsid.c_str())) {
      Serial.println(F("Failed to start AP!"));
      WiFi.printDiag(Serial);
      delay(1000);
      ESP.restart();
    }
  }
  
  Serial.println();
  WiFi.printDiag(Serial);
}

    /**********************************
     ****** web server functions ******
     **********************************/

/**
 * wrapper to send prositive JSON based responses, save string memory
 */
void sendOkResponse(String json) {
  server.send(200, "application/json", json);
}
void sendOkResponse() {
  sendOkResponse("{ \"result\": \"success\" }");
}

/**
 * Needed in many places, a simple wrapper for auth checks
 */
bool isAuthenticatedRequest() {
  return server.authenticate(webCfg.username.c_str(), webCfg.password.c_str());
}

/**
 * Try to find applicable content type give a filename
 */
String getContentType(String filename){
  if(server.hasArg("download")) return "application/octet-stream";
  else if(filename.endsWith(".htm")) return "text/html";
  else if(filename.endsWith(".html")) return "text/html";
  else if(filename.endsWith(".css")) return "text/css";
  else if(filename.endsWith(".js")) return "application/javascript";
  else if(filename.endsWith(".json")) return "application/json";
  else if(filename.endsWith(".png")) return "image/png";
  else if(filename.endsWith(".gif")) return "image/gif";
  else if(filename.endsWith(".jpg")) return "image/jpeg";
  else if(filename.endsWith(".ico")) return "image/x-icon";
  else if(filename.endsWith(".xml")) return "text/xml";
  else if(filename.endsWith(".pdf")) return "application/x-pdf";
  else if(filename.endsWith(".zip")) return "application/x-zip";
  else if(filename.endsWith(".gz")) return "application/x-gzip";
  return "text/plain";
}

/**
 * Read a file from FS, or return false indicating failure
 */
bool handleFileRead(String path){
  if(path.equals("/") || path.equals("/index.html")) {
    server.sendHeader("Location", "/garage.html", true);
    server.send(302, "text/plain", "moved permanently");
  }
  if(path.endsWith("/")) path += "index.html"; // for subdirs
  Serial.printf("SERVER: Serving static file '%s'\n", path.c_str());
  String contentType = getContentType(path);
  String pathWithGz = path + ".gz";
  if(SPIFFS.exists(pathWithGz) || SPIFFS.exists(path)){
    if(SPIFFS.exists(pathWithGz))
      path += ".gz";
    File file = SPIFFS.open(path, "r");
    delay(100);
    server.sendHeader("Connection", "close");
    size_t sent = server.streamFile(file, contentType);
    file.close();
    Serial.printf("SERVER: Sent %d bytes.\n", sent);
    delay(100);
    return true;
  }
  return false;
}

/**** http request handlers ****/

/**
 * Return the current status of the door-open switches
 */
void doorStatus() {
  if(!isAuthenticatedRequest())
    return server.requestAuthentication();

  DynamicJsonBuffer jsonBuffer;
  JsonObject& json = jsonBuffer.createObject();

  json["doorOne"] = (switchOneState == HIGH) ? "open" : "closed";
  json["doorTwo"] = (switchTwoState == HIGH) ? "open" : "closed";

  String result;
  json.prettyPrintTo(result);

  sendOkResponse(result);
}

/**
 * Scan for wifi networks and return those found as JSON
 */
void wifiScan() {
  if(!isAuthenticatedRequest())
    return server.requestAuthentication();

  int n = WiFi.scanNetworks();
  DynamicJsonBuffer jsonBuffer;
  JsonArray& json = jsonBuffer.createArray();
  if (n != 0) {
    for (int i = 0; i < n; ++i) {
      JsonObject& wlan = json.createNestedObject();
      wlan["ssid"] = String(WiFi.SSID(i));
      wlan["rssi"] = WiFi.RSSI(i);
      wlan["secure"] = String((WiFi.encryptionType(i) == ENC_TYPE_NONE) ? "false" : "true");
    }
  }

  String result;
  json.prettyPrintTo(result);

  sendOkResponse(result);
}

/**
 * Activate door one (press door one's button).
 */
void activateDoorOne() {
  if(!isAuthenticatedRequest())
    return server.requestAuthentication();

  activateDoor(relayOne);
  syslog("DOOR1", "activated by web request");
  server.sendHeader("Location", "/");
  sendOkResponse();
}

/**
 * Activate door two (press door two's button).
 */
void activateDoorTwo() {
  if(!isAuthenticatedRequest())
    return server.requestAuthentication();

  activateDoor(relayTwo);
  syslog("DOOR2", "activated by web request");
  server.sendHeader("Location", "/");
  sendOkResponse();
}

/**
 * Clear stored config, reset controller
 */
void factoryReset() {
  if(!isAuthenticatedRequest())
    return server.requestAuthentication();

  SPIFFS.remove(CONFIG_FILE); // erase config file
  SPIFFS.end();
  ESP.eraseConfig(); // clear remembered wifi settings
  sendOkResponse();
  delay(100);
  ESP.reset();
}

/**
 * Restart (reset) the module
 */
void simpleRestart() {
  if(!isAuthenticatedRequest())
    return server.requestAuthentication();

  sendOkResponse();
  SPIFFS.end();
  delay(100);
  ESP.reset();
}

/**
 * Get the firmware version number
 */
void handleVersionRequest() {
  if(!isAuthenticatedRequest())
    return server.requestAuthentication();

  sendOkResponse("{ \"version\": \"" + String(VERSION) + "\" }");
}

/**
 * Get a copy of the stored config
 */
void handleConfigDownload() {
  if(!isAuthenticatedRequest())
    return server.requestAuthentication();

  if(!handleFileRead(CONFIG_FILE))
    server.send(404, "text/plain", "File Not Found");
}

/**
 * Handle post for configuration update
 */
void handleConfigUpdate() {
  if(!isAuthenticatedRequest())
    return server.requestAuthentication();

  if(server.hasArg("plain")) { // JSON is stored here
    String postData = server.arg("plain");
    DynamicJsonBuffer jsonBuffer;
    JsonObject& json = jsonBuffer.parseObject(postData);
    if(json.success()) {
      updateConfiguration(json);
      writeConfig();
      sendOkResponse();
    } else {
      server.send(400, "text/plain",
        "{ \"result\": \"error\","
        "  \"error\": \"Can't parse data (bad format or size)\" }");
    }
  } else {
    server.send(400, "text/plain",
      "{ \"result\": \"error\","
      "  \"error\": \"Bad request format\" }");
  }
}

/**
 * Handle update via file upload
 * 
 * arg: app = true for APP update (spiffs), false for FW update (sketch)
 */
void uploadFileHandler(boolean app = false) {
  if(!isAuthenticatedRequest())
    return; // don't allow upload if auth fails

  // don't process without Content-Length
  if(server.header("Content-Length") == "") {
    server.send(411, "text/plain", "Content length is required");
    return;
  }

  // get the file bytes and write them through the Update object
  HTTPUpload& upload = server.upload();

  if(upload.status == UPLOAD_FILE_START){

    Serial.setDebugOutput(true);

    WiFiUDP::stopAll();

    Serial.printf("Update type: %s\n", app ? "application" : "firmware");
    Serial.printf("Update file: %s\n", upload.filename.c_str());

    // Size here includes the HTTP/POST boundaries and such, so it's a little larger
    // than the actual file size.  That's OK for update purposes, it gives a buffer,
    // but it also means updates have to be smaller than actual free space.
    String cLenStr = server.header("Content-Length");
    long approxFileSize = cLenStr.toInt();

    if(app) { // filesystem update
      SPIFFS.end();

      if(!Update.begin(approxFileSize, U_SPIFFS)) {
        Serial.println("Update.begin() returned false");
        Update.printError(Serial);
      }
    } else { // firmware update
      uint32_t maxSketchSpace = (ESP.getFreeSketchSpace() - 0x1000) & 0xFFFFF000;

      if(approxFileSize > maxSketchSpace) { // too big
        server.send(413, "text/plain", "Update is too large");
        return;
      }
  
      if(!Update.begin(approxFileSize, U_FLASH)) {
        Update.printError(Serial);
      }
    }
  } else if(upload.status == UPLOAD_FILE_WRITE) {
    Serial.printf(".");

    if(Update.write(upload.buf, upload.currentSize) != upload.currentSize){
      Serial.println("Update.write() returned wrong size");
      Update.printError(Serial);
    }
  } else if(upload.status == UPLOAD_FILE_END) {
    if(Update.end(true)){ // true - because we're not using accurate sizes
      Serial.println("Update.end() returned true");
      if(app) {
        Serial.printf("Application update Success: %u\nRebooting...\n", upload.totalSize);
        if(SPIFFS.begin()) {
          writeConfig(); // write configuration to updated filesystem
          SPIFFS.end();
          delay(1);
        } else {
          Serial.println(F("Failed to re-mount SPIFFS FS, config will be lost..."));
        }
        delay(1000);
        ESP.reset();
      } else {
        Serial.printf("Firmware update Success: %u\nRebooting...\n", upload.totalSize);
        delay(1000);
        ESP.reset();
      }
    } else {
      Serial.println("Update.end() returned FALSE");
      Update.printError(Serial);
    }
    Serial.setDebugOutput(false);
  } else if(upload.status == UPLOAD_FILE_ABORTED) {
    Update.end();
    Serial.println("Update was aborted");
  }
  delay(0);
}

/**
 * Handle update when new firmware is sent via the HTTP Post Upload mechanism
 */
void firmwareUploadFileHandler() {
  uploadFileHandler(false);
}

/**
 * Handle update when new application is sent via the HTTP Post Upload mechanism
 */
void applicationUploadFileHandler() {
  uploadFileHandler(true);
}

/**
 * Handle the completion following a firmware/app update (complete the HTTP request)
 */
void uploadCompletionHandler() {
  if(!isAuthenticatedRequest()) // TODO: fix for AUTH/CONFIG
    return server.requestAuthentication();

  if(Update.hasError()) {
    server.send(500, "application/json", "{ \"result\": \"error\", \"error\": \"Upload error\" }");
  } else {
    sendOkResponse();
    ESP.restart();
  }
}

/**
 * Serve a document from the filesystem or 404 if not found
 */
void notFound() {
  if(!isAuthenticatedRequest())
    return server.requestAuthentication();

  if(!handleFileRead(server.uri()))
    server.send(404, "text/plain", "File Not Found");
}

/**
 * Set up the web server
 */
void webServerSetup() {
  // track content length header
  const char * headerkeys[] = { "Content-Length" };
  size_t headerkeyssize = sizeof(headerkeys) / sizeof(char*);
  server.collectHeaders(headerkeys, headerkeyssize);

  server.on("/api/doorStatus", HTTP_GET, doorStatus);
  server.on("/api/wifiScan",   HTTP_GET, wifiScan);

  server.on("/api/door1",   HTTP_POST, activateDoorOne);
  server.on("/api/door2",   HTTP_POST, activateDoorTwo);

  server.on("/api/freset",  HTTP_POST, factoryReset);
  server.on("/api/restart", HTTP_POST, simpleRestart);

  server.on("/api/version", HTTP_GET,  handleVersionRequest);

  server.on("/api/config",  HTTP_GET,  handleConfigDownload);
  server.on("/api/config",  HTTP_POST, handleConfigUpdate);

  server.on("/api/firmwareUpdate",    HTTP_POST,
    uploadCompletionHandler, firmwareUploadFileHandler);

  server.on("/api/applicationUpdate", HTTP_POST,
    uploadCompletionHandler, applicationUploadFileHandler);

  // will catch anything else and try to serve it from the FS
  server.onNotFound(notFound);

  server.begin();
}


