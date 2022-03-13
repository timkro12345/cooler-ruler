/**************************************
 * APPLICATION CONFIGURATION CONSTANTS
 **************************************/
const int DEFAULT_SET_POINT = 37;          // target temperature for room (adjustable)
const int SET_POINT_RANGE = 2;             // to make AC startup less frequent, we'll turn on when SetPoint+Range is reached and cool until SetPoint-Range is reached
const int DEFROST_TEMPERATURE_TRIGGER=28;  // temperate on fins that triggers defrost
const int DEFROST_MINUTES = 3;             // minutes (if CHECK_EVERY = 1 min) to wait to allow ice to melt 
const int TOOLOW_WARNING_TEMPERATURE = 32;  // trigger alarm if temp is below this
const int TOOHIGH_WARNING_TEMPERATURE= 85;  // trigger alarm if temp is above this
const int NOT_NEEDED_POINT = 65;           // when room temperature is above this, shut off heater because AC unit will run on its own
const int SETPOINT_MAX = 64;              // setpoint maximum
const int SETPOINT_MIN = 35;              // setpoint minimum 
const int CHECK_EVERY = 60000;             // how many milliseconds to wait between temperature readings and responses  (60000 = 60 secs = 1 min)
const int ON = 1;
const int OFF = 0;

/**************************************
 * BOARD CONFIGURATION - HiLetGo ESP32 with OLED
 **************************************/
const int ONE_WIRE_BUS_PIN = 17;   // sensor cluster
const int HEATER_PIN = 22;         // heater output
const int RED_BUTTON_PIN = 12;     // red button
const int BLUE_BUTTON_PIN = 13;    // blue button
const int AUDIO_PIN = 25;          // audio signal
const int HUMIDITY_PIN = 21;       // DHT22 humidity sensor

    // not available (spi memory) - 6, 7, 8, 9, 10, 11
    // pins used by OLED - 4, 15, 16
    // pins used by LoRa - 5, 14, 18, 19, 26, 27
    // input only on ESP32 - 32, 33, 34, 35, 36, 37, 38, 39
    // DAC available on 25, 26 - we'll choose 25 for audio output
    // pins 19 and 26 don't have PWM, all else do
    // Not recommended but may be available:  5 14 18 19 26 27
    // hard to solder pins: 26  25 33 32 35 34 39

    // Available for input:   2 12 13 17 21 22 23 X32 X33 X34 X35 36 37 38 X39 
    // Available for output:  2 12 13 17 21 22 23
    




/**************************************
 * LIBRARIES
 **************************************/
#include "mypasswords.h"
#include "OneWire.h"
#include "DallasTemperature.h"
#include "DHT.h"
#include "heltec.h"
#include "WiFi.h"
#include "MoreSounds.h"
#include "XT_DAC_Audio.h"
#include "ThingSpeak.h"





/**************************************
 * VARIABLES
 **************************************/
int SetPoint = DEFAULT_SET_POINT;
int RoomTemp;
int ExtraTemp;   // can be used as second room temp or as heater temp
int FinsTemp;
int OutsideTemp;
float RoomHumidity;
int CoolerRulerStatus = 0;
unsigned long SensorTimer = CHECK_EVERY;
int RedButtonTimer = 0;
int BlueButtonTimer = 0;
int DefrostTimer = 0;

/**************************************
 * CONSTANTS FOR FUNCTIONS
 **************************************/
const int ADJUSTMENT_BEEP=1;
const int ERROR_BEEP=2;
const int ALARM=3;
const int MESSAGE_BAR = 1;
const int SETPOINT_TEMP = 2;
const int TEMPERATURE_BAR = 3;
const int CR_STATUS_IDLE = 0;
const int CR_STATUS_COOLING = 1; 
const int CR_STATUS_DEFROST = 2;
const int CR_STATUS_TOOHOT = 3;
const int CR_STATUS_TOOCOLD = 4;
const int CR_STATUS_JUSTRIGHT = 5;
char *crStatusMessage[] = {"Warm: Running normal AC","Cool: Forcing AC colder","Frozen: Melting ice","Warning: Too Hot","Warning: Below Freezing", "Off: All is well"};





/**************************************
 * INPUTS
 * 
 * TEMPERATURE SENSORS - D18B20 sensors with Dallas Temperature library One-Wire, DHT22 Humidity Sensor
 **************************************/
