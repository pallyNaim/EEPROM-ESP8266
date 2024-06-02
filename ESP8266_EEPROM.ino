// Include necessary libraries for WiFi, web server, EEPROM, and servo functionality
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <EEPROM.h>
#include <Servo.h>

// Define the size of the EEPROM storage
#define EEPROM_SIZE 512

// Define the pin for the servo motor
#define SERVO_PIN D5 // Use D5

// Define a structure to hold the configuration settings
struct Config {
  char ssid[32];
  char password[32];
  char deviceID[32];
  bool outputStatus;
};

Config config; // Create an instance of the configuration structure
ESP8266WebServer server(80); // Create a web server instance on port 80
Servo myServo; // Create a servo instance
bool servoStatus = false; // Global flag to track servo status

void setup() {
  Serial.begin(115200); // Start the serial communication for debugging
  EEPROM.begin(EEPROM_SIZE); // Initialize the EEPROM with defined size

  myServo.attach(SERVO_PIN); // Attach the servo to the defined pin
  myServo.write(0); // Initialize servo to 0 degrees

  // Load configuration from EEPROM
  loadConfig();

  // Check if WiFi credentials are available
  if (strlen(config.ssid) == 0 || strlen(config.password) == 0) {
    startAPMode(); // Start Access Point mode if credentials are missing
  } else {
    connectToWiFi(); // Connect to WiFi if credentials are available
  }
}

void loop() {
  server.handleClient(); // Handle client requests to the web server

  // Control the servo motor based on the global flag
  if (servoStatus) {
    myServo.write(180); // Turn servo to 180 degrees
    delay(1000); // Wait for 1 second
    myServo.write(0); // Turn servo to 0 degrees
    delay(1000); // Wait for 1 second
  }
}

void startAPMode() {
  WiFi.mode(WIFI_AP); // Set WiFi mode to Access Point
  WiFi.softAP("ESP8266_Config"); // Start the Access Point with SSID "ESP8266_Config"

  IPAddress IP = WiFi.softAPIP(); // Get the IP address of the Access Point
  Serial.print("AP IP address: ");
  Serial.println(IP);

  // Define the web server routes and handlers
  server.on("/", HTTP_GET, []() {
    server.send(200, "text/html", getHTML()); // Serve the configuration page
  });

  server.on("/save", HTTP_POST, []() {
    // Handle the form submission and save the configuration
    if (server.hasArg("ssid") && server.hasArg("password") && server.hasArg("deviceID") && server.hasArg("outputStatus")) {
      strcpy(config.ssid, server.arg("ssid").c_str());
      strcpy(config.password, server.arg("password").c_str());
      strcpy(config.deviceID, server.arg("deviceID").c_str());
      config.outputStatus = server.arg("outputStatus") == "on";

      saveConfig(); // Save the configuration to EEPROM
      server.send(200, "text/html", "Configuration saved. Restarting...");
      delay(1000);
      ESP.restart(); // Restart the ESP8266 to apply the new settings
    } else {
      server.send(400, "text/html", "Invalid input."); // Send an error message for invalid input
    }
  });

  server.begin(); // Start the web server
}

String getHTML() {
  // Generate the HTML form for the configuration page with added CSS for styling
  return "<html><head>"
         "<style>"
         "body { font-family: Arial, sans-serif;}"
         "form { margin: 0 auto; max-width: 400px;}"
         "label { display: block; margin-bottom: 10px;}"
         "input[type='text'], input[type='password'], input[type='radio'] { width: 100%; padding: 10px; margin-bottom: 10px;}"
         "input[type='submit'] { width: 100%; padding: 10px; background-color: #4CAF50; color: white; border: none; cursor: pointer;}"
         "</style>"
         "</head><body>"
         "<form action='/save' method='POST'>"
         "<label for='ssid'>SSID:</label>"
         "<input type='text' id='ssid' name='ssid'><br>"
         "<label for='password'>Password:</label>"
         "<input type='password' id='password' name='password'><br>"
         "<label for='deviceID'>Device ID:</label>"
         "<input type='text' id='deviceID' name='deviceID'><br>"
         "<label for='outputStatus'>Output Status:</label>"
         "<input type='radio' id='on' name='outputStatus' value='on'>"
         "<label for='on'>On</label>"
         "<input type='radio' id='off' name='outputStatus' value='off'>"
         "<label for='off'>Off</label><br>"
         "<input type='submit' value='Save'>"
         "</form>"
         "</body></html>";
}

void loadConfig() {
  // Load the configuration from EEPROM into the config structure
  EEPROM.get(0, config);
  if (isValidConfig(config)) {
    Serial.println("Loaded configuration from EEPROM.");
  } else {
    Serial.println("Invalid configuration. Using defaults.");
    memset(&config, 0, sizeof(config)); // Clear the config structure if the data is invalid
  }
}

void saveConfig() {
  // Save the configuration to EEPROM
  EEPROM.put(0, config);
  EEPROM.commit();
}

bool isValidConfig(Config &conf) {
  // Validate the configuration data
  return strlen(conf.ssid) > 0 && strlen(conf.password) > 0;
}

void connectToWiFi() {
  // Attempt to connect to the WiFi network using the stored credentials
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
    setOutputStatus(config.outputStatus); // Set the initial output status
  } else {
    Serial.println("Failed to connect. Starting AP mode...");
    startAPMode(); // Start Access Point mode if connection fails
  }
}

void setOutputStatus(bool status) {
  // Set the output status and update the servo control flag
  Serial.print("Setting output status: ");
  Serial.println(status ? "ON" : "OFF");
  servoStatus = status; // Update the global flag
}
