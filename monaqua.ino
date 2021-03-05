#include <ESP8266WiFi.h>
#include <ESP8266TrueRandom.h>

const char* ssid="Beerkuil";
const char* password = "LetMeInPlease";

int ledPin = LED_BUILTIN;

const char* host = "192.168.0.114";
const int hostPort = 9001;

WiFiClient client;

/* Here we have a few constants that make editing the code easier. I will go
   through them one by one.

   A reading from the ADC might give one value at one sample and then a little
   different the next time around. To eliminate noisy readings, we can sample
   the ADC pin a few times and then average the samples to get something more 
   solid. This constant is utilized in the readThermistor function. 
   */
const int    SAMPLE_NUMBER      = 100;

/* In order to use the Beta equation, we must know our other resistor
   within our resistor divider. If you are using something with large tolerance,
   like at 5% or even 1%, measure it and place your result here in ohms. */
const double BALANCE_RESISTOR   = 10000.0;

// This helps calculate the thermistor's resistance (check article for details).
const double MAX_ADC            = 1023.0;

/* This is thermistor dependent and it should be in the datasheet, or refer to the
   article for how to calculate it using the Beta equation.
   I had to do this, but I would try to get a thermistor with a known
   beta if you want to avoid empirical calculations. */
const double BETA               = 3974.0;

/* This is also needed for the conversion equation as "typical" room temperature
   is needed as an input. */
const double ROOM_TEMP          = 298.15;   // room temperature in Kelvin

/* Thermistors will have a typical resistance at room temperature so write this 
   down here. Again, needed for conversion equations. */
const double RESISTOR_ROOM_TEMP = 100000.0;

//===============================================================================
//  Variables
//===============================================================================
// Here is where we will save the current temperature
double currentTemperature = 0;

//===============================================================================
//  Pin Declarations
//===============================================================================
//Inputs:
int thermistorPin = 0;  // Where the ADC samples the resistor divider's output

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
}

void loop() {
  // put your main code here, to run repeatedly:
  
  sendTemperatureToServer(readThermistor());
  
  delay(5000);

}

void sendTemperatureToServer(double temperature) {

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

//===============================================================================
//  Functions
//===============================================================================
/////////////////////////////
////// readThermistor ///////
/////////////////////////////
/*
This function reads the analog pin as shown below. Converts voltage signal
to a digital representation with analog to digital conversion. However, this is
done multiple times so that we can average it to eliminate measurement errors.
This averaged number is then used to calculate the resistance of the thermistor. 
After this, the resistance is used to calculate the temperature of the 
thermistor. Finally, the temperature is converted to celsius. Please refer to
the allaboutcircuits.com article for the specifics and general theory of this
process.

Quick Schematic in case you are too lazy to look at the site :P

          (Ground) ----\/\/\/-------|-------\/\/\/---- V_supply
                     R_balance      |     R_thermistor
                                    |
                               Analog Pin
*/

double readThermistor() 
{
  // variables that live in this function
  double rThermistor = 0;            // Holds thermistor resistance value
  double tKelvin     = 0;            // Holds calculated temperature
  double tCelsius    = 0;            // Hold temperature in celsius
  double adcAverage  = 0;            // Holds the average voltage measurement
  int    adcSamples[SAMPLE_NUMBER];  // Array to hold each voltage measurement

  /* Calculate thermistor's average resistance:
     As mentioned in the top of the code, we will sample the ADC pin a few times
     to get a bunch of samples. A slight delay is added to properly have the
     analogRead function sample properly */
  
  for (int i = 0; i < SAMPLE_NUMBER; i++) 
  {
    adcSamples[i] = analogRead(thermistorPin);  // read from pin and store
    delay(10);        // wait 10 milliseconds
  }

  /* Then, we will simply average all of those samples up for a "stiffer"
     measurement. */
  for (int i = 0; i < SAMPLE_NUMBER; i++) 
  {
    adcAverage += adcSamples[i];      // add all samples up . . .
  }
  adcAverage /= SAMPLE_NUMBER;        // . . . average it w/ divide

  /* Here we calculate the thermistor’s resistance using the equation 
     discussed in the article. */
  rThermistor = BALANCE_RESISTOR * ( (MAX_ADC / adcAverage) - 1);

  /* Here is where the Beta equation is used, but it is different
     from what the article describes. Don't worry! It has been rearranged
     algebraically to give a "better" looking formula. I encourage you
     to try to manipulate the equation from the article yourself to get
     better at algebra. And if not, just use what is shown here and take it
     for granted or input the formula directly from the article, exactly
     as it is shown. Either way will work! */
  tKelvin = (BETA * ROOM_TEMP) / 
            (BETA + (ROOM_TEMP * log(rThermistor / RESISTOR_ROOM_TEMP)));

  /* I will use the units of Celsius to indicate temperature. I did this
     just so I can see the typical room temperature, which is 25 degrees
     Celsius, when I first try the program out. I prefer Fahrenheit, but
     I leave it up to you to either change this function, or create
     another function which converts between the two units. */
  tCelsius = tKelvin - 273.15;  // convert kelvin to celsius 

  return tCelsius;    // Return the temperature in Celsius
}
