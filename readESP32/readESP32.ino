void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  sendReadCommand();

  Serial.println("CLEARDATA");
  Serial.println("LABEL,TIME,ID,EC,SAL,DO,TEMP,PH,CHL,BAT");
}

void sendReadCommand(){
  LoRa.beginPacket();
  LoRa.print("spdata,C,1");
  LoRa.endPacket();
  }
  
void loop() {
  // put your main code here, to run repeatedly:
  int packetSize = LoRa.parsePacket();
  if (packetSize) {
    // read packet
    while (LoRa.available()) {
      sensorReading = LoRa.readString(); // Assemble new packet
    }
    //rssi = LoRa.packetRssi();
    sensorReading.trim();
    
  }
}
