//  

#include <ESP8266WiFi.h>
#include <ArduinoOTA.h>
#include <Wire.h>
#include <NintendoExtensionCtrl.h>

ExtensionPort controller;
Nunchuk nchuk;
SNESMiniController snes;
String controllerType="";


//#define ESP01
//#define OTA

// PINS SDA/SDL
// Wemos D1 MINI: SDA D2, SCL D1
// ESP01: SDA 0, SCL 2
// USER_LED_PIN para WEMOS D1 MINI: D5
// USER_LED_PIN para ESP01: 1
// OJO: si queremos hacer SERIAL_DEBUG con el ESP01 hay que usar el GPIO3 (RX) para el LED y dejar el GPIO1 (TX) para hacer debug
#ifdef ESP01
 #define SDA_PIN 0 
 #define SCL_PIN 2 
// #define SERIAL_DEBUG
 #ifdef SERIAL_DEBUG
  #define USER_LED_PIN 3
 #else
  #define USER_LED_PIN 1
 #endif
#else // WEMOS D1 MINI
 #define SDA_PIN D2 
 #define SCL_PIN D1 
 #define USER_LED_PIN D5
 #define SWITCH_PIN D6
 #define SERIAL_DEBUG
#endif

#define USER_LED_MILLIS  100
boolean ledOn;
uint32_t ledOffAt;

#define ESP32GO_STASSID "ESP32go"
#define ESP32GO_STAPSK  "boquerones"
const char* esp32go_host = "192.168.4.1";
const uint16_t esp32go_port = 10001;

#define ONSTEPX_STASSID "OnStepX"
#define ONSTEPX_STAPSK  "password"
const char* onstepx_host = "192.168.0.1";
const uint16_t onstepx_port = 9999;


WiFiClient client;
char* ssid;
char* password;
const char* host;
uint16_t port;

String target = "ESP32GO"; // por defecto ESP32go, con el switch en ON, pasa a ONSTEPX

#ifdef OTA
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
#endif

void turnLedOn(bool set = true) {
  digitalWrite( USER_LED_PIN, HIGH );
  if(set)
    ledOn = true;
  ledOffAt = millis() + USER_LED_MILLIS;
}

void turnLedOff(void) {
  digitalWrite( USER_LED_PIN, LOW );
  ledOn = false;
}



int curx, cury, pressed;
int lastx, lasty, lastpress;
int focusmotor = 1; // así la primera pulsación de SELECT irá al 0
bool processed = false; // para pulsaciones de cambio

void swap_focus(void)
{
  focusmotor = (focusmotor == 0 ? 1:0);
}

// pressed --> 0: NONE, 1: FOCUS, 2: AXIS, 3: FOCUS_SWAP, 4: TRACK, 5: STOP_TRACK
// lastx --> 0: EAST, 1: CENTER, 2: WEST
// lasty --> 0: SOUTH, 1: CENTER, 2: NORTH

