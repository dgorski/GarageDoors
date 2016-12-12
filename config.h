
const char* CONFIG_FILE = "/configuration.json";

struct WIFICFG {
  bool startAp;
  String apSsid;
  String ssid;
  String password;
} wifiCfg;

struct WEBCFG {
  String hostname;
  String username;
  String password;
  unsigned int port;
} webCfg;

struct SYSLOGCFG {
  String server;
  unsigned int facility;
  unsigned int severity;
} syslogCfg;

struct NTPCFG {
  String server;
  unsigned int poll;
} ntpCfg;

/**
 * Update config structs from a JSON object
 * 
 * NOTE: There is a lot of checking because this
 * data can come from the network.
 */
void updateConfiguration(JsonObject& json) {
  /*
  Serial.println(F("CFGUPD: JSON: parsing JSON object:"));
  json.prettyPrintTo(Serial);
  Serial.println();
  */
  if(json.containsKey("wifi") && json["wifi"].is<JsonObject>()) {
    JsonObject& wifi = json["wifi"];
    /*
    Serial.println(F("CFGUPD: JSON: found WIFI object:"));
    wifi.printTo(Serial);
    Serial.println();
    */
    if(wifi.containsKey("startAp") && wifi["startAp"].is<bool>())
      wifiCfg.startAp = wifi["startAp"];
    if(wifi.containsKey("apSsid") && wifi["apSsid"].is<char*>())
      wifiCfg.apSsid = wifi["apSsid"].as<String>();
    if(wifi.containsKey("ssid") && wifi["ssid"].is<char*>())
      wifiCfg.ssid = wifi["ssid"].as<String>();
    if(wifi.containsKey("password") && wifi["password"].is<char*>())
      wifiCfg.password = wifi["password"].as<String>();
  }

  if(json.containsKey("web") && json["web"].is<JsonObject>()) {
    JsonObject& web = json["web"];
    /*
    Serial.println(F("CFGUPD: JSON: found WEB object:"));
    web.printTo(Serial);
    Serial.println();
    */
    if(web.containsKey("hostname") && web["hostname"].is<char*>())
      webCfg.hostname = web["hostname"].as<String>();
    if(web.containsKey("username") && web["username"].is<char*>())
      webCfg.username = web["username"].as<String>();
    if(web.containsKey("password") && web["password"].is<char*>())
      webCfg.password = web["password"].as<String>();
    if(web.containsKey("port") && web["port"].is<unsigned int>())
      webCfg.port = web["port"];
  }

  if(json.containsKey("syslog") && json["syslog"].is<JsonObject>()) {
    JsonObject& syslog = json["syslog"];
    /*
    Serial.println(F("CFGUPD: JSON: found SYSLOG object:"));
    syslog.printTo(Serial);
    Serial.println();
    */
    if(syslog.containsKey("server") && syslog["server"].is<char*>())
      syslogCfg.server = syslog["server"].as<String>();
    if(syslog.containsKey("facility") && syslog["facility"].is<unsigned int>())
      syslogCfg.facility = syslog["facility"];
    if(syslog.containsKey("severity") && syslog["severity"].is<unsigned int>())
      syslogCfg.severity = syslog["severity"];
  }
  
  if(json.containsKey("ntp") && json["ntp"].is<JsonObject>()) {
    JsonObject& ntp = json["ntp"];
    /*
    Serial.println(F("CFGUPD: JSON: found NTP object"));
    ntp.printTo(Serial);
    Serial.println();
    */
    if(ntp.containsKey("server") && ntp["server"].is<char*>())
      ntpCfg.server = ntp["server"].as<String>();
    if(ntp.containsKey("poll") && ntp["poll"].is<unsigned int>())
      ntpCfg.poll = ntp["poll"];
  }

}

/**
 * load config from flash
 */
bool readConfig() {
  if(!SPIFFS.exists(CONFIG_FILE)) {
    Serial.println("Configuration: file not found: " + String(CONFIG_FILE));
    return false;
  }

  File cfg = SPIFFS.open(CONFIG_FILE, "r");
  if(!cfg) {
    Serial.println("Configuration: open failed: " + String(CONFIG_FILE));
    return false;
  }

  size_t size = cfg.size();
  std::unique_ptr<char[]> buf(new char[size]);
  cfg.readBytes(buf.get(), size);
  cfg.close();

  DynamicJsonBuffer jsonBuffer;  
  JsonObject& json = jsonBuffer.parseObject(buf.get());

  if (!json.success()) {
    Serial.println("Configuration: parseObject() failed");
    return false;
  }
  /*
  Serial.println("Read JSON object:");
  json.prettyPrintTo(Serial);
  Serial.println();
  */
  updateConfiguration(json);
  return true;
}

/**
 * write the configuration to flash
 */
bool writeConfig() {

  DynamicJsonBuffer jsonBuffer;
  JsonObject& json = jsonBuffer.createObject();

  JsonObject& wifi = json.createNestedObject("wifi");
  wifi["startAp"] = wifiCfg.startAp;
  wifi["apSsid"] = wifiCfg.apSsid;
  wifi["ssid"] = wifiCfg.ssid;
  wifi["password"] = wifiCfg.password;

  JsonObject& web = json.createNestedObject("web");
  web["hostname"] = webCfg.hostname;
  web["username"] = webCfg.username;
  web["password"] = webCfg.password;
  web["port"] = webCfg.port;

  JsonObject& syslog = json.createNestedObject("syslog");
  syslog["server"] = syslogCfg.server;
  syslog["facility"] = syslogCfg.facility;
  syslog["severity"] = syslogCfg.severity;

  JsonObject& ntp = json.createNestedObject("ntp");
  ntp["server"] = ntpCfg.server;
  ntp["poll"] = ntpCfg.poll;

  /*
  Serial.println("New JSON object:");
  json.prettyPrintTo(Serial);
  Serial.println();
  */

  File cfg = SPIFFS.open(CONFIG_FILE, "w");
  if(!cfg) {
    Serial.print("Error opening config for write: " + String(CONFIG_FILE));
    return false;
  }

  json.prettyPrintTo(cfg);
  cfg.close();

  return true;
}

/**
 * init default configuration
 */
void initConfig() {

  // try to load saved config
  if(!readConfig()) {
    // otherwise, initialize defaults

    String apName = String("garage-") + String(ESP.getChipId(), HEX);

    // set defaults
    wifiCfg.startAp = true;
    wifiCfg.apSsid = apName;
    wifiCfg.ssid = "";
    wifiCfg.password = "";

    webCfg.hostname = "garage";
    webCfg.username = "admin";
    webCfg.password = "garage";
    webCfg.port = 80; // TODO: currently not used, web server needs
                      // the port number at constructor time

    syslogCfg.server = "";
    syslogCfg.facility = 20; // LOCAL4
    syslogCfg.severity = 5;  // NOTICE

    ntpCfg.server = "";
    ntpCfg.poll = 86400;

    writeConfig(); // write defaults to config file for next boot
  }
}


