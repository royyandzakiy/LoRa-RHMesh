// === WiFi
// #include <ESP8266WiFi.h> // use for ESP8266
#include <WiFi.h> // use for anything else
#include <PubSubClient.h>
#include "soc/soc.h"
#include "soc/rtc_cntl_reg.h"

const char* ssid = "haluq";
const char* password = "Jun54l54R0y";
const char* mqtt_server = "192.168.1.9";
String topicPub = "theSentinel/nodeHub";
String topicSub = "theSentinel/nodeHubDebug";

WiFiClient thisWifiClient;
PubSubClient pubSubClient(thisWifiClient);

// ----------------------
// === WiFi
void setup_wifi() {

  delay(10);
  // We start by connecting to a WiFi network
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);

//  Serial.println("wifi began");
//  while(1);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  randomSeed(micros());

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println();

  // do something after recieved message here
  // ...
}

void publish_packet(String _packetString) {
  char msg[_packetString.length() + 1];
  strcpy(msg, _packetString.c_str());
  
  pubSubClient.publish(topicPub.c_str(), msg);
  Serial.println("published::" + (String) msg);
}

void reconnect() {
  // Loop until we're reconnected
  while (!pubSubClient.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Create a pubSubClient ID
    String clientId = "NodeClient-";
    clientId += "x";
    // Attempt to connect
    if (pubSubClient.connect(clientId.c_str())) {
      Serial.println("connected");
      // Once connected, publish
      // publish_packet(thisRoutingTable);
      // ... and resubscribe
      pubSubClient.subscribe(topicSub.c_str());
    } else {
      Serial.print("failed, rc=");
      Serial.print(pubSubClient.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

// ----------------------
void setup() {
  Serial.begin(9600);
  while (!Serial);

  // WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0); //disable brownout detector

  // === WiFi Setup
  setup_wifi();
  pubSubClient.setServer(mqtt_server, 1883);
  pubSubClient.setCallback(callback);
}

void loop() { 
    // pastikan wifi terhubung
    if (!pubSubClient.connected()) {
      reconnect();
    }
    pubSubClient.loop();
}
