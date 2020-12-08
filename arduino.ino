//https://robotzero.one/esp8266-and-bme280-temp-pressure-and-humidity-sensor-spi/
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <TridentTD_LineNotify.h>
#include <ESP8266HTTPClient.h>
#include "PMS.h"
#include <Wire.h>
#include <SPI.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BMP280.h>
#include "DHTesp.h"
#include <ESP8266WebServer.h>

#define SSID "BANYAI"
#define SSID2 "Tenda"
#define PASSWORD ""
#define PASSWORD2 "CyseC1234"
#define LINE_TOKEN1 "US3slxATFTwvFYA4woOuOuLi4UdOESzmW8Vdv8DXK2H" //Line Owner
#define LINE_TOKEN2 "o9Jx1QctadSHjBUGWjjDT98yQ8HxpKwU42aZzbMMc4c" //Line Family Group

HTTPClient http;

float temperature, temperature2, humidity, pressure, altitude, pm1_0, pm2_5, pm10;

String IPaddress;

// assign the ESP8266 pins to arduino pins
#define D1 5
#define D2 4
#define D4 12
#define D3 14
//#define D4 13

// assign the SPI bus to pins
#define BMP_SCK D1
#define BMP_MISO D4
#define BMP_MOSI D2
#define BMP_CS D3
//#define BMP2_CS D4

#define SEALEVELPRESSURE_HPA (1006.1)
Adafruit_BMP280 bmp(BMP_CS, BMP_MOSI, BMP_MISO, BMP_SCK); // software SPI
//Adafruit_BMP280 BMP1;// I2C


