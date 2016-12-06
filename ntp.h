
// NTP stuff
long lastPoll = 0;
const int NTP_PACKET_SIZE = 48;
WiFiUDP ntp_;

void ntpSetup() {
  ntp_.begin(123);
}

/**
 * send an NTP request to the time server at the given address
 * (based on NTPClient.ino Arduino example)
 * 
 * Does nothing until the polling interval is has expired
 */
void sendNTPpacket(String server) {

  if((millis() * 1000) - lastPoll < ntpCfg.poll)
    return; // not time yet

  Serial.println("sending NTP packet...");

  byte packetBuffer[NTP_PACKET_SIZE];

  // set all bytes in the buffer to 0
  memset(packetBuffer, 0, NTP_PACKET_SIZE);

  packetBuffer[0] = 0b11100011; // LI, Version, Mode
  packetBuffer[1] = 0;          // Stratum, or type of clock
  packetBuffer[2] = 6;          // Polling Interval
  packetBuffer[3] = 0xEC;       // Peer Clock Precision

  // 8 bytes of zero for Root Delay & Root Dispersion

  packetBuffer[12]  = 49;
  packetBuffer[13]  = 0x4E;
  packetBuffer[14]  = 49;
  packetBuffer[15]  = 52;

  ntp_.beginPacket(server.c_str(), 123);
  ntp_.write(packetBuffer, NTP_PACKET_SIZE);
  ntp_.endPacket();

  lastPoll = millis();
}

/**
 * Read an NTP packet and set the time offset
 */
void readNTPPacket() {
  int len = ntp_.parsePacket();
  if (!len) return;

  Serial.print("NTP packet received, length=");
  Serial.println(len);

  byte packetBuffer[NTP_PACKET_SIZE];

  ntp_.read(packetBuffer, NTP_PACKET_SIZE);

  unsigned long highWord = word(packetBuffer[40], packetBuffer[41]);
  unsigned long lowWord  = word(packetBuffer[42], packetBuffer[43]);

  unsigned long secsSince1900 = highWord << 16 | lowWord;
  Serial.print("Seconds since Jan 1 1900 = " );
  Serial.println(secsSince1900);

  const unsigned long seventyYears = 2208988800UL;
  unsigned long epoch = secsSince1900 - seventyYears;

  Serial.print("Unix (epoch) time = ");
  Serial.println(epoch);
}

