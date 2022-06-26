#include <ESP8266WiFi.h>       
#include <FirebaseArduino.h>  
  
#include <Arduino.h>
#include <LittleFS.h>     
#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <Wire.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClient.h>
#include <Arduino_JSON.h>
//#include <Arduino_JSON.h>

//#include <FirebaseESP8266.h>
#include <DHT.h>      


#define dht_pin D2                                            // the dht air temperature and humidity sensor is conntected to NodeMCU ESP8266 at digital pin D2
//#define DHTTYPE DHT11                                        // initialize dht type as DHT11
#define DHTTYPE DHT22                                        // initialize dht type as DHT22
DHT dht(dht_pin, DHTTYPE); 

#define soil_moisture_pin A0  // the capacitive soil moisture sensor is conntected to NodeMCU ESP8266 at analog pin A0

#define relay_input D4 // the relay is conntected to NodeMCU ESP8266 at digital pin D4  -> the relay is controlling the pump
String pump_state; // variable that keeps whether the pump is on or off to be written on the web server 

#define led_pin D1 // led conntected to nodemcu at digital pin D1

unsigned long previous_millis = 0;        // will store the last time sensor data was read (in milliseconds) 

const long interval = 60000;           // interval at which to read sensor data (in milliseconds) -> !!! 1000 milliseconds = 1 second

// the values given by the soil humidity sensor in special cases (for calibrating the sensor)
const float saturated_soil = 286;
const float water = 305;
const float dry_soil = 560;
const float air = 740;

const float dry_limit = 415; // we experimentally determined that above this limit the soil is starting to become too dry for the plant and it is in need of watering

//initialize the variables used for sensor readings as global variables
float air_humidity = 0.0;                                 // read air humidity from dht sensor
float air_temperature = 0.0;                              // read air temperature from dht sensor
float soil_moisture = 0.0;                                // read soil moisture from capacitive sensor
float calibrated_soil_moisture = 0.0;                     // used for mapping the raw soil moisture value in the interval 0-100%


#define wifi_ssid "CremeneD2.4"                                  
#define wifi_password "deliadiana99"    
#define FIREBASE_HOST "smartirrigation-6cf2c-default-rtdb.firebaseio.com"    // the project name address from firebase id   
#define FIREBASE_AUTH "Qa6z6muxh5y3ffQiQHybafMZ9HrUDQamlgBDKy1z"   // the secret key generated from firebase

String firebase_air_humidity;
String firebase_air_temperature;
String firebase_soil_moisture;

// define ntp client to get time
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org");
// variable to save the current epoch time
unsigned long epoch_time; 
// method that gets the current epoch time
unsigned long getTime() 
{
  timeClient.update();
  unsigned long now = timeClient.getEpochTime();
  return now;
}
// create asynchronous web server object on port 80
AsyncWebServer server(80);

// openweathermmap config
String open_weather_map_api_key = "2a909f3ec1f2afd999960839a3679641";
String city = "Cluj-Napoca";
String country_code = "RO";
String json_buffer_2;
String weather, description, icon, flower;
float temperature, pressure, humidity, wind_speed;

void connectToWiFi()
{
  Serial.print("Connecting to ");
  Serial.print(wifi_ssid);
  WiFi.begin(wifi_ssid, wifi_password);  
  while (WiFi.status() != WL_CONNECTED) 
  {
    Serial.print(".");
    delay(500);
  } 
}

// replaces placeholders with pump state value and real-time sensor readings
String update_placeholders(const String& var){
  if(var == "STATE"){
    if(digitalRead(relay_input)){
      pump_state = "OFF";
    }
    else{
      pump_state = "ON";
    }
    return pump_state;
  }
  else if (var == "TEMPERATURE"){
    return String(air_temperature);
  }
  else if (var == "HUMIDITY"){
    return String(air_humidity);
  }
  else if (var == "MOISTURE"){
    return String(calibrated_soil_moisture);
  } 
  // owa
  else if (var == "OWAWEATHER"){
    return weather;
  }
  else if (var == "OWADESCRIPTION"){
    return description;
  }
  else if (var == "OWATEMPERATURE"){
    return String(temperature);
  }
  else if (var == "OWAPRESSURE"){
    return String(pressure);
  }
  else if (var == "OWAHUMIDITY"){
    return String(humidity);
  }
  else if (var == "OWAWINDSPEED"){
    return String(wind_speed);
  } 
  else if (var == "OWAICON"){
    return icon;
  } 
  else if (var == "FLOWER"){
    return flower;
  }
}