void nunchuck_process(void)
{
  pressed=0;
  boolean zButton = nchuk.buttonZ();
  if(zButton)
  {
    pressed = 1;
    if(lastpress == 1 && !processed)
    {
      pressed = 3; // focus motor swap
    }
  }
  else
  {
    processed = false; // para que la siguiente pulsación aislada se detecte
  }

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

void gamepad_process(void)
{
  pressed=0;
  curx=1;
  cury=1;
  if(snes.dpadLeft())
    curx = 0;
  if(snes.dpadRight())
    curx = 2;
  if(snes.dpadUp())
    cury = 0;
  if(snes.dpadDown())
    cury = 2;

  if(snes.buttonY())
  {
    pressed = 2;
    curx = 0;
  }
  if(snes.buttonX())
  {
    pressed = 2;
    cury = 0;
  }
  if(snes.buttonB())
  {
    pressed = 2;
    curx = 2;
  }
  if(snes.buttonA())
  {
    pressed = 2;
    cury = 2;
  }

  if(snes.buttonStart())
    pressed = 2;

  if(snes.buttonSelect())
  {
    pressed = 1;
    if(lastpress == 1 && !processed)
    {
      pressed = 3; // focus motor swap
    }
  }
  else
  {
    processed = false; // para que la siguiente pulsación aislada se detecte
  }

  if(snes.buttonL())
    pressed = 4; // track_on

  if(snes.buttonR())
    pressed = 5; // track_off

}

void process_action(void)
{
  if (target == "ESP32GO" && lastpress == 1 && pressed == 3 && !processed)
  {
    turnLedOn();
    pressed = 0;
    processed = true;
    swap_focus();
    client.print(":Fs"+String(focusmotor)+"#");
    client.print(":FQ#");
#ifdef SERIAL_DEBUG
Serial.println("focuser select["+String(focusmotor)+"]");
#endif
    return;
  }
  if (target == "ESP32GO" && pressed > 3)
  {
    turnLedOn();
    if(pressed == 5) // track off
      client.print(":Mh#");
    else // assume track on
      client.print(":Qw#");
    pressed = 0;
    return;
  }
  if(lastpress == 1 && pressed == 2)
    swap_focus();
  if (pressed) lastpress = pressed;
  if (lastx != curx)
  {
    if(!client_connected())
      client_connect();
    turnLedOn();
#ifdef SERIAL_DEBUG
Serial.println("X["+String(curx)+"] - B["+String(lastpress)+"]");
#endif
    switch (curx)
    {
        case 0 : // X-EAST ON
                  if (pressed == 2)
                  {
                    if(target == "ESP32GO")
                      client.print(":RS#"); 
                    else
                      client.print(":R9#");
                  } // SLEW RATE
                    else if (lastpress == 1)
                    {
                      if(target != "ESP32GO")
                        client.print(":FS#");
                      client.print(":F+#");
                    }// FOCUS +
                    else if (pressed == 0) client.print(":Mw#"); // WEST
                  break;
        case 1 : // X-CENTER
                  client.print(":Qw#");
                  client.print(":Qe#");
                  client.print(":FQ#");
                  break;
        case 2:  // X-WEST ON
                  if (pressed == 2)
                  {
                    if(target == "ESP32GO")
                      client.print(":RM#"); 
                    else
                      client.print(":R8#");
                  }// FIND RATE
                    else if (lastpress == 1)
                    {
                      if(target != "ESP32GO")
                        client.print(":FS#");
                      client.print(":F-#"); 
                    }// FOCUS -
                    else if (pressed == 0) client.print(":Me#"); // EAST          
                  break;
    }
  lastx = curx;
  }
  if (lasty != cury)
  {
    if(!client_connected())
      client_connect();
    turnLedOn();
#ifdef SERIAL_DEBUG
Serial.println("Y["+String(cury)+"] - B["+String(lastpress)+"]");
#endif
    switch (cury)
    {
        case 0 : // Y-SOUTH ON
                  if (pressed == 2) 
                  {
                    if(target == "ESP32GO")
                      client.print(":RG#");
                    else
                      client.print(":R2#");
                  }// GUIDE RATE
                  else if (lastpress == 1) 
                  {
                      if(target == "ESP32GO")
                      {
                        client.print(":F++#");
                      }
                      else
                      {
                        client.print(":FF#");
                        client.print(":F+#");
                      }
                  } // FOCUS ++
                  else if (pressed == 0) client.print(":Ms#"); // SOUTH

                  break;
        case 1 : // Y-CENTER
                  client.print(":Qs#");
                  client.print(":Qn#");
                  client.print(":FQ#");
                  break;
        case 2:  // Y-NORTH ON
                  if (pressed == 2)
                  {
                    if(target == "ESP32GO")
                      client.print(":RC#"); 
                    else
                      client.print(":R5#");
                  } // CENTER RATE
                  else if (lastpress == 1)
                  {
                    if(target == "ESP32GO")
                    {
                      client.print(":F--#");
                    }
                    else
                    {
                      client.print(":FF#");
                      client.print(":F-#");
                    }
                  } // FOCUS --
                  else if (pressed == 0) client.print(":Mn#"); // NORTH          
                  break;
    }
    lasty = cury;
  }

}

bool client_connected()
{
  return client.connected();
}

void client_connect()
{
  turnLedOn();
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
    delay(1000);
    ESP.restart();
    return;
  }
#ifdef SERIAL_DEBUG
  Serial.print("connected");
  Serial.println(":IP"+String(WiFi.localIP()[3])+"."+String(WiFi.localIP()[2])+"#");
#endif
  if(target == "ESP32GO")
    client.print(":IP"+String(WiFi.localIP()[3])+"."+String(WiFi.localIP()[2])+"#");
  turnLedOff();
}



