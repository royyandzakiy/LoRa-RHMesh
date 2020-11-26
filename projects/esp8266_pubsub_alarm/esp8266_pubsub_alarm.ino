#include <ESP8266WiFi.h>
#include <PubSubClient.h>

// Update these with values suitable for your network.

const char* ssid = "haluq_atas";
const char* password = "Jun54l54R0y";
const char* mqtt_server = "192.168.5.185";

WiFiClient espClient;
PubSubClient client(espClient);
unsigned long lastMsg = 0;
#define MSG_BUFFER_SIZE	(50)
char msg[MSG_BUFFER_SIZE];
int value = 0;

#define LED_RED 12
#define LED_GREEN 14
#define BUZZER 13

// -------------
// SETUP
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

void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println();

  // triggers the alarm mechanism
   alarm_mechanism();
}

void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Create a random client ID
    String clientId = "ESP8266Client-";
    clientId += String(random(0xffff), HEX);
    // Attempt to connect
    if (client.connect(clientId.c_str())) {
      Serial.println("connected");
      // Once connected, publish an announcement...
      client.publish("theSentinel/nodeHubDebug", "Notification Listener, up and running!");
      // ... and resubscribe
      client.subscribe("theSentinel/nodeHub/alert");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

void setup_LED() {
  pinMode(LED_GREEN, OUTPUT);
  pinMode(LED_RED, OUTPUT);
  pinMode(BUZZER, OUTPUT);
}

void setup_mqtt() {
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);
}

// -----------
// ACTION
void alarm_mechanism() {
  digitalWrite(LED_GREEN, LOW);
  for(int i=0; i<2; i++) {
    digitalWrite(LED_RED, HIGH);
    digitalWrite(BUZZER, HIGH);
    delay(200);
    digitalWrite(LED_RED, LOW);
    digitalWrite(BUZZER, LOW);
    delay(200);
  }
}

// ------------
void setup() {
  Serial.begin(9600);
  setup_LED();
  setup_wifi();  
  setup_mqtt();
}

void loop() {
  digitalWrite(LED_GREEN, HIGH);
  if (!client.connected()) {
    reconnect();
  }
  client.loop();
}
