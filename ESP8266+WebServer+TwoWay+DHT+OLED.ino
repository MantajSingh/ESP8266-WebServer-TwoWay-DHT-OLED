#ifdef ESP32
  #include <WiFi.h>
  #include <AsyncTCP.h>
#else
  #include <ESP8266WiFi.h>
  #include <ESPAsyncTCP.h>
#endif
#include <ESPAsyncWebServer.h>

#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <Hash.h>
#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <Adafruit_Sensor.h>
#include <DHT.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>


//const char* ssid = "VM1254810"; //Write your SSID
//const char* password = "m3xkWkqFcytc";   //Write your password

const char* ssid = "CIK0950"; //Write your SSID
const char* password = "000ef4c61de0";   //Write your password
#define DHTPIN 2     // Digital pin D4 connected to the DHT sensor



// Uncomment the type of sensor in use:
#define DHTTYPE    DHT11     // DHT 11
//#define DHTTYPE    DHT22     // DHT 22 (AM2302)
//#define DHTTYPE    DHT21     // DHT 21 (AM2301)

DHT dht(DHTPIN, DHTTYPE);


#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels
#define OLED_ADDR 0x3C   // Address may be different for your module, also try with 0x3D if this doesn't work

// Declaration for an SSD1306 display connected to I2C (SDA, SCL pins)
#define OLED_RESET -1 // Reset pin # (or -1 if sharing Arduino reset pin)
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
//For more hardware details, visit https://iotforgeeks.com/interface-i2c-oled-display-ssd1306-with-nodemcu


// current temperature & humidity, updated in loop()
float t = 0.0;
float h = 0.0;
float s = 0.0;

const char* input_parameter = "state";

const int output = 14;
const int Push_button_GPIO = 13 ;

// Variables will change:
int LED_state = LOW;         
int button_state;             
int lastbutton_state = LOW;   

unsigned long lastDebounceTime = 0;  
unsigned long debounceDelay = 50;    

unsigned long previousMillis = 0;    // will store last time DHT was updated

// Updates DHT readings every 10 seconds
const long interval = 500;  

AsyncWebServer server(80);

const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML><html>
<head>
 
  <title>ESP Output Control Web Server</title>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  
  <style>
    html {font-family: Times New Roman; display: inline-block; text-align: center;}
    h2 {font-size: 3.0rem;}
    h4 {font-size: 2.0rem;}
    p {font-size: 3.0rem;}
    body {max-width: 900px; margin:0px auto; padding-bottom: 25px;}
    .switch {position: relative; display: inline-block; width: 120px; height: 68px} 
    .switch input {display: none}
    .slider {position: absolute; top: 0; left: 0; right: 0; bottom: 0; background-color: #FF0000; border-radius: 34px}
    .slider:before {position: absolute; content: ""; height: 52px; width: 52px; left: 8px; bottom: 8px; background-color: #fff; -webkit-transition: .4s; transition: .4s; border-radius: 68px}
    input:checked+.slider {background-color: #27c437}
    input:checked+.slider:before {-webkit-transform: translateX(52px); -ms-transform: translateX(52px); transform: translateX(52px)}
  </style>

  
</head>
<body>
  <h2>ESP RDS Server</h2>
  %BUTTONPLACEHOLDER%
  
<script>
function toggleCheckbox(element) {
  var xhr = new XMLHttpRequest();
  if(element.checked){ xhr.open("GET", "/update?state=1", true); }
  else { xhr.open("GET", "/update?state=0", true); }
  xhr.send();
}

setInterval(function ( ) {
  var xhttp = new XMLHttpRequest();
  xhttp.onreadystatechange = function() {
    if (this.readyState == 4 && this.status == 200) {
      var inputChecked;
      var outputStateM;
      if( this.responseText == 1){ 
        inputChecked = true;
        outputStateM = "ON";
      }
      else { 
        inputChecked = false;
        outputStateM = "OFF";
      }
      document.getElementById("output").checked = inputChecked;
      document.getElementById("outputState").innerHTML = outputStateM;
    }
  };
  xhttp.open("GET", "/state", true);
  xhttp.send();
}, 1000 ) ;
</script>



  <p>
    <i class="fas fa-thermometer-half" style="color:#059e8a;"></i> 
    <span class="dht-labels">Temperature</span> 
    <span id="temperature">%TEMPERATURE%</span>
    <sup class="units">!</sup>
  </p>

  <p>
   <i class="fas fa-tint" style="color:#00add6;"></i>
    <span class="dht-labels">Humidity</span> 
    <span id="humidity">%HUMIDITY%</span>
    <sup class="units">@</sup>
  </p>



   <p>
   
  <i class="fas fa-thermometer-half" style="color:#059e8a;"></i>
    <span class="dht-labels">Valve</span>
    <span id="str">%STR%</span>
    <sup class="units">%</sup>
  </p>





<script>
setInterval(function ( ) {
  var xhttp = new XMLHttpRequest();
  xhttp.onreadystatechange = function() {
    if (this.readyState == 4 && this.status == 200) {
      document.getElementById("temperature").innerHTML = this.responseText;
    }
  };
  xhttp.open("GET", "/temperature", true);
  xhttp.send();
}, 1000 ) ;



setInterval(function ( ) {
  var xhttp = new XMLHttpRequest();
  xhttp.onreadystatechange = function() {
    if (this.readyState == 4 && this.status == 200) {
      document.getElementById("humidity").innerHTML = this.responseText;
    }
  };
  xhttp.open("GET", "/humidity", true);
  xhttp.send();
}, 1000 ) ;



setInterval(function ( ) {
  var xhttp = new XMLHttpRequest();
  xhttp.onreadystatechange = function() {
    if (this.readyState == 4 && this.status == 200) {
      document.getElementById("str").innerHTML = this.responseText;
    }
  };
  xhttp.open("GET", "/str", true);
  xhttp.send();
}, 1000 ) ;


</script>
</body>
</html>
)rawliteral";

String processor(const String& var){
  if(var == "BUTTONPLACEHOLDER")
  {
    String buttons ="";
    String outputStateValue = outputState();
    buttons+= "<h4>Output State: <span id=\"outputState\"></span></h4><label class=\"switch\"><input type=\"checkbox\" onchange=\"toggleCheckbox(this)\" id=\"output\" " + outputStateValue + "><span class=\"slider\"></span></label>";
    return buttons;
  }

   if(var == "TEMPERATURE")
  {
   
    return String(t);
  }
  else if(var == "HUMIDITY")
  {
    return String(h);
  }
  
  return String();
}

String outputState(){
  if(digitalRead(output)){
    return "checked";
  }
  else {
    return "";
  }
  return "";
}

void setup(){
  // Serial port for debugging purposes
  Serial.begin(115200);

dht.begin();
  
  pinMode(output, OUTPUT);
  digitalWrite(output, LOW);
  pinMode(Push_button_GPIO, INPUT);
  
  // Connect to Wi-Fi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi..");
  }


  Serial.println("IP Address: ");
  Serial.println(WiFi.localIP());

display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDR);
  display.clearDisplay();           // Required to clear display
  display.display();
  
  display.setTextColor(WHITE);
  display.setTextSize(2); // Set text size
  display.print(" Connected   Broo  ");
  display.print(WiFi.localIP());
  display.display(); 

  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/html", index_html, processor);
  });


