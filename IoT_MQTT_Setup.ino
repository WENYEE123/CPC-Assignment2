#include <PubSubClient.h>
#include <WiFi.h>
#include "DHT.h"
//#include <WiFiClientSecure.h>  // Include WiFiClientSecure for SSL/TLS support

// Sensor configuration
#define DHTTYPE DHT11
#define DHT_PIN 42           // DHT11 sensor pin
#define WATER_LEVEL_PIN 14   // Water level sensor pin
#define RAIN_SENSOR_PIN 4    // Rain sensor pin

// WiFi and MQTT configuration
const char* WIFI_SSID = "OPPO A74 5G";        // Your WiFi SSID
const char* WIFI_PASSWORD = "ezj2cdgj";       // Your WiFi password
const char* MQTT_SERVER = "35.193.17.193";    // Your MQTT Broker's public IP
const char* MQTT_TOPIC = "iot";                // MQTT topic for publishing
//const int MQTT_PORT = 8883;                    // MQTT port for TLS communication
const int MQTT_PORT = 1883;                   // MQTT port for non-TLS communication
// Initialize sensors and MQTT client
DHT dht(DHT_PIN, DHTTYPE);
//WiFiClientSecure espClient;  // Use WiFiClientSecure for SSL/TLS
WiFiClient espClient;
PubSubClient client(espClient);

char buffer[128]; // Buffer for MQTT messages

// Include the certificate as a string
/*const char* ca_cert = \
"-----BEGIN CERTIFICATE-----\n" \
"MIIDizCCAnMCFGYEt9AfglE0nBiCRueX17iyYqtmMA0GCSqGSIb3DQEBCwUAMIGB\n" \
"MQswCQYDVQQGEwJNWTERMA8GA1UECAwIU2VsYW5nb3IxEjAQBgNVBAcMCVNla2lu\n" \
"Y2hhbjEMMAoGA1UECgwDVVNNMQswCQYDVQQLDAJDUzEMMAoGA1UEAwwDV1dZMSIw\n" \
"IAYJKoZIhvcNAQkBFhNwdWFzYWl5ZW5AZ21haWwuY29tMB4XDTI1MDEyNTE1Mzkx\n" \
"NVoXDTI2MDEyNTE1MzkxNVowgYExCzAJBgNVBAYTAk1ZMREwDwYDVQQIDAhTZWxh\n" \
"bmdvcjESMBAGA1UEBwwJU2VraW5jaGFuMQwwCgYDVQQKDANVU00xCzAJBgNVBAsM\n" \
"AkNTMQwwCgYDVQQDDANXV1kxIjAgBgkqhkiG9w0BCQEWE3B1YXNhaXllbkBnbWFp\n" \
"bC5jb20wggEiMA0GCSqGSIb3DQEBAQUAA4IBDwAwggEKAoIBAQDdXoGDRQ30eNAC\n" \
"dl7FiV41XX3X1n3nM+Y8GqIGZUg1tBykbiP8/JyFpCnNeVBZSKc77nSmvDLMBMu9\n" \
"IFgzMLHbNTvAXS/jNbHzAkdXi/RIzc75hzZ8DuWou7ZuaFnPOTbWEd6z1N316BsA\n" \
"UgWnsH2w4cqe3r9/uJMOnDp72+5Cz4xzF2T78DzEgMCiS05ximbgrJBvr/QNaxBH\n" \
"ENLV74XLDs1nig3J9eQuxOAFvvaptRDvHwcytyFkyByv9cbjztYCHUtsJpd2kjGL\n" \
"wddoAwYmpk11B9VUpJPMQfF2XIWt23MB9aT0gXeh2tSB92oHPDRoCvBLq6B0YS14\n" \
"RhHmxUd3AgMBAAEwDQYJKoZIhvcNAQELBQADggEBAA0XBj+2ciJJwjFjToqrdzrW\n" \
"fM+egYTJU+ZKnqw2O/m1C9iQod8Uck+WEE/1ubfA5R2BJtDU1XGIQpcKSD9ZTsY9\n" \
"i9SR+3Eu/e48igvEsoYLVG+QBNZuI68AlmEglbDLV07o4SsqQweyc3OLCza0zzDd\n" \
"goj7Ao3tzqKEZJW9r6KgHbIU1USWDUnbpJYSN4mWLzgg1HkyOqUeLpuKc1nJojBd\n" \
"DbpoLiSHfurVcYeMPcUCGNIdsupgxC9cgB2TpYW/6DHnG+q1WtPSV0i2/U41f8jy\n" \
"soMCljrm2q8mtEnwUBp3MbNwBRYP6tabO8tHoQrNmYylaweNbJaTehHss2/97SU=\n" \
"-----END CERTIFICATE-----";*/

// Function to connect to WiFi
void setup_wifi() {
  delay(10);
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(WIFI_SSID);
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi connected");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
}

// Function to reconnect to MQTT server
void reconnect() {
  while (!client.connected()) {
    Serial.println("Attempting MQTT connection...");
    if (client.connect("ESP32Client")) { // Client ID for MQTT
      Serial.println("Connected to MQTT server");
    } else {
      Serial.print("Failed, rc=");
      Serial.print(client.state());
      Serial.println(" Retrying in 5 seconds...");
      delay(5000);
    }
  }
}

void setup() {
  Serial.begin(115200);      // Start serial communication
  dht.begin();               // Initialize DHT sensor
  pinMode(WATER_LEVEL_PIN, INPUT);  // Set water level sensor as input
  pinMode(RAIN_SENSOR_PIN, INPUT);  // Set rain sensor as input
  setup_wifi();              // Connect to WiFi
  client.setServer(MQTT_SERVER, MQTT_PORT); // Set MQTT server and port
}

void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  // Read temperature and humidity
  float temperature = dht.readTemperature();
  float humidity = dht.readHumidity();

  // Read water level sensor value
  int waterLevel = digitalRead(WATER_LEVEL_PIN); // Value from 0-4095 (12-bit ADC)

  // Read rain sensor value
  int rainValue = digitalRead(RAIN_SENSOR_PIN); // Assuming a digital output (0 or 1)

  // Prepare message
  String rainStatus = (rainValue == 1) ? "1" : "0";
  sprintf(buffer, "Temperature: %.2f°C, Humidity: %.2f%%, Water Level: %d, Rain Status: %s",
          temperature, humidity, waterLevel, rainStatus.c_str());

  // Publish data to MQTT topic
  client.publish(MQTT_TOPIC, buffer);
  Serial.println(buffer);

  delay(5000); // Delay for 5 seconds
}