const long utcOffsetInSeconds = 25200;
char daysOfTheWeek[7][12] = {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"};

// Define NTP Client to get time
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", utcOffsetInSeconds);

PMS pms(Serial);
PMS::DATA data;

DHTesp dht;

ESP8266WebServer server(80);    

void setup()
{
  Serial.begin(9600);

  bool status;
  // default settings
  status = bmp.begin();
  if (!status) {
    Serial.println("Could not find a valid BMP280 sensor 1 , check wiring!");
  }
  
  pms.passiveMode();    // Switch to passive mode
  
  WiFi.begin(SSID, PASSWORD);
  Serial.printf("WiFi connecting to %s\n", SSID);
  int wificheck = 0;
  while (WiFi.status() != WL_CONNECTED) {
    if (wificheck <= 20) {
    Serial.print(".");
    wificheck++;
    delay(500);
    } else {
      WiFi.disconnect();
      Serial.printf("WiFi connecting to %s\n", SSID2);
      WiFi.begin(SSID2, PASSWORD2);
      while (WiFi.status() != WL_CONNECTED) {
        Serial.print(".");
        delay(500);
      }
    }
  }
  Serial.printf("\nWiFi connected\nIP : ");
  Serial.println(WiFi.localIP());
  dht.setup(D8, DHTesp::DHT22);
  timeClient.begin();
  
  server.on("/", handle_OnConnect);
  server.onNotFound(handle_NotFound);

  server.begin();
  Serial.println("HTTP server started");
}

int dht22check = 0;
int checkpm = 0;

void loop()
{
  server.handleClient();

  timeClient.update();
  Serial.print(daysOfTheWeek[timeClient.getDay()]);
  Serial.print(", ");
  Serial.print(timeClient.getHours());
  Serial.print(":");
  Serial.print(timeClient.getMinutes());
  Serial.print(":");
  Serial.println(timeClient.getSeconds());
  Serial.println(timeClient.getFormattedTime());

  float h = dht.getHumidity();
  float t = dht.getTemperature();
  float f = dht.toFahrenheit(t);
  float hic = dht.computeHeatIndex(t, h);
  float hif = dht.computeHeatIndex(f, h, true);

  if (dht22check == 0) {
    LINE.notify("DHT22 Status = " + String (dht.getStatusString()));
    dht22check++;
  }

  Serial.println("Status\t\tHumidity (%)\tTemperature (°C)\t(°F)");
  Serial.print(dht.getStatusString());
  Serial.print("\t\t");
  Serial.print(h, 1);
  Serial.print("\t\t\t");
  Serial.print(t, 1);
  Serial.print("\t\t\t\t");
  Serial.println(f, 1);
  Serial.print("Heat index (ดัชนีความร้อน) อุณหภูมิที่รู้สึกได้จริง : ");
  Serial.print(hic, 1);
  Serial.print(" °C (");
  Serial.print(hif, 1);
  Serial.println(" °F)");

  Serial.print("Temperature = ");
  Serial.print(bmp.readTemperature(), 1);
  Serial.print(" °C (");  
  Serial.print(dht.toFahrenheit(bmp.readTemperature()), 1);
  Serial.println(" °F)");
  Serial.print("Heat index (ดัชนีความร้อน) อุณหภูมิที่รู้สึกได้จริง : ");
  Serial.print(dht.computeHeatIndex(bmp.readTemperature(), h),1);
  Serial.print(" °C (");
  Serial.print(dht.computeHeatIndex(dht.toFahrenheit(bmp.readTemperature()), h, true), 1);
  Serial.println(" °F)");
  Serial.print("Pressure = ");
  Serial.print(bmp.readPressure() / 100.0F);
  Serial.println(" hPa");

  Serial.print("Approx. Altitude = ");
  Serial.print(bmp.readAltitude(SEALEVELPRESSURE_HPA));
  Serial.println(" m");
  Serial.println();
  
  if ((timeClient.getMinutes() % 5) == 0 && checkpm == 0) {
  
    Serial.println("Waking up, wait 30 seconds for stable readings...");
    pms.wakeUp();
    delay(30000);

    Serial.println("Send read request...");
    pms.requestRead();

    Serial.println("Reading data...");
    if (pms.readUntil(data))
    {
      Serial.print("PM 1.0 (ug/m3): ");
      Serial.println(data.PM_AE_UG_1_0);

      Serial.print("PM 2.5 (ug/m3): ");
      Serial.println(data.PM_AE_UG_2_5);

      Serial.print("PM 10.0 (ug/m3): ");
      Serial.println(data.PM_AE_UG_10_0);
      
      Serial.println("Going to sleep for 5 minutes.");
      pms.sleep();
    } else {
      Serial.println("No data from PMS Sensor.");
    }

}

  if ((timeClient.getMinutes() % 5) == 1 && checkpm == 1) {
  checkpm = 0;
  }
  delay(5000);
}

void handle_OnConnect() {
  temperature = bmp.readTemperature();
  temperature2 = dht.getTemperature();
  humidity = dht.getHumidity();
  pressure = bmp.readPressure() / 100.0F;
  altitude = bmp.readAltitude(SEALEVELPRESSURE_HPA);
  pm1_0 = data.PM_AE_UG_1_0;
  pm2_5 = data.PM_AE_UG_2_5;
  pm10 = data.PM_AE_UG_10_0;
  server.send(200, "text/html", SendHTML(temperature,temperature2,humidity,pressure,altitude,pm1_0,pm2_5,pm10)); 
}

void handle_NotFound(){
  server.send(404, "text/plain", "Not found");
}

String SendHTML(float temperature,float temperature2,float humidity,float pressure,float altitude,float pm1_0,float pm2_5,float pm10_0){
  String ptr = "<!DOCTYPE html>";
  ptr +="<html>";
  ptr +="<head>";
  ptr +="<title>Weather Station</title>";
  ptr +="<meta name='viewport' content='width=device-width, initial-scale=1.0'>";
  ptr +="<link href='https://fonts.googleapis.com/css?family=Open+Sans:300,400,600' rel='stylesheet'>";
  ptr +="<style>";
  ptr +="html { font-family: 'Open Sans', sans-serif; display: block; margin: 0px auto; text-align: center;color: #444444;}";
  ptr +="body{margin: 0px;} ";
  ptr +="h1 {margin: 50px auto 30px;} ";
  ptr +=".side-by-side{display: table-cell;vertical-align: middle;position: relative;}";
  ptr +=".text{font-weight: 600;font-size: 19px;width: 200px;}";
  ptr +=".reading{font-weight: 300;font-size: 50px;padding-right: 25px;}";
  ptr +=".temperature .reading{color: #F29C1F;}";
  ptr +=".temperature2 .reading{color: #F29C1F;}";
  ptr +=".humidity .reading{color: #3B97D3;}";
  ptr +=".pressure .reading{color: #26B99A;}";
  ptr +=".altitude .reading{color: #955BA5;}";
  ptr +=".pm1_0 .reading{color: #3d3d5c;}";
  ptr +=".pm2_5 .reading{color: #3d3d5c;}";
  ptr +=".pm10 .reading{color: #3d3d5c;}";
  ptr +=".superscript{font-size: 17px;font-weight: 600;position: absolute;top: 10px;}";
  ptr +=".data{padding: 10px;}";
  ptr +=".container{display: table;margin: 0 auto;}";
  ptr +=".icon{width:65px}";
  ptr +="</style>";
  ptr +="</head>";
  ptr +="<body>";
  ptr +="<h1>ESP8266 Weather Station</h1>";
  ptr +="<div class='container'>";
  ptr +="<div class='data temperature'>";
  ptr +="<div class='side-by-side icon'>";
  ptr +="<svg enable-background='new 0 0 19.438 54.003'height=54.003px id=Layer_1 version=1.1 viewBox='0 0 19.438 54.003'width=19.438px x=0px xml:space=preserve xmlns=http://www.w3.org/2000/svg xmlns:xlink=http://www.w3.org/1999/xlink y=0px><g><path d='M11.976,8.82v-2h4.084V6.063C16.06,2.715,13.345,0,9.996,0H9.313C5.965,0,3.252,2.715,3.252,6.063v30.982";
  ptr +="C1.261,38.825,0,41.403,0,44.286c0,5.367,4.351,9.718,9.719,9.718c5.368,0,9.719-4.351,9.719-9.718";
  ptr +="c0-2.943-1.312-5.574-3.378-7.355V18.436h-3.914v-2h3.914v-2.808h-4.084v-2h4.084V8.82H11.976z M15.302,44.833";
  ptr +="c0,3.083-2.5,5.583-5.583,5.583s-5.583-2.5-5.583-5.583c0-2.279,1.368-4.236,3.326-5.104V24.257C7.462,23.01,8.472,22,9.719,22";
  ptr +="s2.257,1.01,2.257,2.257V39.73C13.934,40.597,15.302,42.554,15.302,44.833z'fill=#F29C21 /></g></svg>";
  ptr +="</div>";
  ptr +="<div class='side-by-side text'>Temperature</div>";
  ptr +="<div class='side-by-side reading'>";
  ptr +=(int)temperature;
  ptr +="<span class='superscript'>&deg;C</span></div>";
  ptr +="</div>";
  ptr +="<div class='data temperature 2'>";
  ptr +="<div class='side-by-side icon'>";
  ptr +="<svg enable-background='new 0 0 19.438 54.003'height=54.003px id=Layer_1 version=1.1 viewBox='0 0 19.438 54.003'width=19.438px x=0px xml:space=preserve xmlns=http://www.w3.org/2000/svg xmlns:xlink=http://www.w3.org/1999/xlink y=0px><g><path d='M11.976,8.82v-2h4.084V6.063C16.06,2.715,13.345,0,9.996,0H9.313C5.965,0,3.252,2.715,3.252,6.063v30.982";
  ptr +="C1.261,38.825,0,41.403,0,44.286c0,5.367,4.351,9.718,9.719,9.718c5.368,0,9.719-4.351,9.719-9.718";
  ptr +="c0-2.943-1.312-5.574-3.378-7.355V18.436h-3.914v-2h3.914v-2.808h-4.084v-2h4.084V8.82H11.976z M15.302,44.833";
  ptr +="c0,3.083-2.5,5.583-5.583,5.583s-5.583-2.5-5.583-5.583c0-2.279,1.368-4.236,3.326-5.104V24.257C7.462,23.01,8.472,22,9.719,22";
  ptr +="s2.257,1.01,2.257,2.257V39.73C13.934,40.597,15.302,42.554,15.302,44.833z'fill=#F29C21 /></g></svg>";
  ptr +="</div>";
  ptr +="<div class='side-by-side text'>Temperature 2</div>";
  ptr +="<div class='side-by-side reading'>";
  ptr +=(int)temperature2;
  ptr +="<span class='superscript'>&deg;C</span></div>";
  ptr +="</div>";
  ptr +="<div class='data humidity'>";
  ptr +="<div class='side-by-side icon'>";
  ptr +="<svg enable-background='new 0 0 29.235 40.64'height=40.64px id=Layer_1 version=1.1 viewBox='0 0 29.235 40.64'width=29.235px x=0px xml:space=preserve xmlns=http://www.w3.org/2000/svg xmlns:xlink=http://www.w3.org/1999/xlink y=0px><path d='M14.618,0C14.618,0,0,17.95,0,26.022C0,34.096,6.544,40.64,14.618,40.64s14.617-6.544,14.617-14.617";
  ptr +="C29.235,17.95,14.618,0,14.618,0z M13.667,37.135c-5.604,0-10.162-4.56-10.162-10.162c0-0.787,0.638-1.426,1.426-1.426";
  ptr +="c0.787,0,1.425,0.639,1.425,1.426c0,4.031,3.28,7.312,7.311,7.312c0.787,0,1.425,0.638,1.425,1.425";
  ptr +="C15.093,36.497,14.455,37.135,13.667,37.135z'fill=#3C97D3 /></svg>";
  ptr +="</div>";
  ptr +="<div class='side-by-side text'>Humidity</div>";
  ptr +="<div class='side-by-side reading'>";
  ptr +=(int)humidity;
  ptr +="<span class='superscript'>%</span></div>";
  ptr +="</div>";
  ptr +="<div class='data pressure'>";
  ptr +="<div class='side-by-side icon'>";
  ptr +="<svg enable-background='new 0 0 40.542 40.541'height=40.541px id=Layer_1 version=1.1 viewBox='0 0 40.542 40.541'width=40.542px x=0px xml:space=preserve xmlns=http://www.w3.org/2000/svg xmlns:xlink=http://www.w3.org/1999/xlink y=0px><g><path d='M34.313,20.271c0-0.552,0.447-1,1-1h5.178c-0.236-4.841-2.163-9.228-5.214-12.593l-3.425,3.424";
  ptr +="c-0.195,0.195-0.451,0.293-0.707,0.293s-0.512-0.098-0.707-0.293c-0.391-0.391-0.391-1.023,0-1.414l3.425-3.424";
  ptr +="c-3.375-3.059-7.776-4.987-12.634-5.215c0.015,0.067,0.041,0.13,0.041,0.202v4.687c0,0.552-0.447,1-1,1s-1-0.448-1-1V0.25";
  ptr +="c0-0.071,0.026-0.134,0.041-0.202C14.39,0.279,9.936,2.256,6.544,5.385l3.576,3.577c0.391,0.391,0.391,1.024,0,1.414";
  ptr +="c-0.195,0.195-0.451,0.293-0.707,0.293s-0.512-0.098-0.707-0.293L5.142,6.812c-2.98,3.348-4.858,7.682-5.092,12.459h4.804";
  ptr +="c0.552,0,1,0.448,1,1s-0.448,1-1,1H0.05c0.525,10.728,9.362,19.271,20.22,19.271c10.857,0,19.696-8.543,20.22-19.271h-5.178";
  ptr +="C34.76,21.271,34.313,20.823,34.313,20.271z M23.084,22.037c-0.559,1.561-2.274,2.372-3.833,1.814";
  ptr +="c-1.561-0.557-2.373-2.272-1.815-3.833c0.372-1.041,1.263-1.737,2.277-1.928L25.2,7.202L22.497,19.05";
  ptr +="C23.196,19.843,23.464,20.973,23.084,22.037z'fill=#26B999 /></g></svg>";
  ptr +="</div>";
  ptr +="<div class='side-by-side text'>Pressure</div>";
  ptr +="<div class='side-by-side reading'>";
  ptr +=(int)pressure;
  ptr +="<span class='superscript'>hPa</span></div>";
  ptr +="</div>";
  ptr +="<div class='data altitude'>";
  ptr +="<div class='side-by-side icon'>";
  ptr +="<svg enable-background='new 0 0 58.422 40.639'height=40.639px id=Layer_1 version=1.1 viewBox='0 0 58.422 40.639'width=58.422px x=0px xml:space=preserve xmlns=http://www.w3.org/2000/svg xmlns:xlink=http://www.w3.org/1999/xlink y=0px><g><path d='M58.203,37.754l0.007-0.004L42.09,9.935l-0.001,0.001c-0.356-0.543-0.969-0.902-1.667-0.902";
  ptr +="c-0.655,0-1.231,0.32-1.595,0.808l-0.011-0.007l-0.039,0.067c-0.021,0.03-0.035,0.063-0.054,0.094L22.78,37.692l0.008,0.004";
  ptr +="c-0.149,0.28-0.242,0.594-0.242,0.934c0,1.102,0.894,1.995,1.994,1.995v0.015h31.888c1.101,0,1.994-0.893,1.994-1.994";
  ptr +="C58.422,38.323,58.339,38.024,58.203,37.754z'fill=#955BA5 /><path d='M19.704,38.674l-0.013-0.004l13.544-23.522L25.13,1.156l-0.002,0.001C24.671,0.459,23.885,0,22.985,0";
  ptr +="c-0.84,0-1.582,0.41-2.051,1.038l-0.016-0.01L20.87,1.114c-0.025,0.039-0.046,0.082-0.068,0.124L0.299,36.851l0.013,0.004";
  ptr +="C0.117,37.215,0,37.62,0,38.059c0,1.412,1.147,2.565,2.565,2.565v0.015h16.989c-0.091-0.256-0.149-0.526-0.149-0.813";
  ptr +="C19.405,39.407,19.518,39.019,19.704,38.674z'fill=#955BA5 /></g></svg>";
  ptr +="</div>";
  ptr +="<div class='side-by-side text'>Altitude</div>";
  ptr +="<div class='side-by-side reading'>";
  ptr +=(int)altitude;
  ptr +="<span class='superscript'>m</span></div>";
  ptr +="</div>";
  
  ptr +="<div class='data pm1_0'>";
  ptr +="<div class='side-by-side icon'>";
  ptr +="<svg class='svg-icon' viewBox='0 0 20 20'>";
  ptr +="<path fill=#3d3d5c d='M8.652,16.404c-0.186,0-0.337,0.151-0.337,0.337v2.022c0,0.186,0.151,0.337,0.337,0.337s0.337-0.151,0.337-0.337v-2.022C8.989,16.555,8.838,16.404,8.652,16.404z'></path>";
  ptr +="<path fill=#3d3d5c d='M11.348,16.404c-0.186,0-0.337,0.151-0.337,0.337v2.022c0,0.186,0.151,0.337,0.337,0.337s0.337-0.151,0.337-0.337v-2.022C11.685,16.555,11.535,16.404,11.348,16.404z'></path>";
  ptr +="<path fill=#3d3d5c d='M17.415,5.281V4.607c0-2.224-1.847-4.045-4.103-4.045H10H6.687c-2.256,0-4.103,1.82-4.103,4.045v0.674H10H17.415z'></path>";
  ptr +="<path fill=#3d3d5c d='M18.089,10.674V7.304c0,0,0-0.674-0.674-0.674V5.955H10H2.585v0.674c-0.674,0-0.674,0.674-0.674,0.674v3.371c-0.855,0.379-1.348,1.084-1.348,2.022c0,1.253,2.009,3.008,2.009,3.371c0,2.022,1.398,3.371,3.436,3.371c0.746,0,1.43-0.236,1.98-0.627c-0.001-0.016-0.009-0.03-0.009-0.047v-2.022c0-0.372,0.303-0.674,0.674-0.674c0.301,0,0.547,0.201,0.633,0.474h0.041v-0.137c0-0.372,0.303-0.674,0.674-0.674s0.674,0.302,0.674,0.674v0.137h0.041c0.086-0.273,0.332-0.474,0.633-0.474c0.371,0,0.674,0.302,0.674,0.674v2.022c0,0.016-0.008,0.03-0.009,0.047c0.55,0.391,1.234,0.627,1.98,0.627c2.039,0,3.436-1.348,3.436-3.371c0-0.362,2.009-2.118,2.009-3.371C19.438,11.758,18.944,11.053,18.089,10.674z M5.618,18.089c-0.558,0-1.011-0.453-1.011-1.011s0.453-1.011,1.011-1.011s1.011,0.453,1.011,1.011S6.177,18.089,5.618,18.089z M6.629,13.371H5.474c-0.112,0-0.192-0.061-0.192-0.135c0-0.074,0.08-0.151,0.192-0.174l1.156-0.365V13.371z M8.652,12.521c-0.394,0.163-0.774,0.366-1.148,0.55c-0.061,0.03-0.132,0.052-0.2,0.076v-0.934c0.479-0.411,0.906-0.694,1.348-0.879V12.521z M5.281,10c-1.348,0-1.348-2.696-1.348-2.696h5.393C9.326,7.304,6.629,10,5.281,10z M10.674,12.296c-0.22-0.053-0.444-0.084-0.674-0.084s-0.454,0.032-0.674,0.084v-1.168C9.539,11.086,9.762,11.06,10,11.05c0.238,0.01,0.461,0.036,0.674,0.078V12.296z M12.696,13.146c-0.068-0.024-0.14-0.046-0.2-0.076c-0.374-0.184-0.754-0.386-1.148-0.55v-1.188c0.442,0.185,0.87,0.467,1.348,0.879V13.146zM14.382,18.089c-0.558,0-1.011-0.453-1.011-1.011s0.453-1.011,1.011-1.011c0.558,0,1.011,0.453,1.011,1.011S14.94,18.089,14.382,18.089z M13.371,13.371v-0.674l1.156,0.365c0.112,0.022,0.192,0.099,0.192,0.174c0,0.074-0.08,0.135-0.192,0.135H13.371z M14.719,10c-1.348,0-4.045-2.696-4.045-2.696h5.393C16.067,7.304,16.067,10,14.719,10z'></path>";
  ptr +="<path fill=#3d3d5c d='M10,16.067c-0.186,0-0.337,0.151-0.337,0.337V19.1c0,0.186,0.151,0.337,0.337,0.337s0.337-0.151,0.337-0.337v-2.696C10.337,16.218,10.186,16.067,10,16.067z'></path>";
  ptr +="</svg>";
  ptr +="</div>";
  ptr +="<div class='side-by-side text'>PM 1.0</div>";
  ptr +="<div class='side-by-side reading'>";
  ptr +=(int)pm1_0;
  ptr +="<span class='superscript'>ug/m3</span></div>";
  ptr +="</div>";

  ptr +="<div class='data pm2_5'>";
  ptr +="<div class='side-by-side icon'>";
  ptr +="<svg height='100%' viewBox='0 0 512 512' width='100%' xmlns='http://www.w3.org/2000/svg'><g id='filled_outline' data-name='filled outline'><path d='m256 80a120 120 0 0 1 120 120v56a0 0 0 0 1 0 0h-240a0 0 0 0 1 0 0v-56a120 120 0 0 1 120-120z' fill='#533222'/><path d='m144 416h224a72 72 0 0 1 72 72 0 0 0 0 1 0 0h-368a0 0 0 0 1 0 0 72 72 0 0 1 72-72z' fill='#0296e5'/><path d='m296 400v-40h-80v40a16 16 0 0 1 -16 16 40 40 0 0 0 40 40h32a40 40 0 0 0 40-40 16 16 0 0 1 -16-16z' fill='#fdc9a6'/><path d='m296 360v22.4a88.133 88.133 0 0 1 -80 0v-22.4z' fill='#fdc9a6'/><path d='m88 320a23.916 23.916 0 0 0 -9.008 1.755 32 32 0 1 0 0 44.49 24 24 0 1 0 9.008-46.245z' fill='#656666'/><path d='m120 40a32 32 0 0 0 -8.96 1.281 24 24 0 0 0 -42.826-6.862 32 32 0 1 0 -12.214 61.581c.381 0 .759-.016 1.137-.029 6.593 4.929 16.182 8.029 26.863 8.029a49.275 49.275 0 0 0 20.242-4.153 32 32 0 1 0 15.758-59.847z' fill='#cbcbcb'/><path d='m256 376a88 88 0 0 1 -88-88v-96a42.388 42.388 0 0 0 36.348-20.58l11.652-19.42a136.567 136.567 0 0 0 96.568 40h31.432v96a88 88 0 0 1 -88 88z' fill='#fdc9a6'/><path d='m296 360v22.4a88.133 88.133 0 0 1 -80 0v-22.4z' fill='#f6bb92'/><path d='m344 288h-176a8 8 0 0 0 0 16h176a8 8 0 0 0 0-16z' fill='#fbb540'/><circle cx='216' cy='240' fill='#533222' r='16'/><circle cx='296' cy='240' fill='#533222' r='16'/><path d='m168 224a0 0 0 0 1 0 0v64a0 0 0 0 1 0 0 32 32 0 0 1 -32-32 32 32 0 0 1 32-32z' fill='#fdc9a6'/><path d='m376 224a0 0 0 0 1 0 0v64a0 0 0 0 1 0 0 32 32 0 0 1 -32-32 32 32 0 0 1 32-32z' fill='#fdc9a6' transform='matrix(-1 0 0 -1 720 512)'/><path d='m224 232h-32a8 8 0 0 1 0-16h28.687l5.656-5.657a8 8 0 0 1 11.314 11.314l-8 8a8 8 0 0 1 -5.657 2.343z' fill='#533222'/><path d='m320 232h-32a8 8 0 0 1 -5.657-2.343l-8-8a8 8 0 0 1 11.314-11.314l5.656 5.657h28.687a8 8 0 0 1 0 16z' fill='#533222'/><path d='m120 488v-24a8 8 0 0 1 16 0v24z' fill='#026ca2'/><path d='m376 488v-24a8 8 0 0 1 16 0v24z' fill='#026ca2'/><path d='m479.914 350.041a23.994 23.994 0 0 0 -32.922-20.286 32 32 0 1 0 -4.7 48.492 24 24 0 1 0 37.619-28.206z' fill='#cbcbcb'/><path d='m471.25 73.117a32.062 32.062 0 0 0 -18.1-22.293 26.637 26.637 0 0 0 -52.61-2.809c-.18 0-.357-.015-.538-.015a24 24 0 1 0 10.926 45.366 31.96 31.96 0 0 0 34.747 18.122 24 24 0 1 0 25.575-38.371z' fill='#656666'/><circle cx='416' cy='256' fill='#cbcbcb' r='8'/><circle cx='152' cy='368' fill='#656666' r='8'/><circle cx='360' cy='352' fill='#cbcbcb' r='8'/><circle cx='408' cy='144' fill='#cbcbcb' r='8'/><circle cx='456' cy='208' fill='#656666' r='8'/><circle cx='64' cy='216' fill='#656666' r='8'/><circle cx='104' cy='280' fill='#cbcbcb' r='8'/><circle cx='32' cy='280' fill='#cbcbcb' r='8'/><circle cx='80' cy='72' fill='#a4a4a4' r='8'/><circle cx='104' cy='168' fill='#cbcbcb' r='8'/><circle cx='472' cy='272' fill='#cbcbcb' r='8'/><circle cx='264' cy='48' fill='#656666' r='8'/><path d='m312 360-19.367 8.3a92.993 92.993 0 0 1 -73.266 0l-19.367-8.3v-64l23.033-6.581a120.006 120.006 0 0 1 65.934 0l23.033 6.581z' fill='#04957c'/><path d='m280 320h-48a8 8 0 0 1 0-16h48a8 8 0 0 1 0 16z' fill='#2ad3b3'/><path d='m280 352h-48a8 8 0 0 1 0-16h48a8 8 0 0 1 0 16z' fill='#2ad3b3'/><path d='m368 408h-56a8.009 8.009 0 0 1 -8-8v-27.9c.81-.327 1.618-.656 2.422-1l8.729-3.741a8 8 0 0 0 4.849-7.359v-.456a95.671 95.671 0 0 0 31.722-64.3 40.063 40.063 0 0 0 32.278-39.244v-56a128 128 0 0 0 -256 0v56a40.063 40.063 0 0 0 32.278 39.245 95.639 95.639 0 0 0 31.722 64.299v.456a8 8 0 0 0 4.849 7.353l8.729 3.741c.842.361 1.689.707 2.537 1.049-.07.424-.115 27.857-.115 27.857a8.009 8.009 0 0 1 -8 8h-56a80.091 80.091 0 0 0 -80 80 8 8 0 0 0 8 8h368a8 8 0 0 0 8-8 80.091 80.091 0 0 0 -80-80zm-64-53.275-3.881 1.663a112.5 112.5 0 0 1 -88.238 0l-3.85-1.65c-.011-.071-.018-.143-.031-.213v-52.491l17.231-4.923a111.885 111.885 0 0 1 61.538 0l17.231 4.923zm-128-130.725v-24.648a50.691 50.691 0 0 0 35.208-23.815l6.565-10.943a143.5 143.5 0 0 0 94.795 35.406h23.432v88h-22.878l-21.957-6.273a127.885 127.885 0 0 0 -70.33 0l-21.957 6.273h-22.878zm144 112.009v-32.009h14.391a79.611 79.611 0 0 1 -14.391 32.009zm32-57.38v-45.258a24 24 0 0 1 0 45.258zm-96-190.629a112.127 112.127 0 0 1 112 112v24.022a39.835 39.835 0 0 0 -16-7.217v-24.805a8 8 0 0 0 -8-8h-31.432a127.726 127.726 0 0 1 -90.911-37.657 8 8 0 0 0 -12.517 1.541l-11.653 19.421a34.558 34.558 0 0 1 -29.487 16.695 8 8 0 0 0 -8 8v24.805a39.841 39.841 0 0 0 -16 7.217v-24.022a112.127 112.127 0 0 1 112-112zm-112 168a24.042 24.042 0 0 1 16-22.629v45.258a24.042 24.042 0 0 1 -16-22.629zm33.608 48h14.392v32.011a79.576 79.576 0 0 1 -14.392-32.011zm78.392 77.394a128.79 128.79 0 0 0 32-4.033v22.639a24.041 24.041 0 0 0 15.358 22.392 32.054 32.054 0 0 1 -31.358 25.608h-32a32.054 32.054 0 0 1 -31.358-25.608 24.041 24.041 0 0 0 15.358-22.392v-22.639a128.79 128.79 0 0 0 32 4.033zm136 98.606v-16a8 8 0 0 0 -16 0v16h-240v-16a8 8 0 0 0 -16 0v16h-39.5a64.1 64.1 0 0 1 63.5-56h48.679a48.069 48.069 0 0 0 47.321 40h32a48.069 48.069 0 0 0 47.321-40h48.679a64.1 64.1 0 0 1 63.5 56z'/><path d='m88 376a32 32 0 1 0 -7-63.228 39.613 39.613 0 0 0 -25-8.772 40 40 0 0 0 0 80 39.613 39.613 0 0 0 25-8.772 31.95 31.95 0 0 0 7 .772zm-14.756-15.318a24 24 0 1 1 0-33.364 8 8 0 0 0 8.756 1.85 16 16 0 1 1 0 29.664 8 8 0 0 0 -8.756 1.85z'/><path d='m54.646 103.978c8.024 5.189 18.33 8.022 29.354 8.022a58.674 58.674 0 0 0 19.9-3.384 40 40 0 1 0 11.923-76.4 31.982 31.982 0 0 0 -50.234-7.058 40 40 0 1 0 -10.946 78.817zm1.354-63.978a23.868 23.868 0 0 1 9.158 1.812 8 8 0 0 0 9.648-2.859 16 16 0 0 1 28.551 4.564 8 8 0 0 0 9.919 5.445 24 24 0 1 1 -5.087 43.927 8 8 0 0 0 -7.255-.326 41.239 41.239 0 0 1 -16.934 3.437c-8.553 0-16.6-2.347-22.075-6.438a7.952 7.952 0 0 0 -5.068-1.586l-.159.006c-.231.008-.464.018-.7.018a24 24 0 0 1 0-48z'/><path d='m280 304h-48a8 8 0 0 0 0 16h48a8 8 0 0 0 0-16z'/><path d='m280 336h-48a8 8 0 0 0 0 16h48a8 8 0 0 0 0-16z'/><path d='m202.158 232a16 16 0 1 0 26.622-1.6 7.966 7.966 0 0 0 .877-.746l8-8a8 8 0 0 0 -11.314-11.314l-5.656 5.66h-28.687a8 8 0 0 0 0 16z'/><path d='m282.343 229.657a8.06 8.06 0 0 0 .877.746 16 16 0 1 0 26.622 1.6h10.158a8 8 0 0 0 0-16h-28.687l-5.656-5.657a8 8 0 0 0 -11.314 11.314z'/><path d='m456 320a31.95 31.95 0 0 0 -7 .772 39.613 39.613 0 0 0 -25-8.772 40 40 0 1 0 15.7 76.8 31.991 31.991 0 1 0 47.78-42.535 32.19 32.19 0 0 0 -31.48-26.265zm8 64a16.083 16.083 0 0 1 -14.473-9.173 8 8 0 0 0 -11.815-3.138 24 24 0 1 1 3.532-36.371 8 8 0 0 0 8.755 1.85 15.9 15.9 0 0 1 6.001-1.168 16.071 16.071 0 0 1 15.94 14.687 8 8 0 0 0 2.665 5.34 15.995 15.995 0 0 1 -10.605 27.973z'/><path d='m400 104a32.036 32.036 0 0 0 7.378-.866 40.052 40.052 0 0 0 35.353 16.77 32 32 0 1 0 35.149-52.749 40.263 40.263 0 0 0 -17.48-21.555 34.646 34.646 0 0 0 -65.917-5.12 32 32 0 0 0 5.517 63.52zm.018-48 .341.011a7.965 7.965 0 0 0 8.014-6.4 18.638 18.638 0 0 1 36.808 1.965 8 8 0 0 0 4.671 6.537 24.1 24.1 0 0 1 13.581 16.716 8 8 0 0 0 5.4 5.913 16 16 0 1 1 -17.056 25.577 8 8 0 0 0 -6.106-2.833 7.917 7.917 0 0 0 -1.411.126 24 24 0 0 1 -26.068-13.592 8 8 0 0 0 -10.915-3.773 15.81 15.81 0 0 1 -7.277 1.753 16 16 0 1 1 .018-32z'/><circle cx='416' cy='256' r='8'/><circle cx='152' cy='368' r='8'/><circle cx='360' cy='352' r='8'/><circle cx='408' cy='144' r='8'/><circle cx='456' cy='208' r='8'/><circle cx='64' cy='216' r='8'/><circle cx='104' cy='280' r='8'/><circle cx='32' cy='280' r='8'/><circle cx='80' cy='72' r='8'/><circle cx='104' cy='168' r='8'/><circle cx='472' cy='272' r='8'/><circle cx='264' cy='48' r='8'/></g></svg>";
  ptr +="</div>";
  ptr +="<div class='side-by-side text'>PM 2.5</div>";
  ptr +="<div class='side-by-side reading'>";
  ptr +=(int)pm2_5;
  ptr +="<span class='superscript'>ug/m3</span></div>";
  ptr +="</div>";
  
  ptr +="<div class='data pm10'>";
  ptr +="<div class='side-by-side icon'>";
  ptr +="<svg id='_x31_-outline-expand' enable-background='new 0 0 64 64' height='100%' viewBox='0 0 64 64' width='100%' xmlns='http://www.w3.org/2000/svg'><path d='m32 62h-29v-1c.62-1.86 1.71-3.48 3.12-4.74 1.42-1.26 3.16-2.16 5.07-2.56l2.2-.46 7.43-1.57c1.85-.39 3.18-2.02 3.18-3.92v-2.64h16v2.64c0 1.9 1.33 3.53 3.18 3.92l7.43 1.57 2.2.46c3.83.8 6.95 3.58 8.19 7.3v1z' fill='#4f5559'/><path d='m43.18 51.67 7.43 1.57c-3.9 4.06-10.78 6.76-18.61 6.76s-14.71-2.7-18.61-6.76l7.43-1.57c1.85-.39 3.18-2.02 3.18-3.92v-2.64h16v2.64c0 1.9 1.33 3.53 3.18 3.92' fill='#f2daae'/><path d='m50 22v7c0 9.94-8.06 18-18 18s-18-8.06-18-18v-7-1c0-9.94 8.06-18 18-18 4.97 0 9.47 2.02 12.73 5.27 3.25 3.26 5.27 7.76 5.27 12.73z' fill='#f2daae'/><path d='m44 31-12-2-12 2v12c7.111 5.333 16.889 5.333 24 0z' fill='#62c4c3'/><path d='m12 29h2v-8h-2c-2.209 0-4 1.791-4 4s1.791 4 4 4' fill='#e0c485'/><path d='m50 12h-2.428c1.243 2.146 2.07 4.563 2.334 7.153.059.273.094.556.094.847v1 8 7c0 6.075 4.925 11 11 11v-24c0-6.075-4.925-11-11-11' fill='#aa494a'/><path d='m52 21h-2v8h2c2.209 0 4-1.791 4-4s-1.791-4-4-4' fill='#e0c485'/><path d='m50 21v1c-12.71 0-23-4.92-23-11 0 6.08-5.81 11-13 11v-1c0-9.94 8.06-18 18-18 4.97 0 9.47 2.02 12.73 5.27 3.25 3.26 5.27 7.76 5.27 12.73' fill='#d36767'/><g fill='#3a4249'><path d='m44.6 43.8c.252-.189.4-.485.4-.8v-.197c1.699-1.601 3.068-3.481 4.069-5.538.634 6.024 5.742 10.735 11.931 10.735.553 0 1-.448 1-1v-24c0-6.617-5.383-12-12-12h-1.875c-3.356-5.392-9.32-9-16.125-9-10.139 0-18.424 7.989-18.949 18h-1.051c-2.757 0-5 2.243-5 5s2.243 5 5 5h1.054c.26 4.852 2.382 9.443 5.946 12.803v.197c0 .315.148.611.4.8 1.136.852 2.347 1.557 3.6 2.149v1.805c0 1.411-1.001 2.645-2.382 2.936l-9.635 2.029c-4.163.876-7.586 3.928-8.931 7.965-.035.102-.052.209-.052.316v1h2v-.833c1.156-3.292 3.976-5.771 7.395-6.491l1.675-.353c4.295 4.187 11.314 6.677 18.93 6.677 7.617 0 14.636-2.49 18.93-6.677l1.675.353c3.42.72 6.24 3.199 7.395 6.491v.833h2v-1c0-.107-.017-.214-.051-.316-1.346-4.037-4.768-7.089-8.932-7.965l-9.635-2.029c-1.38-.291-2.382-1.525-2.382-2.936v-1.805c1.253-.592 2.464-1.297 3.6-2.149m-24.208-13.879-5.392-6.291v-.673c5.699-.348 10.457-3.624 12.238-8.085 3.032 4.587 11.348 7.866 21.524 8.113l-5.197 6.929-11.4-1.9c-.11-.019-.22-.019-.329 0zm31.608-7.921c1.655 0 3 1.346 3 3s-1.345 3-3 3h-1v-6zm-7 17.891v-8.558l4-5.333v3c0 3.997-1.447 7.846-4 10.891m5-26.891c5.514 0 10 4.486 10 10v22.95c-5.046-.503-9-4.774-9-9.95v-6h1c2.757 0 5-2.243 5-5s-2.243-5-5-5h-1.051c-.13-2.491-.736-4.852-1.739-7zm-18-9c9.37 0 16.993 7.62 17 16.988-11.483-.247-21-4.72-21-9.988h-2c0 5.232-4.849 9.531-10.998 9.958.024-9.354 7.638-16.958 16.998-16.958m-23 21c0-1.654 1.346-3 3-3h1v6h-1c-1.654 0-3-1.346-3-3m6 4v-2.297l4 4.667v8.521c-2.553-3.045-4-6.894-4-10.891m6 11.22 3.804.76.393-1.96-4.197-.84v-2.36l4.197-.84-.393-1.96-3.804.76v-1.933l11-1.833 11 1.833v1.933l-3.803-.76-.393 1.96 4.196.84v2.36l-4.196.84.393 1.96 3.803-.76v2.272c-6.547 4.645-15.453 4.645-22 0zm21.97 12.427 5.538 1.166c-3.98 3.271-9.991 5.187-16.508 5.187s-12.527-1.916-16.508-5.187l5.539-1.166c2.3-.484 3.969-2.542 3.969-4.893v-.982c2.261.792 4.628 1.201 7 1.201s4.74-.409 7-1.201v.982c0 2.351 1.67 4.409 3.97 4.893'/><path d='m25 24c-1.104 0-2 .896-2 2s.896 2 2 2 2-.896 2-2-.896-2-2-2'/><path d='m39 28c1.104 0 2-.896 2-2s-.896-2-2-2-2 .896-2 2 .896 2 2 2'/></g></svg>";
  ptr +="</div>";
  ptr +="<div class='side-by-side text'>PM 10</div>";
  ptr +="<div class='side-by-side reading'>";
  ptr +=(int)pm10;
  ptr +="<span class='superscript'>ug/m3</span></div>";
  ptr +="</div>";
  
  ptr +="</div>";
  ptr +="</body>";
  ptr +="</html>";
  return ptr;
}