server.on("/temperature", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/plain", String(t).c_str());
  });
  
  server.on("/humidity", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/plain", String(h).c_str());
  });

  server.on("/str", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/plain", String(s).c_str());
  });

  
  server.on("/update", HTTP_GET, [] (AsyncWebServerRequest *request) {
    String input_message;
    String inputParameter;
    // GET input1 value on <ESP_IP>/update?state=<input_message>
    if (request->hasParam(input_parameter)) {
      input_message = request->getParam(input_parameter)->value();
      inputParameter = input_parameter;
      digitalWrite(output, input_message.toInt());
      LED_state = !LED_state;
    }
    else {
      input_message = "No message sent";
      inputParameter = "none";
    }
    Serial.println(input_message);
    request->send(200, "text/plain", "OK");
  });

  
  server.on("/state", HTTP_GET, [] (AsyncWebServerRequest *request) {
    request->send(200, "text/plain", String(digitalRead(output)).c_str());
  });
  // Start server
  server.begin();
}
  
void loop() {

   s=45 ;
  unsigned long currentMillis = millis();
  if (currentMillis - previousMillis >= interval) {
    // save the last time you updated the DHT values
    previousMillis = currentMillis;
    // Read temperature as Celsius (the default)
    float newT = dht.readTemperature();
    // Read temperature as Fahrenheit (isFahrenheit = true)
    //float newT = dht.readTemperature(true);
    // if temperature read failed, don't change t value
    if (isnan(newT)) {
      Serial.println("Failed to read from DHT sensor!");
    }
    else {
      t = newT;
      Serial.println(t);
    }
    // Read Humidity
    float newH = dht.readHumidity();
    // if humidity read failed, don't change h value 
    if (isnan(newH)) {
      Serial.println("Failed to read from DHT sensor!");
    }
    else {
      h = newH;
      Serial.println(h);
    }
  }
  int data = digitalRead(Push_button_GPIO);

  if (data != lastbutton_state) {
    // reset the debouncing timer
    lastDebounceTime = millis();
  }

  if ((millis() - lastDebounceTime) > debounceDelay) {
    if (data != button_state) {
      button_state = data;


      if (button_state == HIGH) {
        LED_state = !LED_state;
      }
    }
  }

  
  digitalWrite(output, LED_state);


  lastbutton_state = data;
}
