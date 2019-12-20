/*
    품명  세부명칭/규격 PN  구입처 단가  수량  가격
    분전함 ABS 콘트롤박스 투명    orangelight  ₩9,100   2  ₩18,200
    단자대 15A 6P  YS-FT010-02 electricdisty  ₩1,000   4  ₩4,000
    아두이노 메가 확장 쉴드   [6355]MIZ-SSA207  메카솔루션  ₩22,000  2  ₩44,000
    아두이노메가 Mega 2560 호환보드     A-2 에듀이노   ₩10,890  2  ₩21,780
    MQ-7    600147863 도매키트   ₩2,300   2  ₩4,600
    토양습도센서    291852454 도매키트   ₩1,800   2  ₩3,600
    방수형온도센서   [6260]YWR-SEN050007 메카솔루션  ₩5,500   2  ₩11,000
    물 수위센서    313679124 도매키트   ₩700   2  ₩1,400
    CDS센서   569145971 도매키트   ₩200   2  ₩400
    브레드보드 170홀  E-83  에듀이노   ₩660   2  ₩1,320
    저항  30종 E-1 에듀이노   ₩9,700   1  ₩9,700
    점퍼선 길이와 상황에 따라 달라서 금액에서 제외

    MQ-7 :  http://thesis.jmsaavedra.com/prototypes/software/mq-7-breakout-arduino-library/
            http://blog.naver.com/evnngsky/220350083650
    DHT11 : https://github.com/adafruit/DHT-sensor-library

*/

// DHT Sensor : Temperature, Humidity
#include <DHT_U.h>
#include <Adafruit_Sensor.h>

// Gas Sensor
#include <CS_MQ7.h>

// Temperature Sensor For Arduino 18B20 DS18B20-LINE
#include <OneWire.h>

#define WaterPin          A2
#define GasPinA           A3
#define SoilA             A4
#define CdsPin            A5

#define SoilD             3
#define GasPinD           4
#define DS18S20_Pin       5
#define DHTPIN            8           // Pin which is connected to the DHT sensor.

//#define DHT22USE          0         // use DHT11 Sensor

// DHT sensor and class declare
#ifdef DHT22USE
#define DHTTYPE           DHT22       // DHT 22 (AM2302), DHT 21 (AM2301)
#else
#define DHTTYPE           DHT11       // DHT 11
#endif

//#define DEMO true

// AP & Server Information
#ifdef DEMO
#define SSID      "U+Net_Linktree"                   // AP Name
#define PASS      "2000011191"         // AP Password
#define DST_IP    "106.246.227.250"    // IP address of network Service daemon 
#define DST_PORT  9999                 // network Service daemon port
#else
#define SSID      "jeon"              // AP Name
#define PASS      "12345678910"        // AP Password
#define DST_IP    "192.168.0.6"        // IP address of network Service daemon 
#define DST_PORT  23                    // network Service daemon port
#endif

// Debug Message Print
#define DEBUG true                    // Debug message print boolean Flag

/*
   Global Variables
*/
int temperature;
int humidity;
int CoDataA;
int CoDataD;
int waterLevel;
int SoilMValueA;
int SoilMValueD;
int lightLevel;
int gasLevel;
int SoilTemp;

// for autotune (LED)
int low;
int high;

// for Sensing data & Command String on Socket
String sndData;
String sndDatai;

// String manipulation.
int pos;
char cTempData[4];

boolean ALREADY = false;

// Global Class
OneWire ds(DS18S20_Pin);
CS_MQ7 MQ7(4, 12);                    // 4 = digital Pin connected to "tog" from sensor board
DHT_Unified dht(DHTPIN, DHTTYPE);     // DHT class object
sensor_t sensor;                      // DHT sensor parameter object (limitation, precision)

int positions[12] = {0};
int tok[12] = {30, 10, 90, 40, 100, 50, 8, 7, 1000, 500, 200, 100};

String macaddr;                       // mac address on wifi
String inputString;                   // User Command from Service Daemon
String writeString;                   // message by Sensor value for Sending to Service Daemon

void setup() {
  Serial.begin(9600);
  Serial3.begin(115200);

  // initialize for network connection.
  initNet();
}