// these numbers are unique to the sensor and would have to be discovered
OneWire oneWire(ONE_WIRE_BUS_PIN);  
DallasTemperature sensors(&oneWire);
DeviceAddress sensor1 = {0x28, 0xA8, 0xEB, 0x4F, 0x32, 0x20, 0x1, 0x3D};
DeviceAddress sensor2 = {0x28, 0xAC, 0xFC, 0x4F, 0x32, 0x20, 0x1, 0xCC};
DeviceAddress sensor3 = {0x28, 0x16, 0x82, 0x9C, 0x32, 0x20, 0x1, 0x42};
DeviceAddress sensor4 = {0x28, 0x1F, 0xB5, 0x9E, 0x32, 0x20, 0x1, 0x6};
#define DHTTYPE DHT22 
DHT dht(HUMIDITY_PIN, DHTTYPE); 

/**************************************
 * BUTTONS
 **************************************/
const long DEBOUNCE_DELAY = 20; 
unsigned long RedButtonDebounce =0;
unsigned long BlueButtonDebounce =0;
int RedButtonLastState = LOW;
int BlueButtonLastState = LOW;
int RedButtonState;
int BlueButtonState;

void MonitorButtons(void) {
  int reading = digitalRead(RED_BUTTON_PIN);
  if (reading!=RedButtonLastState) {
    RedButtonDebounce=millis();
  }
  if ((millis() - RedButtonDebounce) > DEBOUNCE_DELAY) {
    if (reading != RedButtonState) {
      RedButtonState = reading;
      if (RedButtonState==HIGH) {
        if (SetPoint>=SETPOINT_MAX) {
          MakeSound(ERROR_BEEP);
        } else {
          SetPoint++;
          ChangeDisplay(SETPOINT_TEMP,String(SetPoint));
          MakeSound(ADJUSTMENT_BEEP);
        }
      }
    }
  }
  RedButtonLastState=reading;
  
  reading = digitalRead(BLUE_BUTTON_PIN);
  if (reading!=BlueButtonLastState) {
    BlueButtonDebounce=millis();
  }
  if ((millis() - BlueButtonDebounce) > DEBOUNCE_DELAY) {
    if (reading != BlueButtonState) {
      BlueButtonState = reading;
      if (BlueButtonState==HIGH) {
        if (SetPoint<=SETPOINT_MIN) {
          MakeSound(ERROR_BEEP);
        } else {
          SetPoint--;
          ChangeDisplay(SETPOINT_TEMP,String(SetPoint));
          MakeSound(ADJUSTMENT_BEEP);
        }
      }
    }
  }
  BlueButtonLastState=reading;
}

void ReadSensors(void){
  sensors.requestTemperatures();
  RoomTemp=sensors.getTempF(sensor1);
  ExtraTemp=sensors.getTempF(sensor2);
  FinsTemp=sensors.getTempF(sensor3);
  OutsideTemp=sensors.getTempF(sensor4);
  RoomHumidity=dht.readHumidity();
}




/**************************************
 * DISPLAY & GRAPH
 **************************************/
void ChangeDisplay(int attribute, String displayedValue) {
  switch (attribute) {
    case MESSAGE_BAR:
      Heltec.display->setColor(BLACK);
      Heltec.display->fillRect(0,0,128,15);
      Heltec.display->setColor(WHITE);
      Heltec.display->setTextAlignment(TEXT_ALIGN_LEFT);
      Heltec.display->setFont(ArialMT_Plain_10);
      Heltec.display->drawStringMaxWidth(0, 0, 128, displayedValue);
      Heltec.display->display();
      Serial.print("MSG:");
      Serial.println(displayedValue);
      break;
    case SETPOINT_TEMP:
      Heltec.display->setColor(BLACK);
      Heltec.display->fillRect(103,20,128,30);
      Heltec.display->setColor(WHITE);
      Heltec.display->setTextAlignment(TEXT_ALIGN_RIGHT);
      Heltec.display->setFont(ArialMT_Plain_24);
      Heltec.display->drawString(128, 32-12, displayedValue);
      Heltec.display->display();
      Serial.print("Set Point is ");
      Serial.println(displayedValue);
      break;
    case TEMPERATURE_BAR:
      Heltec.display->setColor(BLACK);
      Heltec.display->fillRect(0,49,128,63);
      Heltec.display->setColor(WHITE);
      Heltec.display->setTextAlignment(TEXT_ALIGN_LEFT);
      Heltec.display->setFont(ArialMT_Plain_10);
      Heltec.display->drawString(0, 49, displayedValue);
      Heltec.display->display();
      Serial.print("TEMPS:");
      Serial.println(displayedValue);
      break;
    default:
      Serial.println(displayedValue);
      break;    
  }
}

const float GRAPH_BOUNDS_X_LEFT = 0.0;
const float GRAPH_BOUNDS_X_RIGHT = 90.0;
const float GRAPH_BOUNDS_Y_TOP = 16.0;
const float GRAPH_BOUNDS_Y_BOTTOM = 64.0-16.0;
const float GRAPH_RANGE_UPPER = 70.0;
const float GRAPH_RANGE_LOWER = 32.0;
int GRAPH_POSITION = GRAPH_BOUNDS_X_LEFT;

