#include <ESP8266WiFi.h>
#include <ESP8266TrueRandom.h>
#include <OneWire.h>
#include <DallasTemperature.h>

// GPIO where the DS18B20 is connected to
const int oneWireBus = 4;     

// Setup a oneWire instance to communicate with any OneWire devices
OneWire oneWire(oneWireBus);

// Pass our oneWire reference to Dallas Temperature sensor 
DallasTemperature sensors(&oneWire);

//light to switch on after wifi connect
int ledPin = LED_BUILTIN;

//host to send information to
const char* host = "192.168.0.109";
const int hostPort = 9001;

//wifi to connect to
const char* ssid="Beerkuil";
const char* password = "LetMeInPlease";

WiFiClient client;

void setup() {
  // basic setup
  pinMode(ledPin,OUTPUT);
  digitalWrite(ledPin,HIGH);

  Serial.begin(115200);
  Serial.println();
  Serial.print("Wifi connecting to ");
  Serial.println( ssid );

  // connect to wifi
  WiFi.begin(ssid,password);

  Serial.println();
  Serial.print("Connecting");

  while( WiFi.status() != WL_CONNECTED ){
      delay(500);
      Serial.print(".");        
  }
  

  digitalWrite( ledPin , LOW);
  Serial.println();

  Serial.println("Wifi Connected Success!");
  Serial.print("NodeMCU IP Address : ");
  Serial.println(WiFi.localIP() );

  sensors.begin();
}

void loop() {
  // put your main code here, to run repeatedly:
  sensors.requestTemperatures(); 
  float temperatureC = sensors.getTempCByIndex(0);
  sendTemperatureToServer(temperatureC);
  
  delay(5000);

}

void sendTemperatureToServer(float temperature) {

  String json = "{\"temperature\":\"" + String(temperature) + "\"}";

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
