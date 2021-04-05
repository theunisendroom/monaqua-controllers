#include <ESP8266WiFi.h>
#include <ESP8266TrueRandom.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include "DHT.h"

// Digital pin connected to the DHT sensor GPIO14 = D5
#define DHTPIN 14    
#define DHTTYPE DHT22

// GPIO where the DS18B20 water Temp Sensor is connected to D2 = GPIO4
const int oneWireBus = 4;  

//digital that the relay is connected to GPIO5 = D1
int relayPin = 5;

//light intensity pin GPIO4 = D2
int lightIntensityPin = 0;

//automated relay switch parameters
float MIN_TEMP = 24.00;
float MAX_TEMP = 28.00;

// Setup a oneWire instance to communicate with any OneWire devices
OneWire oneWire(oneWireBus);

// Pass our oneWire reference to Dallas Temperature sensor 
DallasTemperature sensors(&oneWire);

// DHT definition
DHT dht(DHTPIN, DHTTYPE);

//host to send information to
const char* host = "i-ernest.co.za";
const int hostPort = 80;
//char* host = "192.168.0.109";
//int hostPort = 9001;

//wifi to connect to
const char* ssid="Thyme Events";
const char* password = "pierre@home";
//char* ssid="Beerkuil";
//char* password = "LetMeInPlease";

//variables
int measurementIntervalInMiliSec = 60000; 

//indicates how long ago the last measurement happened (in millis since program started)
unsigned long lastMeasurementMillis = 0; 

WiFiServer server(80);

WiFiClient client;

void setup() {

  pinMode(DHTPIN,INPUT);
  pinMode (relayPin, OUTPUT);
  pinMode(lightIntensityPin, INPUT);

  Serial.begin(115200);
  
  connectToWifi();

  //switch off relay
  digitalWrite (relayPin, LOW);
  
  sensors.begin();
  dht.begin();

}

void loop() {

  unsigned long currentMillis = millis();
  if (currentMillis - lastMeasurementMillis >= measurementIntervalInMiliSec) {
    // save the last time you blinked the LED
    lastMeasurementMillis = currentMillis;
    takeAndSendMeasurements();
  }
  
  if(WiFi.status() != WL_CONNECTED){
    connectToWifi();
  }

  WiFiClient client = server.available();
  if (!client) {
    return;
  }

  Serial.println("Request Received");
  while(!client.available()){
    delay(1);
  }
  
  // Read the first line of the request
  String request = client.readStringUntil('\r');
  Serial.println(request);
  client.flush();

  hanldeRequest(request);
  
  client.stop();

}

void takeAndSendMeasurements(){
  // take water temperature
  sensors.requestTemperatures(); 
  float waterTempCelcius = readWaterTemp();

  if(waterTempCelcius < MIN_TEMP){
    digitalWrite(relayPin, HIGH);
  }else if(waterTempCelcius > MAX_TEMP){
    digitalWrite(relayPin, LOW);
  }

  float airTempCelcius = readAirTemp();

  float airHumidity = readAirHumidity();

  float airHeatIndex = calculateAirHeatIndex(airTempCelcius, airHumidity);

  int lightIntensity = readLightIntensity();

    //send to server
  sendToServer(waterTempCelcius, airTempCelcius, airHumidity, airHeatIndex, lightIntensity);
}

void sendToServer(float waterTemperature, float airTemperature, float humidity, float heatIndex, int lightIntensity) {

  String json = "{\"temperature\":\"" + String(waterTemperature) + "\", \"airTemperature\":\"" + String(airTemperature) + "\", \"humidity\":\"" + String(humidity) + "\", \"heatIndex\":\"" + String(heatIndex) + "\", \"lightIntensity\":\"" + String(lightIntensity) + "\"}";

  if(client.connect(host,hostPort)){

    client.println("POST / HTTP/1.1");
    client.println("Host: host");
    client.println("Connection: keep-alive");
    client.println("Cache-Control: no-cache");
    client.println("Content-Type: application/json");
    client.print("Content-Length: ");
    client.println(json.length());
    client.println();
    client.println(json);
    
    delay(10);
    // Read all the lines of the response and print them to Serial
    Serial.println("Response: ");
    while(client.available()){
      String line = client.readStringUntil('\r');
      Serial.print(line);
    }
  }
}

void connectToWifi(){
  WiFi.begin(ssid, password);

  Serial.println("");
  Serial.print("Connecting to WiFi.");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("Connected to the WiFi network");

  // Start the server
  server.begin();
  Serial.println("Server started");
  
  // Print the IP address
  Serial.print("Use this URL to connect: ");
  Serial.print("http://");
  Serial.print(WiFi.localIP());
  Serial.println("/");
}

void hanldeRequest(String request){
  // Match the request
  int value = LOW;
  if (request.indexOf("/water/on") != -1) {
    Serial.println("Switching ON relay!");
    digitalWrite(relayPin, HIGH);
    value = HIGH;
  } else if (request.indexOf("/water/off") != -1){
    Serial.println("Switching OFF relay!");
    digitalWrite(relayPin, LOW);
    value = LOW;
  } else {
    Serial.println("Unknown request received: " + request);
  }
}

float readWaterTemp(){
  float waterTemperature = sensors.getTempCByIndex(0);
  
  Serial.println();
  if (isnan(waterTemperature)) {
    Serial.println(F("Failed to read from water temperature sensor!"));
  }

  Serial.print(F("-  Water Temperature: "));
  Serial.println(waterTemperature); 
  Serial.println();

  return waterTemperature;
}

float readAirTemp(){
  float airTemperature = dht.readTemperature();

  Serial.println();
  if (isnan(airTemperature)) {
    Serial.println(F("Failed to read from DHT sensor for temperature!"));
  }

  Serial.print(F("-  Temperature: "));
  Serial.println(airTemperature); 
  Serial.println();

  return airTemperature;
}

float readAirHumidity(){
  float airHumidity = dht.readHumidity();

  Serial.println();
  if (isnan(airHumidity)) {
    Serial.println(F("Failed to read from DHT sensor for humidity!"));
  }

  Serial.print(F("-  Humidity: "));
  Serial.println(airHumidity); 
  Serial.println();
   
  return airHumidity;
  
}

float calculateAirHeatIndex(float temperature, float humidity){
  float heatIndex;

  Serial.println();
  if (isnan(temperature) || isnan(humidity)) {
    Serial.println(F("Can not calculate heat index. Temperature or humidity readings failed."));
    return heatIndex;
  }

  // Compute heat index in Celsius (isFahreheit = false)
  float heatIndexCelcius = dht.computeHeatIndex(temperature, humidity, false);

  Serial.print(F("-  Heat Index: "));
  Serial.println(heatIndexCelcius); 
  Serial.println();
   
  return heatIndexCelcius;
}

int readLightIntensity() {
  int lightIntensity = analogRead(lightIntensityPin);

  Serial.println();
  if (isnan(lightIntensity)) {
    Serial.println(F("Failed to read from light sensor for intensity!"));
  }
  
  Serial.print(F("-  Light Intensity: "));
  Serial.println(lightIntensity); 
  Serial.println();

  return lightIntensity;
 
}