// send request for control
/*
   temp_h_limit, temp_l_limit,                   : 0, 1
   humidity_h_limit, humidity_l_limit,           : 2, 3
   CoDataA_h_limit, CoDataA_l_limit,             : 4, 5
   waterLevel_h_limit, waterLevel_l_limit,       : 6, 7
   SoilMValueA_h_limit,SoilMValueA_l_limit,      : 8, 9
   lightLevel_h_limit, lightLevel_h_limit        : 10,11
*/
void requestControl(boolean debug)
{
  sndData = macaddr;

  // FO
  if (temperature >  tok[0] or humidity > tok[2]) {
    sndData += ",FO";
    sendData(sndData, sndData.length(), DEBUG);
  }

  if (CoDataA > tok[4]) {
    sndData += ",FO";
    sendData(sndData, sndData.length(), DEBUG);
  }

  // PO
  if (waterLevel < tok[7]) {
    sndData += ",PO";
    sendData(sndData, sndData.length(), DEBUG);
  }

  // PF
  if (waterLevel > tok[6]) {
    sndData += ",PF";
    sendData(sndData, sndData.length(), DEBUG);
  }

  // MO
  if (humidity < tok[3])
  {
    sndData += ",MO";
    sendData(sndData, sndData.length(), DEBUG);
  }

  if (SoilMValueA < tok[9])
  {
    sndData += ",MO";
    sendData(sndData, sndData.length(), DEBUG);
  }


  // MF
  if (humidity > tok[2])
  {
    sndData += ",MF";
    sendData(sndData, sndData.length(), DEBUG);
  }

  if (SoilMValueA > tok[8])
  {
    sndData += ",MF";
    sendData(sndData, sndData.length(), DEBUG);
  }

  // LO
  if (lightLevel > tok[11])
  {
    sndData += ",LO";
    sendData(sndData, sndData.length(), DEBUG);
  }

  // LF
  if (lightLevel < tok[10])
  {
    sndData += ",LF";
    sendData(sndData, sndData.length(), DEBUG);
  }

}

void loop()
{
  sndDatai = macaddr;
  sndDatai += ",QRY\r\n";
  String tmp = "AT+CIPSEND=";
  tmp += sndDatai.length();
  tmp += "\r\n";
  sendData(tmp, tmp.length(), DEBUG);
  sendData(sndDatai, sndDatai.length(), DEBUG);

  GetWaterLevel(false);
  GetDHTLevel(false);
  GetCDSLevel(false);
  GetSoilLevel(false);
  gasLevel = GetCoGas(false, 1);      // debug, type
  SoilTemp = getSoilTemp(false);      // 온도 측정 후 변수에 저장

  int length = makeData();

  sndDatai = "AT+CIPSEND=";
  sndDatai += length;
  sndDatai += "\r\n";
  sendData(sndDatai, 1000, DEBUG);
  sendData(writeString, 1000, DEBUG);

  requestControl(false);


  delay(6000);
}

// initiate network with esp8266.
void initNet()
{
  ALREADY = true;
  String data;
  sendData("AT+RST\r\n", 2000, DEBUG);                              // reset module
  sendData("AT+CWMODE=1\r\n", 1000, DEBUG);                         // configure as access point (working mode:STA)

  data = "AT+CWJAP=\"";
  data += SSID;
  data += "\",\"";
  data += PASS;
  data += "\"\r\n";
  sendData(data, 5000, DEBUG);                                      // join the access point
  sendData("AT+CIFSR\r\n", 2000, DEBUG);                            // get ip address

  data = "AT+CIPSTART=\"TCP\",\"";
  data += DST_IP;
  data += "\",";
  data += DST_PORT;
  data += "\r\n";
  sendData(data, 2000, DEBUG);

  data = "AT+CIPSTAMAC?";
  data += "\r\n";
  sendData(data, 100, DEBUG);

  ALREADY = false;
}

// make message protocol for networking.
String sendData(String command, const int timeout, boolean debug) {
  String response = "";
  Serial3.print(command);             // send the read character to the esp8266
  long int time = millis();

  while ( (time + timeout) > millis()) {
    while (Serial3.available()) {
      // The esp has data so display its output to the serial window
      char c = Serial3.read();        // read the next character.
      response += c;
    }
  }

  if (debug)
  {
    Serial.print(response);
  }

  if (response.indexOf("link is not valid") > 1)
  {
    if (!ALREADY)
    {
      initNet();
    }
  }

  // AT+CIPSTAMAC?
  // +CIPSTAMAC:"5c:cf:7f:a5:60:19"
  int pos = response.indexOf("+CIPSTAMAC:");
  if (pos > 1)
  {
    macaddr = response.substring(pos + 12, pos + 29);
  }

  return response;
}

// make message from Sensing values. and return length of writeString message.
int makeData()
{
  /*
     수분이 없으면 아날로그 출력값은 1023, 디지털 출력값은 1이며,
     센서를 물에 깊이 담글수록 아날로그 출력값이 감소하고 디지털 출력값은 0이 된다.
  */

  writeString = "\n";                     // 시작코드
  writeString += macaddr;
  writeString += ",";
  writeString += waterLevel;              // 수위
  writeString += ",";
  writeString += temperature;             // 온도(대기,섭씨)
  writeString += ",";
  writeString += humidity;                // 습도(대기)
  writeString += ",";
  writeString += lightLevel;              // 조도(lux는 아니지만 단위는 lux)
  writeString += ",";
  writeString += SoilMValueA;             // 토양습도(실제값)
  writeString += ",";
  writeString += SoilMValueD;             // 토양습도(플래그)
  writeString + ",";
  writeString += CoDataA;                 // 일산화탄소(실제값)
  writeString += ",";
  writeString += CoDataD;                 // 일산화탄소(플래그)
  writeString += ",";
  writeString += SoilTemp;
  writeString += "\n";

  return writeString.length();
}

