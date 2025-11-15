#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <AccelStepper.h>


const char *ssid = "Livebox6-9F30";
const char *password = "wifiabajo1";


AsyncWebServer server(80);


// Define stepper motor connections and steps per revolution
#define motorStep 14
#define motorDir 15
#define motorEnable 4
AccelStepper stepper(1, motorStep, motorDir);


void setup() {
  // Set up serial communication
  Serial.begin(115200);


  // Connect to Wi-Fi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }
  Serial.println("Connected to WiFi");


  // Wait for connection
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }


  Serial.println("");
  Serial.print("Connected to ");
  Serial.println(ssid);
  Serial.print("IP address: http://");
  Serial.println(WiFi.localIP());


  // Initialize stepper motor
  stepper.setMaxSpeed(1000.0);
  stepper.setAcceleration(500.0);


  // Define web server routes
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(200, "text/html", "<html><body><h1>Stepper Motor Control</h1><p><a href=\"/forward500\">Forward 500</a></p><p><a href=\"/backward500\">Backward 500</a></p><p><a href=\"/backward2000\">Backward 2000</a></p><p><a href=\"/forward2000\">Forward 2000</a></p></body></html>");
  });
  server.on("/forward500", HTTP_GET, [](AsyncWebServerRequest *request){
    Serial.print("/forward500");
    stepper.move(500);
    stepper.runToPosition();
    request->send(200, "text/html", "<html><body><h1>Stepper Motor Control</h1><p><a href=\"/forward500\">Forward 500</a></p><p><a href=\"/backward500\">Backward 500</a></p><p><a href=\"/backward2000\">Backward 2000</a></p><p><a href=\"/forward2000\">Forward 2000</a></p></body></html>");
  });
  server.on("/backward500", HTTP_GET, [](AsyncWebServerRequest *request){
    Serial.print("/backward500");
    stepper.move(-500);
    stepper.runToPosition();
    request->send(200, "text/html", "<html><body><h1>Stepper Motor Control</h1><p><a href=\"/forward500\">Forward 500</a></p><p><a href=\"/backward500\">Backward 500</a></p><p><a href=\"/backward2000\">Backward 2000</a></p><p><a href=\"/forward2000\">Forward 2000</a></p></body></html>");
  });
 server.on("/backward2000", HTTP_GET, [](AsyncWebServerRequest *request){
    Serial.print("/backward2000");
    stepper.move(-2000);
    stepper.runToPosition();
    request->send(200, "text/html", "<html><body><h1>Stepper Motor Control</h1><p><a href=\"/forward500\">Forward 500</a></p><p><a href=\"/backward500\">Backward 500</a></p><p><a href=\"/backward2000\">Backward 2000</a></p><p><a href=\"/forward2000\">Forward 2000</a></p></body></html>");
  });
 server.on("/forward2000", HTTP_GET, [](AsyncWebServerRequest *request){
    Serial.print("/forward2000");
    stepper.move(-2000);
    stepper.runToPosition();
    request->send(200, "text/html", "<html><body><h1>Stepper Motor Control</h1><p><a href=\"/forward500\">Forward 500</a></p><p><a href=\"/backward500\">Backward 500</a></p><p><a href=\"/backward2000\">Backward 2000</a></p><p><a href=\"/forward2000\">Forward 2000</a></p></body></html>");
  });


  // Start server
  server.begin();
}


void loop() {
  // Handle any necessary background tasks
  stepper.run();
}