// get data from OpenWeatherMap API
//String httpGETRequest(const char* server_name) {
//  HTTPClient http;   
//  // the ip address with path or domain name with url path 
//  http.begin(server_name);  
//  // send http request
//  int http_response_code = http.GET();
//  String payload = "{}";   
//  if (http_response_code>0) {
//    Serial.print("HTTP Response code: ");
//    Serial.println(http_response_code);
//    payload = http.getString();
//  }
//  else {
//    Serial.print("Error code: ");
//    Serial.println(http_response_code);
//  }
//  http.end();
//  return payload;
//}

void setup() 
{  
  Serial.begin(9600);

  pinMode(relay_input, OUTPUT); // set the digital pin of the relay as output
  pinMode(led_pin, OUTPUT); // set the digital pin of the led as output
  pinMode(dht_pin, INPUT); // set the dht pin as input
  pinMode(soil_moisture_pin, INPUT); // set the analog pin of the soil moisture sensor as input

  dht.begin();   //reads dht sensor data  
  
  // connecting the nodemcu to wireless
  connectToWiFi();                                 
  Serial.println("\nConnected");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());   // prints local IP address
  
  timeClient.begin();

  // connecting to firebase
  Firebase.begin(FIREBASE_HOST, FIREBASE_AUTH);  
  if (Firebase.failed()) 
  {
       Serial.print("Can't log to Firebase");
       Serial.println(Firebase.error()); 
       return;
  }
  else Serial.println("Connected to Firebase");

  //initialize LittleFS
  if(!LittleFS.begin()){
    Serial.println("An Error has occurred while mounting LittleFS");
    return;
  }
  else Serial.println("LittleFS works");

  // route for root web page
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request)
  {
    request->send(LittleFS, "/index.html", String(), false, update_placeholders);
  });
  
  // route to load style.css file
  server.on("/style.css", HTTP_GET, [](AsyncWebServerRequest *request)
  {
    request->send(LittleFS, "/style.css", "text/css");
  });

  // route to load script.js file
  server.on("/script.js", HTTP_GET, [](AsyncWebServerRequest *request)
  {
    request->send(LittleFS, "/script.js", "text/javascript");
  });

  // route to turn pump on
  server.on("/on", HTTP_GET, [](AsyncWebServerRequest *request)
  {
    digitalWrite(relay_input, LOW);    
    digitalWrite(led_pin, HIGH);
    request->send(LittleFS, "/index.html", String(), false, update_placeholders);
  });
  
  // route to turn pump off
  server.on("/off", HTTP_GET, [](AsyncWebServerRequest *request)
  {
    digitalWrite(relay_input, HIGH);   
    digitalWrite(led_pin, LOW); 
    request->send(LittleFS, "/index.html", String(), false, update_placeholders);
  });

  // updating real-time sensor readings
  server.on("/temperature", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/plain", String(air_temperature).c_str());
  });
  
  server.on("/humidity", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/plain", String(air_humidity).c_str());
  });
  
  server.on("/moisture", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/plain", String(calibrated_soil_moisture).c_str());
  });

  //owa
  server.on("/owaweather", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/plain", weather.c_str());
  });
  
  server.on("/owadescription", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/plain", description.c_str());
  });
  
  server.on("/owatemperature", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/plain", String(temperature).c_str());
  });
  
  server.on("/owapressure", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/plain", String(pressure).c_str());
  });
  
  server.on("/owahumidity", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/plain", String(humidity).c_str());
  });
  
  server.on("/owawindspeed", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/plain", String(wind_speed).c_str());
  });
  
  server.on("/owaicon", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/plain", icon.c_str());
  });

  server.on("/flower", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/plain", icon.c_str());
  });

  // route to load images
  server.on("/black1", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(LittleFS, "/black1.png", "image/png");
  });

  server.on("/black2", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(LittleFS, "/black2.png", "image/png");
  });

  server.on("/black3", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(LittleFS, "/black3.png", "image/png");
  });

  server.on("/black4", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(LittleFS, "/black4.png", "image/png");
  });
  
  // starting the server
  server.begin();
}
 
