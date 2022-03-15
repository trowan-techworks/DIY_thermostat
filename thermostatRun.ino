#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include "DHT.h"
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define DHTPIN 4     // Digital pin connected to the DHT sensor
#define DHTTYPE DHT11   // DHT 11

#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 32 // OLED display height, in pixels
#define OLED_RESET     3 // Reset pin # (or -1 if sharing Arduino reset pin)
#define SCREEN_ADDRESS 0x3C ///< See datasheet for Address; 0x3D for 128x64, 0x3C for 128x32
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// REPLACE WITH YOUR NETWORK CREDENTIALS
const char* ssid = "YOUR SSID HERE";
const char* password = "YOUR PASSWORD HERE";

// Set your static IP address
IPAddress local_IP(192, 168, 1, 200);
// Set your Gateway IP address
IPAddress gateway(192, 168, 1, 1);
IPAddress subnet(255, 255, 0, 0);

// Default Threshold Temperature Value
String inputMessage = "70";
String lastTemperature;

// HTML web page to handle 2 input fields (threshold_input, enable_arm_input)
const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML><html><head>
  <title>Temperature Threshold Output Control</title>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <style>
    html {
      font-family: Arial, Helvetica, sans-serif;
      text-align: center;
    }
    h1 {
      font-size: 1.8rem;
      color: rgb(0, 0, 0);
    }
    h2{
      font-size: 1.5rem;
      font-weight: bold;
      color: #143642;
    }
    .topnav {
      overflow: hidden;
      background-color: #000000;
    }
    body {
      margin: 0;
    }
    .content {
      padding: 30px;
      max-width: 600px;
      margin: 0 auto;
     }
    </style>
  </head><body>
  <h2>DIY THERMOSTAT</h2> 
  <h3>Current Temp: %TEMPERATURE% &deg;F</h3>
  <form action="/get">
    <h3>Set Temp <input type="number" step="0.1" name="threshold_input" value="%THRESHOLD%" required><br></h3>
    <input type="submit" value="Submit">
  </form>
</body></html>)rawliteral";

void notFound(AsyncWebServerRequest *request) {
  request->send(404, "text/plain", "Not found");
}

// Start webserver on port 80
AsyncWebServer server(80);

// Replaces placeholder with DHT11 values
String processor(const String& var){
  //Serial.println(var);
  if(var == "TEMPERATURE"){
    return lastTemperature;
  }
  else if(var == "THRESHOLD"){
    return inputMessage;
  }
  return String();
}

const char* PARAM_INPUT_1 = "threshold_input";

// GPIO where the relay output is connected to
const int output = 5;

DHT dht(DHTPIN, DHTTYPE);

void setup() {
  Serial.begin(115200);
  if (!WiFi.config(local_IP, gateway, subnet)) {
    Serial.println("STA Failed to configure");
  }
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  if (WiFi.waitForConnectResult() != WL_CONNECTED) {
    Serial.println("WiFi Failed!");
    return;
  }
  Serial.println();
  Serial.print("ESP IP Address: http://");
  Serial.println(WiFi.localIP());
  
  // Set digital output for relay control
  pinMode(output, OUTPUT);
  digitalWrite(output, LOW);
  
  // Initialize and clear the display buffer
  if(!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    Serial.println(F("SSD1306 allocation failed"));
    for(;;); // Don't proceed, loop forever
  }
  display.clearDisplay();

  // Start the DHT11 sensor
 dht.begin();
  
  // Send web page to client
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/html", index_html, processor);
  });

  // Receive an HTTP GET request at <ESP_IP>/get?threshold_input=<inputMessage>&enable_arm_input=<inputMessage2>
  server.on("/get", HTTP_GET, [] (AsyncWebServerRequest *request) {
    // GET threshold_input value on <ESP_IP>/get?threshold_input=<inputMessage>
    if (request->hasParam(PARAM_INPUT_1)) {
      inputMessage = request->getParam(PARAM_INPUT_1)->value();
    }
    Serial.println(inputMessage);
    request->send(200, "text/html", "Set temp updated.<br><a href=\"/\">Return to Home Page</a>");
  });
  server.onNotFound(notFound);
  server.begin();
}

void loop() {
  delay(2000); 
  display.clearDisplay();
  int temperature = dht.readTemperature(true);
  lastTemperature = String(temperature);
  Serial.println(temperature);
  // Check if temperature is above threshold and if it needs to trigger output
  if(temperature > inputMessage.toInt()){
    String message = String("Temperature above threshold. Current temperature: ") + 
                          String(temperature) + String("F");
    Serial.println(message);
    digitalWrite(output, LOW);
    Serial.println("output low");
  }
    // Check if temperature is below threshold and if it needs to trigger output
  else if((temperature < inputMessage.toInt())) {
    String message = String("Temperature below threshold. Current temperature: ") + 
                          String(temperature) + String(" F");
    Serial.println(message);
    digitalWrite(output, HIGH);
    Serial.println("output high");
  }
  display.setTextColor(WHITE); 
  display.setTextSize(1);
  display.setCursor(1,25);
  display.print("Set ");
  display.print(inputMessage);
  display.setTextColor(WHITE); 
  display.setTextSize(4);
  display.setCursor(64,5);
  display.print(temperature);
  display.display();
}

