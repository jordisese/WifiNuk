//  

#include <ESP8266WiFi.h>
#include <ArduinoOTA.h>
#include <Wire.h>
#include <NintendoExtensionCtrl.h>

Nunchuk nchuk;
#define ESP01

// PINS SDA/SDL
// Wemos D1 MINI: SDA D2, SCL D1
// ESP01: SDA 0, SCL 2
// USER_LED_PIN para WEMOS D1 MINI: D5
// USER_LED_PIN para ESP01: 1
#ifdef ESP01
#define SDA_PIN 0 // D2
#define SCL_PIN 2 // D1
#define USER_LED_PIN 1
#else // WEMOS D1 MINI
#define SDA_PIN D2 // 0 // D2
#define SCL_PIN D1 // 2 // D1
#define USER_LED_PIN D5
#define SERIAL_DEBUG
#endif



#define USER_LED_MILLIS  100
boolean ledOn;
uint32_t ledOffAt;


#include <Ticker.h>
Ticker tickerKey;


#define AP
#ifndef AP
#define STASSID "MYSSID"
#define STAPSK  "MYPASS"
const char* host = "192.168.1.14";
#else
#define STASSID "ESP32go"
//#define STASSID "ESP-PGT"
#define STAPSK  "boquerones"
const char* host = "192.168.4.1";
#endif

WiFiClient client;
const char* ssid     = STASSID;
const char* password = STAPSK;
bool bklight = true;
bool focusF = false;
const uint16_t port = 10001;
void InitOTA()
{ ArduinoOTA.setHostname("wifinuk.local");
  ArduinoOTA.onStart([]() {
    String type;
    if (ArduinoOTA.getCommand() == U_FLASH) {
      type = "sketch";
    } else { // U_SPIFFS
      type = "filesystem";
    }
  });
  ArduinoOTA.onEnd([]() { });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) { });
  ArduinoOTA.onError([](ota_error_t error) { });
  ArduinoOTA.begin();

}

void turnLedOn(bool set = true) {
  digitalWrite( USER_LED_PIN, HIGH );
  if(set)
    ledOn = true;
  ledOffAt = millis() + USER_LED_MILLIS;
}

void turnLedOff() {
  digitalWrite( USER_LED_PIN, LOW );
  ledOn = false;
}



int curx, cury, pressed;
int lastx, lasty, lastpress;

// pressed --> 0: NONE, 1: FOCUS, 2: AXIS
// lastx --> 0: EAST, 1: CENTER, 2: WEST
// lasty --> 0: SOUTH, 1: CENTER, 2: NORTH

void nunchuck_process(void)
{
  pressed=0;
  boolean zButton = nchuk.buttonZ();
  if(zButton)
    pressed = 1;
  boolean cButton = nchuk.buttonC();
  if(cButton)
    pressed = 2;
  uint8_t hor = nchuk.joyX();
  uint8_t ver = nchuk.joyY();
  curx=1;
  cury=1;
  if(hor < 70)
    curx = 0;
  if(hor > 140)
    curx = 2;
  if(ver < 70)
    cury = 0;
  if(ver > 140)
    cury = 2;    
}

void process_action(void)
{
  if (pressed) lastpress = pressed;
  if (lastx != curx)
  {
    turnLedOn();
#ifdef SERIAL_DEBUG
Serial.println("X["+String(curx)+"] - B["+String(lastpress)+"]");
#endif
    switch (curx)
    {
        case 0 : // X-EAST ON
                  if (pressed == 2) client.print(":RS#"); // SLEW RATE
                    else if (lastpress == 1) {/*client.print(":FS#");*/client.print(":F+#");} // FOCUS +
                    else if (pressed == 0) client.print(":Mw#"); // WEST
                  break;
        case 1 : // X-CENTER
                  client.print(":Qw#");
                  client.print(":Qe#");
                  client.print(":FQ#");
                  break;
        case 2:  // X-WEST ON
                  if (pressed == 2) client.print(":RM#"); // FIND RATE
                    else if (lastpress == 1) {/*client.print(":FS#");*/client.print(":F-#");} // FOCUS -
                    else if (pressed == 0) client.print(":Me#"); // EAST          
                  break;
    }
  lastx = curx;
  }
  if (lasty != cury)
  {
    turnLedOn();
#ifdef SERIAL_DEBUG
Serial.println("Y["+String(cury)+"] - B["+String(lastpress)+"]");
#endif
    switch (cury)
    {
        case 0 : // Y-SOUTH ON
                  if (pressed == 2) client.print(":RG#"); // GUIDE RATE
                    else if (lastpress == 1) client.print(":F++#"); // FOCUS ++
                    else if (pressed == 0) client.print(":Ms#"); // SOUTH
                  break;
        case 1 : // Y-CENTER
                  client.print(":Qs#");
                  client.print(":Qn#");
                  client.print(":FQ#");
                  break;
        case 2:  // Y-NORTH ON
                  if (pressed == 2) client.print(":RC#"); // CENTER RATE
                    else if (lastpress == 1) client.print(":F--#"); // FOCUS --
                    else if (pressed == 0) client.print(":Mn#"); // NORTH          
                  break;
    }
    lasty = cury;
  }

}


void setup() {
  Wire.begin(SDA_PIN, SCL_PIN);
#ifdef SERIAL_DEBUG
  Serial.begin(115200);
  Serial.println();
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);
#endif
  pinMode(USER_LED_PIN, OUTPUT);

  turnLedOn(false);

  WiFi.mode(WIFI_STA);
#ifndef AP
  IPAddress ip(192, 168, 1, 35);
  IPAddress gateway(192, 168, 1, 1);
  IPAddress subnet(255, 255, 0, 0);
  IPAddress DNS(192, 168, 1, 1);
#else
  IPAddress ip(192, 168, 4, 88);
  IPAddress gateway(192, 168, 4, 1);
  IPAddress subnet(255, 255, 0, 0);
  IPAddress DNS(192, 168, 4, 1);
#endif
  WiFi.config(ip, gateway, subnet, gateway);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(300);
#ifdef SERIAL_DEBUG    
    Serial.print("Wifi status: ");
    Serial.println(WiFi.status());
#endif
  }
#ifdef SERIAL_DEBUG
  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
#endif
  nchuk.begin();
  while (!nchuk.connect()) {
#ifdef SERIAL_DEBUG
    Serial.println("Nunchuk not detected!");
#endif
    delay(1000);
  }

  InitOTA();
}


void loop() {
#ifdef SERIAL_DEBUG
  Serial.print("connecting to ");
  Serial.print(host);
  Serial.print(':');
  Serial.println(port);
#endif
  //String s;
  // Use WiFiClient class to create TCP connections
#ifdef SERIAL_DEBUG
  Serial.println("Connecting....");
#endif
  if (!client.connect(host, port)) {
#ifdef SERIAL_DEBUG
    Serial.println("connection failed");
#endif
    delay(5000);
    return;
  }
#ifdef SERIAL_DEBUG
  Serial.print("connected");
  Serial.println(":IP"+String(WiFi.localIP()[3])+"."+String(WiFi.localIP()[2])+"#");
#endif
  turnLedOff();

  while (1) 
  {
    if( ledOn && ( millis() > ledOffAt ) )
      turnLedOff();
    boolean success = nchuk.update();  // Get new data from the controller
    if (!success) 
    {
#ifdef SERIAL_DEBUG
      Serial.println("Controller disconnected!");
#endif
      turnLedOn(false);
      delay(1000);
      ESP.reset();
    }
    else
    {
      nunchuck_process();
      process_action();
    }

    ArduinoOTA.handle();
    delay(100); //
  }
}