void loop() 
{
  unsigned long current_millis = millis();   // will store the current time to help with reading data from the sensors at a constant interval

  if (current_millis - previous_millis >= interval)
  {    
    previous_millis = current_millis; // save the last time you read data from the sensors
    
    air_humidity = dht.readHumidity();                                 // read air humidity
    air_temperature = dht.readTemperature();                           // read air temperature
    soil_moisture = analogRead(soil_moisture_pin);                     // read soil moisture
 
    int ok = 1;
    
    if (isnan(air_humidity) || isnan(air_temperature))                                     // checking if dht11 sensor is not working i.e. if the values are not numbers
    {                                   
      Serial.println(F("Failed to read from dht sensor!"));
      ok = 0;      
    } 
    if (soil_moisture < 200.00 | soil_moisture > 800)                                     // checking if the soil moisture is working i.e. 200 < value < 800
    {
      Serial.println(F("The soil moisture sensor is broken!"));
      ok = 0;
    }
    if (ok == 1) 
    {
      Serial.print("Humidity: ");  
      Serial.print(air_humidity);
      firebase_air_humidity = String(air_humidity) + String("%");                   // integer to string conversion for air humidity
      
      Serial.print("%  Temperature: ");  
      Serial.print(air_temperature);  
      Serial.print("°C ");
      firebase_air_temperature = String(air_temperature) + String("°C");                  // integer to string conversion for air temperature
         
      Serial.print(" Soil moisture raw: ");
      Serial.print(soil_moisture);
      firebase_soil_moisture = String(soil_moisture); 
      
      // map(value, fromLow, fromHigh, toLow, toHigh)
      // re-maps a number from one range to another -> we want to go from 286-560 raw values to 0-100%
      calibrated_soil_moisture = map(soil_moisture, dry_soil, saturated_soil, 0, 100);      
      Serial.print(" | calibrated: ");
      Serial.print(calibrated_soil_moisture);
      Serial.println("%");

      // get real-time weather data
//      String server_path = "http://api.openweathermap.org/data/2.5/weather?q=" + city + "," + country_code + "&APPID=" + open_weather_map_api_key; 
//      json_buffer_2 = httpGETRequest(server_path.c_str());
//      Serial.println(json_buffer_2);
//      JSONVar my_object = JSON.parse(json_buffer_2);
//      if (JSON.typeof(my_object) == "undefined") {
//        Serial.println("Parsing input failed!");
//        return;
//      }   
//      weather = my_object["weather"][0]["main"];
//      description = my_object["weather"][0]["description"];
//      icon = my_object["weather"][0]["icon"];
//      temperature = int(my_object["main"]["temp"])-273.15;    // transform kelvin degrees into celsius degrees
//      pressure = int(my_object["main"]["pressure"]);
//      humidity = int(my_object["main"]["humidity"]);
//      wind_speed = int(my_object["wind"]["speed"]);
//
//      Serial.print("JSON object = ");
//      Serial.println(my_object);
//      Serial.print("Weather: ");
//      Serial.println(weather);
//      Serial.print("Description: ");
//      Serial.println(description);
//      Serial.print("Icon: ");
//      Serial.println(icon);
//      Serial.print("Temperature: ");
//      Serial.println(temperature);
//      Serial.print("Pressure: ");
//      Serial.println(pressure);
//      Serial.print("Humidity: ");
//      Serial.println(humidity);
//      Serial.print("Wind Speed: ");
//      Serial.println(wind_speed);
      
      if (soil_moisture >= dry_limit) // if the soil is starting to dry we need to start the pump -> 2000 miliseconds = 2 seconds <=> the pump delivers 50 ml of water to the plant
      {
        digitalWrite(relay_input, LOW); // turn relay on = turn the pump on in the normally open circuit
        digitalWrite(led_pin, HIGH);
        delay(2000);
        digitalWrite(relay_input, HIGH); // turn relay off = turn the pump off        
        digitalWrite(led_pin, LOW);
        Serial.println("Watered the plants. The pump was turned ON for 2 seconds");
      }   

      // set a flower icon according to the soil moisture
      if (soil_moisture >= 415) flower = "black1";
      else if (soil_moisture >= 390 && soil_moisture < 415) flower = "black2";
      else if (soil_moisture >= 360 && soil_moisture < 390) flower = "black3";
      else if (soil_moisture < 360) flower = "black4";
   
      epoch_time = getTime();
      Serial.println(epoch_time);
      StaticJsonBuffer<256> json_buffer_1;
      JsonObject& root = json_buffer_1.createObject();
      root["timestamp"] = epoch_time;
      root["soil_moisture"] = calibrated_soil_moisture;
      root["air_humidity"] = air_humidity;
      root["air_temperature"] = air_temperature;
      Firebase.push("timestamped_measures", root);
               
      // check if the data could be written in the database
      if (Firebase.failed()) 
      {
        Serial.print("Pushing failed:");
        Serial.println(Firebase.error()); 
        return;
     }     
    }
 }  
}
