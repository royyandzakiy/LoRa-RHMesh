// ----------------------
// PRE-REQUISITES
//#include "soc/soc.h"
//#include "soc/rtc_cntl_reg.h"

// === LoRa
#include <SPI.h>
#include <RHMesh.h>
#include <RH_RF95.h>
#include <EEPROM.h>

#define EEPROM_SIZE 1 // ESP32 max 512, Arduino Uno max 1024

/*// Arduino
#define RFM95_CS 10
#define RFM95_RST 9
#define RFM95_INT 2 //*/

// ESP32
#define RFM95_CS 5
#define RFM95_RST 14
#define RFM95_INT 2 //*/

// Change to 434.0 or other frequency, must match RX's freq!
#define RF95_FREQ 915.0

int nodeIdSelf;
 
// Singleton instance of the radio driver
RH_RF95 driver(RFM95_CS, RFM95_INT);

// Class to manage message delivery and receipt, using the driver declared above
RHMesh *manager;

String messageResponse = "Alert Received!";

// === WiFi
// #include <ESP8266WiFi.h> // use for ESP8266
#include <WiFi.h> // use for anything else

const char* ssid = "haluq_atas";
const char* password = "Jun54l54R0y";

WiFiClient thisWifiClient;

// === MQTT
#include <PubSubClient.h>

const char* mqtt_server = "192.168.5.185";
String topicPubAlert = "theSentinel/nodeHub/alert";
String topicPubRT = "theSentinel/nodeHub/routingTable";
String topicSub = "theSentinel/nodeHubDebug";
PubSubClient pubSubClient(thisWifiClient);

// ----------------------
// SETUP
// === LoRa
void setup_lora() {
  driver.setTxPower(23, false);
  driver.setFrequency(RF95_FREQ);
  driver.setCADTimeout(500);

  EEPROM.begin(EEPROM_SIZE); // gunakan untuk ESP32
  nodeIdSelf = EEPROM.read(0);

  manager = new RHMesh(driver, nodeIdSelf);
  
  if (!manager->init()) {
      Serial.println(F("init failed"));
  } else {
      Serial.println("Mesh Node \"" + (String) nodeIdSelf + "\" Up and Running!");
  }
}

// === WiFi
void setup_wifi() {
  delay(10);
  // We start by connecting to a WiFi network
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);

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

// === MQTT
void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println();

  // do something right after recieved
  // ...write code here
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
      // publish_mqtt(thisRoutingTable);
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

void setup_mqtt() {
  delay(10);
  pubSubClient.setServer(mqtt_server, 1883);
  pubSubClient.setCallback(callback);
}

// ----------------------
// ACTIONS
// === LoRa
uint8_t buf[RH_MESH_MAX_MESSAGE_LEN];

void listen_lora() {
  uint8_t len = sizeof(buf);
  uint8_t nodeIdFrom;
  
  if (manager->recvfromAck(buf, &len, &nodeIdFrom))
  {
    Serial.print("Got Message nodeIdFrom ");
    Serial.print(nodeIdFrom, HEX);
    Serial.println(":");
    Serial.println((char*)buf);

    // publish to MQTT server when receive LoRa message
    // check if message is an alert, or a routingTable
    // if (1) {
    // check if message is routingTable
      publish_mqtt((String) (char*) buf, topicPubRT);
    // } else if (1) {
    // check if message is alert
      publish_mqtt((String) (char*) buf, topicPubAlert);
    // }

    // Send a reply back to the originator client
    char messageResponseChar[messageResponse.length() + 1];
    strcpy(messageResponseChar, messageResponse.c_str());
    if (manager->sendtoWait((uint8_t*) messageResponseChar, sizeof((uint8_t*) messageResponseChar), nodeIdFrom) != RH_ROUTER_ERROR_NONE)
      Serial.println("sendtoWait failed");
  }
}

// === MQTT
void publish_mqtt(String _packetString, String _topicPub) {
  // converting String to char*
//  char msg[_packetString.length() + 1];
//  strcpy(msg, _packetString.c_str());
  
  pubSubClient.publish(_topicPub.c_str(), _packetString.c_str());
  Serial.println("published::" + (String) _packetString);
}

// ----------------------
// MAIN
void setup() {
    Serial.begin(9600);
    setup_lora();
    setup_wifi();
    setup_mqtt();
}

void loop()
{
  listen_lora();
  if (!pubSubClient.connected()) {
    reconnect();
  }
  pubSubClient.loop();
}