void setup() {
  Wire.begin(SDA_PIN, SCL_PIN);
#ifdef SERIAL_DEBUG
#ifdef ESP01
  Serial.begin(115200,SERIAL_8N1,SERIAL_TX_ONLY);
#else
  Serial.begin(115200);
#endif
  delay(1000);
  Serial.println();
#endif

#ifdef SWITCH_PIN
  pinMode(SWITCH_PIN,INPUT_PULLUP);
  delay(10);
  if(digitalRead(SWITCH_PIN) == LOW)
  {
    target = "ONSTEPX";
#ifdef SERIAL_DEBUG
Serial.println("Switch is LOW, ONSTEPX selected");
#endif
  }
#endif

#ifdef SERIAL_DEBUG
  Serial.println();
  Serial.print("Connecting to ");
  //Serial.println(ssid);
  Serial.println(target=="ESP32GO" ? ESP32GO_STASSID:ONSTEPX_STASSID);
#endif
  pinMode(USER_LED_PIN, OUTPUT);

  turnLedOn(false);

  WiFi.mode(WIFI_STA);

  if(target=="ESP32GO")
  {
    ssid = ESP32GO_STASSID;
    password = ESP32GO_STAPSK;
    host = esp32go_host;
    port = esp32go_port;
    IPAddress ip(192, 168, 4, 88);
    IPAddress gateway(192, 168, 4, 1);
    IPAddress subnet(255, 255, 0, 0);
    IPAddress DNS(192, 168, 4, 1);
    WiFi.config(ip, gateway, subnet, gateway);
    WiFi.begin(ssid, password);
  }
  else // ONSTEPX
  {
    ssid = ONSTEPX_STASSID;
    password = ONSTEPX_STAPSK;
    host = onstepx_host;
    port = onstepx_port;
    IPAddress ip(192, 168, 0, 88);
    IPAddress gateway(192, 168, 0, 1);
    IPAddress subnet(255, 255, 0, 0);
    IPAddress DNS(192, 168, 0, 1);
    WiFi.config(ip, gateway, subnet, gateway);
    WiFi.begin(ssid, password);
  }

  int count=0;
  while (WiFi.status() != WL_CONNECTED) {
    delay(300);
#ifdef SERIAL_DEBUG    
    Serial.print("Wifi status: ");
    Serial.println(WiFi.status());
#endif
    count++;
    if(count > 15)
      ESP.restart();
  }
  turnLedOff();
#ifdef SERIAL_DEBUG
  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
#endif
  delay(500);
  turnLedOn(false);
	controller.begin();
  delay(500);
  while(!controller.connect())
  {
#ifdef SERIAL_DEBUG
    Serial.println("No controller detected!");
#endif
    delay(1000);
  }

	ExtensionType conType = controller.getControllerType();
  switch (conType) 
  {
		case(ExtensionType::Nunchuk):
#ifdef SERIAL_DEBUG
			Serial.println("Nunchuk connected!");
#endif
      controllerType="Nunchuk";
      controller.reset();
      nchuk.begin();
      nchuk.connect();
			break;

		case(ExtensionType::ClassicController):
#ifdef SERIAL_DEBUG
			Serial.println("Classic Controller connected!");
#endif
      controllerType="Gamepad";
      controller.reset();
      snes.begin();
      snes.connect();
			break;

    default:
#ifdef SERIAL_DEBUG
      Serial.println("Unsupported controller detected");
#endif
      turnLedOn(false);
      delay(1000);
      ESP.restart();
  }
  delay(100);

#ifdef OTA
  InitOTA();
#endif

  client_connect();
}



void loop() 
{
  if( ledOn && ( millis() > ledOffAt ) )
    turnLedOff();
  boolean success = false;
  if(controllerType=="Nunchuk")
    success = nchuk.update();  // Get new data from the controller
  else
    success = snes.update();  // Get new data from the controller
  if (!success) 
  {
#ifdef SERIAL_DEBUG
    Serial.println("Controller disconnected!");
#endif
    turnLedOn(false);
    delay(1000);
    ESP.restart();
  }
  else
  {
    if(controllerType=="Nunchuk")
      nunchuck_process();
    else
      gamepad_process();

    process_action();
  }
#ifdef OTA
  ArduinoOTA.handle();
#endif

  delay(50); 

}