void LabelGraph(void) {
  Heltec.display->setColor(WHITE);
  Heltec.display->setTextAlignment(TEXT_ALIGN_LEFT);
  Heltec.display->setFont(ArialMT_Plain_10);
  Heltec.display->drawString(0, (int) GRAPH_BOUNDS_Y_TOP, String(GRAPH_RANGE_UPPER));
  Heltec.display->drawString(0, (int) GRAPH_BOUNDS_Y_BOTTOM - 10, String(GRAPH_RANGE_LOWER));
  Heltec.display->display();
}
 
void DisplayGraph(void) {
  float x, y;
  float y1, y2;
  Heltec.display->setColor(WHITE);
  x = GRAPH_POSITION;
  y = (GRAPH_RANGE_UPPER - RoomTemp) * ((GRAPH_BOUNDS_Y_BOTTOM - GRAPH_BOUNDS_Y_TOP) / (GRAPH_RANGE_UPPER - GRAPH_RANGE_LOWER)) + GRAPH_BOUNDS_Y_TOP;
  if (y < GRAPH_BOUNDS_Y_TOP) { y=GRAPH_BOUNDS_Y_TOP; }
  if (y > GRAPH_BOUNDS_Y_BOTTOM) { y=GRAPH_BOUNDS_Y_BOTTOM; }
  Heltec.display->setPixel(x,y);

  y = (GRAPH_RANGE_UPPER - SetPoint) * ((GRAPH_BOUNDS_Y_BOTTOM - GRAPH_BOUNDS_Y_TOP) / (GRAPH_RANGE_UPPER - GRAPH_RANGE_LOWER)) + GRAPH_BOUNDS_Y_TOP;
  if (y < GRAPH_BOUNDS_Y_TOP) { y=GRAPH_BOUNDS_Y_TOP; }
  if (y > GRAPH_BOUNDS_Y_BOTTOM) { y=GRAPH_BOUNDS_Y_BOTTOM; }
  if (int(x) % 2) 
    Heltec.display->setPixel(x,y);

  Heltec.display->setColor(BLACK);
  GRAPH_POSITION++;
  if (GRAPH_POSITION > GRAPH_BOUNDS_X_RIGHT) { GRAPH_POSITION = GRAPH_BOUNDS_X_LEFT; };
  Heltec.display->drawLine(GRAPH_POSITION,GRAPH_BOUNDS_Y_TOP, GRAPH_POSITION, GRAPH_BOUNDS_Y_BOTTOM);
  Heltec.display->display();
}

String DisplayTemperatureFormatted(float t) {
  //if (t>99) {
  //  return("HI");
  //} 
  //if (t<10) {
  //  return("LO");
  //}
  return(String((int) round(t)));
}
  
void DisplayTemperatures(void) {
  // Room, Air, Fins, eXtra, Humidity
  String s;
  s="R";
  s+=DisplayTemperatureFormatted(RoomTemp);
  s+=" A";
  s+=DisplayTemperatureFormatted(OutsideTemp);
  s+=" F";
  s+=DisplayTemperatureFormatted(FinsTemp);
  s+=" X";
  s+=DisplayTemperatureFormatted(ExtraTemp);
  s+=" H";
  s+=DisplayTemperatureFormatted(RoomHumidity);
  ChangeDisplay(TEMPERATURE_BAR, s);
}

/**************************************
 * HEATER
 **************************************/
byte HeaterState=OFF;
void Heater(byte state) {
  if (state==ON) {
    digitalWrite(HEATER_PIN, LOW);
  } else {
    digitalWrite(HEATER_PIN, HIGH);
  }
}

/**************************************
 * SOUNDS (OPTIONAL)
 **************************************/
XT_Wav_Class Sound_beep(wav_beep);
XT_Wav_Class Sound_error(wav_error);
XT_Wav_Class Sound_alarm(wav_alarm);
XT_DAC_Audio_Class DacAudio(AUDIO_PIN,0);

void MakeSound(int i) {
  DacAudio.FillBuffer();
  switch (i) {
    case ADJUSTMENT_BEEP: 
      DacAudio.DacVolume=50;     
      if (Sound_beep.Playing==false)
        DacAudio.Play(&Sound_beep, false);
      break;
    case ERROR_BEEP:
      DacAudio.DacVolume=50;
      if (Sound_error.Playing==false)
        DacAudio.Play(&Sound_error, false);   
      break;
    case ALARM:
      DacAudio.DacVolume=100;
      if (Sound_alarm.Playing==false)
        DacAudio.Play(&Sound_alarm, false);
      break;  
  }
}