// Exchange Message between Serial1 and Serial3
void exChange()
{
  if (Serial3.available()) {
    Serial.write(Serial3.read());
  }
  if (Serial.available()) {
    Serial3.write(Serial.read());
  }
}

// return Temperature from Anti-Water Temperature Sensor
float getSoilTemp(boolean debug) {

  byte data[12];
  byte addr[8];

  if ( !ds.search(addr)) {
    ds.reset_search();
    return -1000;
  }

  if ( OneWire::crc8( addr, 7) != addr[7]) {
    if (debug)
      Serial.println("CRC is not valid!");

    return -1000;
  }

  if ( addr[0] != 0x10 && addr[0] != 0x28) {
    if (debug)
      Serial.print("Device is not recognized");

    return -1000;
  }

  ds.reset();
  ds.select(addr);
  ds.write(0x44, 1);   //변환

  byte present = ds.reset();
  ds.select(addr);
  ds.write(0xBE);


  for (int i = 0; i < 9; i++) {
    data[i] = ds.read();  //Scratchpad 읽음
  }

  ds.reset_search();

  byte MSB = data[1];
  byte LSB = data[0];

  float tempRead = ((MSB << 8) | LSB);
  float TemperatureSum = tempRead / 16;

  return TemperatureSum;

}

// return CO Gas level from MQ-7 Sensor
int GetCoGas(boolean debug, int datatype)
{
  MQ7.CoPwrCycler();

  /* your code here and below! */
  CoDataA = analogRead(GasPinA);
  CoDataD = digitalRead(GasPinD);

  if (MQ7.CurrentState() == LOW) { //we are at 1.4v, read sensor data!
    if (debug)
    {
      Serial.print("CO : ");
      Serial.print(CoDataA);
      Serial.print(" (");
      Serial.print(CoDataD);
      Serial.print(")");
      Serial.print("\t");
    }
  }
  else {                           //sensor is at 5v, heating time
    if (debug)
    {
      Serial.print("CO : ");
      Serial.print(CoDataA);
      Serial.print(" (");
      Serial.print(CoDataD);
      Serial.print(")");
      Serial.print(" (Heating)\t");
    }
  }

  if (datatype == 1) {
    return CoDataA;
  }
  else
  {
    return CoDataD;
  }

}

// Get temperature and humidity for Air
void GetDHTLevel(boolean debug)
{
  sensors_event_t event;
  dht.temperature().getEvent(&event);
  if (isnan(event.temperature)) {
    if (debug > 0)
      Serial.println("Error reading temperature!");
  }
  else {
    temperature = event.temperature;
    if (debug)
    {
      Serial.print("Temperature: ");
      Serial.print(temperature);
      Serial.print(" *C");
    }
  }

  dht.humidity().getEvent(&event);
  if (isnan(event.relative_humidity)) {
    if (debug)
      Serial.println("Error reading humidity!");
  }
  else {
    humidity = event.relative_humidity;
    if (debug)
    {
      Serial.print(", Humidity: ");
      Serial.print(humidity);
      Serial.println("%\t");
    }
  }
}

// adjust value to expected Range from CDS sensor.
void autoTune()
{
  if (lightLevel < low)
  {
    low = lightLevel;
  }

  if (lightLevel > high)
  {
    high = lightLevel;
  }

  lightLevel = map(lightLevel, low + 30, high - 30, 0, 255);
  lightLevel = constrain(lightLevel, 0, 255);
}

void GetCDSLevel(boolean debug)
{
  lightLevel = analogRead(CdsPin);
  autoTune();

  if (debug)
  {
    Serial.print("Lux: ");
    Serial.print(lightLevel);
    Serial.print(" Lux\t");
  }

}

void GetWaterLevel(boolean debug)
{
  waterLevel = analogRead(WaterPin);
  waterLevel /= 70;

  if (debug)
  {
    Serial.print("Water: ");
    Serial.print(waterLevel);
    Serial.print(" Cm\t");
  }
}

void GetSoilLevel(boolean debug)
{

  SoilMValueA = analogRead(SoilA);
  SoilMValueD = digitalRead(SoilD);

  /*
     수분이 없으면 아날로그 출력값은 1023, 디지털 출력값은 1이며,
     센서를 물에 깊이 담글수록 아날로그 출력값이 감소하고 디지털 출력값은 0이 된다.
  */
  if (debug)
  {
    Serial.print("Soil A:");
    Serial.print(SoilMValueA);
    Serial.print(", Soil D:");
    Serial.print(SoilMValueD);
    Serial.print("\t");
  }
}

void serialEvent() {
  while (Serial.available())
  {
    char inChar = (char) Serial.read();

    if (inChar == '\n' || inChar == '\r')
    {
      pos = inputString.indexOf("+QRY");
      if (pos > 1)
      {
        pos += 4;
        for (int i = 1; i <= 12; i++)
        {
          positions[i] = inputString.indexOf(",", pos);
          inputString.substring(",", positions[i - 1] + 1).toCharArray(cTempData, 4);
          tok[i] = atoi(cTempData);
          pos = positions[i] + 1;
        }

        inputString = "";

      }
      else
      {
        inputString = inputString + inChar;
      }
    }
  }
}
