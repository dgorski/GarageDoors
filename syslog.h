/*
 * syslog stuff
 */

// udp socket
WiFiUDP syslog_;

void syslogSetup() {
  syslog_.begin(514);
}

/**
 * Log a message to a remote log server using the SYSLOG protocol
 */
void syslog(String proc, String msg, unsigned int sev = syslogCfg.severity, unsigned int fac = syslogCfg.facility) {
  Serial.println("[" + proc + "]: " + msg);

  String pri = String((syslogCfg.facility * 8) + syslogCfg.severity);

  String message = "<" + pri + ">" + webCfg.hostname + " " + proc + ": " + msg;

  unsigned int strLen = message.length();

  char packetBuffer[255];
  message.toCharArray(packetBuffer, 255);
  
  syslog_.beginPacket(syslogCfg.server.c_str(), 514);
  syslog_.write(packetBuffer, strLen + 1);
  syslog_.endPacket();
}