/**************************************
 * NETWORK AND THINGSPEAK
 **************************************/
// loaded from myPassword.h (not saved to Git)
// const char* ssid     = ****;
// const char* password = ****;
// char thingSpeakAddress[] = ****;
// unsigned long channelID = ****;
// char* readAPIKey = ****
// char* writeAPIKey = ****
WiFiClient client;

void StartWifi() {
    WiFi.begin( ssid, password );   
    ThingSpeak.begin( client );
}

int connectWiFi(){
  if (WiFi.status() != WL_CONNECTED) {
    WiFi.disconnect();
    WiFi.reconnect();
    delay(10000);
  }
}

void TransmitTemperatures(void) {
  connectWiFi();
  ThingSpeak.setField( 1, RoomTemp );
  ThingSpeak.setField( 2, FinsTemp );
  ThingSpeak.setField( 3, OutsideTemp );
  ThingSpeak.setField( 4, ExtraTemp);
  ThingSpeak.setField( 5, RoomHumidity );
       
  int writeSuccess = ThingSpeak.writeFields( channelID, writeAPIKey );
  if (writeSuccess) {
    Serial.println("Data sent");
  } else {
    Serial.println("Failed sending data");
  }
  Serial.print(WiFi.status());
  Serial.print("->");
  Serial.println(WiFi.localIP());
}






/**************************************
 * APPLICATION
 **************************************/
void setup() {
  Serial.begin(115200);
  Heltec.begin(true /*DisplayEnable Enable*/, false /*LoRa Disable*/, true /*Serial Enable*/);
  Heltec.display->clear();
  Heltec.display->setContrast(255);
  ChangeDisplay(SETPOINT_TEMP,String(SetPoint));

  pinMode(17,INPUT);
  pinMode(HUMIDITY_PIN, INPUT);
  sensors.begin();
  dht.begin();

  pinMode(HEATER_PIN, OUTPUT);
  CoolerRulerStatus=CR_STATUS_IDLE;

  pinMode(RED_BUTTON_PIN, INPUT);
  pinMode(BLUE_BUTTON_PIN, INPUT);

  MakeSound(ALARM);
  
  delay(1000);
  StartWifi();
}

void loop() {
  DacAudio.FillBuffer();
  MonitorButtons();
  
  if ((unsigned long) (millis() - SensorTimer ) >= CHECK_EVERY ){
    SensorTimer = millis(); // reset timer
    ReadSensors();
    DisplayTemperatures();
    DisplayGraph();
    TransmitTemperatures();

    // first priority - alarms when ambient temperature extremes are exceeded
    if ((int)RoomTemp >= TOOHIGH_WARNING_TEMPERATURE ) CoolerRulerStatus = CR_STATUS_TOOHOT; 
    else if ((int)RoomTemp <= TOOLOW_WARNING_TEMPERATURE) CoolerRulerStatus = CR_STATUS_TOOCOLD;
    else {

      // second priority - defrost if fins are icing
      if (FinsTemp <= DEFROST_TEMPERATURE_TRIGGER) {
          CoolerRulerStatus = CR_STATUS_DEFROST;          
          Heater(OFF);
          DefrostTimer = DEFROST_MINUTES;
      } else {
        if ((CoolerRulerStatus==CR_STATUS_IDLE) && (RoomTemp < NOT_NEEDED_POINT) && (RoomTemp > SetPoint + SET_POINT_RANGE) ) {
          CoolerRulerStatus = CR_STATUS_COOLING;
          Heater(ON);
        } 
        if (CoolerRulerStatus==CR_STATUS_DEFROST) {
          if ((DefrostTimer <= 0) && (FinsTemp > DEFROST_TEMPERATURE_TRIGGER)) {
            DefrostTimer=0;
            CoolerRulerStatus = CR_STATUS_IDLE;
          } else {
            DefrostTimer --;
          }
        }
        if ((CoolerRulerStatus==CR_STATUS_COOLING)) {
          if (RoomTemp >= NOT_NEEDED_POINT) {
            CoolerRulerStatus = CR_STATUS_IDLE;
            Heater(OFF);
            }
          if (RoomTemp < SetPoint - SET_POINT_RANGE ) {
            CoolerRulerStatus = CR_STATUS_JUSTRIGHT;          
            Heater(OFF);
          }
        } 
      }          
    }
  ChangeDisplay(MESSAGE_BAR, crStatusMessage[CoolerRulerStatus]);
  }
  if ((CoolerRulerStatus==CR_STATUS_TOOHOT) || (CoolerRulerStatus==CR_STATUS_TOOCOLD)) MakeSound(ALARM);
}
