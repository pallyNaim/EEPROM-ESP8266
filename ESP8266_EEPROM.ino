#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <EEPROM.h>
#include <Servo.h>

#define EEPROM_SIZE 512

#define SERVO_PIN D5 // Use D5 (GPIO14) which supports PWM

struct Config {
  char ssid[32];
  char password[32];
  char deviceID[32];
  bool outputStatus;
};

Config config;
ESP8266WebServer server(80);
Servo myServo;
bool servoStatus = false; // Global flag to track servo status

void setup() {
  Serial.begin(115200);
  EEPROM.begin(EEPROM_SIZE);

  myServo.attach(SERVO_PIN);
  myServo.write(0); // Initialize servo to 0 degrees

  // Load configuration from EEPROM
  loadConfig();

  // Check if WiFi credentials are available
  if (strlen(config.ssid) == 0 || strlen(config.password) == 0) {
    startAPMode();
  } else {
    connectToWiFi();
  }
}

void loop() {
  server.handleClient();
  if (servoStatus) {
    myServo.write(180); // Turn servo to 180 degrees
    delay(1000); // Wait for 1 second
    myServo.write(0); // Turn servo to 0 degrees
    delay(1000); // Wait for 1 second
  }
}

void startAPMode() {
  WiFi.mode(WIFI_AP);
  WiFi.softAP("ESP8266_Config");

  IPAddress IP = WiFi.softAPIP();
  Serial.print("AP IP address: ");
  Serial.println(IP);

  server.on("/", HTTP_GET, []() {
    server.send(200, "text/html", getHTML());
  });

  server.on("/save", HTTP_POST, []() {
    if (server.hasArg("ssid") && server.hasArg("password") && server.hasArg("deviceID") && server.hasArg("outputStatus")) {
      strcpy(config.ssid, server.arg("ssid").c_str());
      strcpy(config.password, server.arg("password").c_str());
      strcpy(config.deviceID, server.arg("deviceID").c_str());
      config.outputStatus = server.arg("outputStatus") == "on";

      saveConfig();
      server.send(200, "text/html", "Configuration saved. Restarting...");
      delay(1000);
      ESP.restart();
    } else {
      server.send(400, "text/html", "Invalid input.");
    }
  });

  server.begin();
}

String getHTML() {
  return "<html><body>"
         "<form action='/save' method='POST'>"
         "SSID: <input type='text' name='ssid'><br>"
         "Password: <input type='text' name='password'><br>"
         "Device ID: <input type='text' name='deviceID'><br>"
         "Output Status: <input type='radio' name='outputStatus' value='on'> On"
         "<input type='radio' name='outputStatus' value='off'> Off<br>"
         "<input type='submit' value='Save'>"
         "</form>"
         "</body></html>";
}

void loadConfig() {
  EEPROM.get(0, config);
  if (isValidConfig(config)) {
    Serial.println("Loaded configuration from EEPROM.");
  } else {
    Serial.println("Invalid configuration. Using defaults.");
    memset(&config, 0, sizeof(config));
  }
}

void saveConfig() {
  EEPROM.put(0, config);
  EEPROM.commit();
}

bool isValidConfig(Config &conf) {
  return strlen(conf.ssid) > 0 && strlen(conf.password) > 0;
}

void connectToWiFi() {
  WiFi.mode(WIFI_STA);
  WiFi.begin(config.ssid, config.password);

  Serial.print("Connecting to WiFi...");
  int attempt = 0;
  while (WiFi.status() != WL_CONNECTED && attempt < 20) {
    delay(500);
    Serial.print(".");
    attempt++;
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("Connected!");
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());
    setOutputStatus(config.outputStatus);
  } else {
    Serial.println("Failed to connect. Starting AP mode...");
    startAPMode();
  }
}

void setOutputStatus(bool status) {
  Serial.print("Setting output status: ");
  Serial.println(status ? "ON" : "OFF");
  servoStatus = status; // Update the global flag
}
