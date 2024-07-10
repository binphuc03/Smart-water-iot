#include <DHT.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <WiFi.h>
#include <Firebase_ESP_Client.h>
#include <BH1750.h>
#include <WiFiUdp.h>
#include <ArduinoJson.h>
#include <NTPClient.h>
#include <TimeLib.h>
#include <Arduino.h>
#include<WiFi.h>
#include<WiFiManager.h>
#include<WiFiClientSecure.h>
#define DATABASE_URL "https://smartwater-fe007-default-rtdb.asia-southeast1.firebasedatabase.app"  // the project name address from firebase id
#define DATABASE_SECRET "c5d5bRuTW3b0EmhxYM13DOmyiYcUoFr5tOzwxweJ"                                 // the secret key generated from firebase
#define API_KEY "AIzaSyDgtSgrRwTQxLC75wZ0QanIoSjtg40bgwI"
int low_threshold=40;
int high_threshold= 70;
LiquidCrystal_I2C lcd(0x27, 16, 2);

// Khởi tạo Firebase
FirebaseConfig firebaseConfig;
FirebaseAuth firebaseAuth;
FirebaseData firebaseData;

int mode;  //chế độ tự động
String path = "/plants/0/";
bool signupOK = false;

// DHT11 và cảm biến độ ẩm đất
#define DHTPIN 2
#define DHTTYPE DHT11
#define SOIL_MOISTURE_PIN 35  // Chân kết nối cảm biến độ ẩm đất

int relay = 33;           // Chân relay cho ESP32
int water_volume = 700;   
int water_velocity = 20;
float humidity;
float temperature ;
float soilMoisture ;
float lux ;
int duration;
BH1750 lightMeter;
int T=0;   //thời gian tưới
char ssid[] = "MANG DAY KTX H1-511";
char password[] = "123456789a";
int prev_minute=-1,prev_hour=-1;
DHT dht(DHTPIN, DHTTYPE);
WiFiUDP ntpUDP;

NTPClient timeClient(ntpUDP, "asia.pool.ntp.org", 25200, 60000);

char Time[] = "00:00:00 ";
char Date[] = "00-00-2000";
byte last_second, second_, minute_, hour_, day_, month_;
int year_;

void setup() {
  Serial.begin(115200);
  Wire.begin();
  lcd.init();
  delay(100);
  lcd.backlight();
  lcd.clear();
  dht.begin();
  pinMode(relay, OUTPUT);
  lightMeter.begin();

  WiFiManager wm;
    if(!wm.autoConnect("ESP32-WiFi-Manager","123456789"))
    {
        Serial.println("Failed to connect to wifi");
        ESP.restart();
    }
    Serial.println("Connected to wifi ");  

  setupFirebase();
  timeClient.begin();
  Firebase.RTDB.getInt(&firebaseData, path + "updated_log_time");
  prev_hour=firebaseData.to<int>();
  String macAddress = "";
  byte mac[6];
  WiFi.macAddress(mac);
  
  for (int i = 0; i < 6; i++) {
    if (mac[i] < 16) {
      macAddress += "0";
    }
    macAddress += String(mac[i], HEX);
    if (i < 5) {
      macAddress += ":";
    }
  }
  Firebase.RTDB.setString(&firebaseData, path + "device_mac", macAddress);
}

void loop() {

  Firebase.RTDB.getInt(&firebaseData, path + "water_mode");  //mode 1-watering automatic; mode 2-watering handle; mode 3-watering schedule
  mode = firebaseData.to<int>();
  timeClient.update();
  unsigned long unix_epoch = timeClient.getEpochTime();
  second_ = second(unix_epoch);
  if (last_second != second_)
  {

    minute_ = minute(unix_epoch);
    hour_ = hour(unix_epoch);
    day_ = day(unix_epoch);
    month_ = month(unix_epoch);
    year_ = year(unix_epoch);

    Time[7] = second_ % 10 + 48;
    Time[6] = second_ / 10 + 48;
    Time[4] = minute_ % 10 + 48;
    Time[3] = minute_ / 10 + 48;
    Time[1] = hour_ % 10 + 48;
    Time[0] = hour_ / 10 + 48;

    Date[0] = day_ / 10 + 48;
    Date[1] = day_ % 10 + 48;
    Date[3] = month_ / 10 + 48;
    Date[4] = month_ % 10 + 48;
    Date[8] = (year_ / 10) % 10 + 48;
    Date[9] = year_ % 10 % 10 + 48;

    //5 phut cap nhap thong tin 1 lan
    int minute = (Time[8] - '0') * 10 + (Time[9] - '0');
    int second = (Time[11] - '0') * 10 + (Time[12] - '0');
    int hour = (Time[5] - '0') * 10 + (Time[6] - '0');
String hourStr = String(hour);
  if(minute/5 != prev_minute)
      {
        CollectData();prev_minute=minute/5;
      }
  if (mode==1) automatic();
  else if (mode==2) handle();
  delay(5000);

    FirebaseJson dayLog;
    dayLog.add("temperature", temperature);
    dayLog.add("humidity", humidity);
    dayLog.add("light", lux);
    dayLog.add("moisture", soilMoisture);

    String time = String(Time) + String (Date);
    dayLog.add("time",time);

    Serial.println("Constructed JSON:");
    Serial.println(dayLog.raw());

    if (prev_hour != hour) {
      prev_hour = hour;
      Firebase.RTDB.pushJSON(&firebaseData, path + "daylogs/", &dayLog);
    }
  }
  lcdDisplay();
}
void automatic() {
  CollectData();
  T = countT();
  if (T > 0) {
    digitalWrite(relay, HIGH);
    Firebase.RTDB.setInt(&firebaseData, path + "automatic_mode/duration", T);
    delay((T-5)*1000);
  } else {
    digitalWrite(relay, LOW);
    Firebase.RTDB.setInt(&firebaseData, path + "automatic_mode/duration", 0);
  }
}

void handle() {
   CollectData();
 Firebase.RTDB.getInt(&firebaseData, path + "manual_mode/server");
   int check = firebaseData.to<int>();
  if (check == 1) {
    Firebase.RTDB.setInt(&firebaseData, path + "manual_mode/device", 1);
    delay(1000);
    digitalWrite(relay, HIGH);
  }
  else {
    digitalWrite(relay, LOW);
    Firebase.RTDB.setInt(&firebaseData, path + "manual_mode/device", 0);
  }
}
void CollectData() {
  humidity = dht.readHumidity();
  temperature = dht.readTemperature();
  int value = analogRead(SOIL_MOISTURE_PIN);
  soilMoisture = map(value, 0, 4095, 100, 0);
  lux = lightMeter.readLightLevel();
  if (!isnan(humidity) && !isnan(temperature) && !isnan(soilMoisture) && !isnan(lux)) {
    //Cập nhập dữ liệu lên firebase mới sửa float sang int của độ ẩm đất và ánh sáng
    Firebase.RTDB.setFloat(&firebaseData, path + "humidity", humidity);
    Firebase.RTDB.setInt(&firebaseData, path + "light", lux);  
    Firebase.RTDB.setInt(&firebaseData, path + "moisture", soilMoisture);
    Firebase.RTDB.setFloat(&firebaseData, path + "temperature", temperature);
  } 
  else {
    Serial.println("Failed to read from DHT sensor!");
  }
}

//Tính thời gian tưới 
int countT( )
{    
  // Kiểm tra giá trị độ ẩm đất để quyết định có bật hay tắt bơm nước
  Firebase.RTDB.getInt(&firebaseData, path + "low_threshold");
  low_threshold = firebaseData.to<int>();
  if (soilMoisture < low_threshold ) {
  Firebase.RTDB.getInt(&firebaseData, path + "high_threshold");
  high_threshold = firebaseData.to<int>();
  int threshold = (low_threshold + high_threshold) /2;
  Firebase.RTDB.getInt(&firebaseData, path + "water_volume");
  water_volume = firebaseData.to<int>();
  Firebase.RTDB.getInt(&firebaseData, path + "water_velocity");
  water_velocity = firebaseData.to<int>();
  int s = water_volume * (threshold - soilMoisture) / threshold ;          // luong nuoc can tuoi
  duration = s / water_velocity;  
  return duration;         //thoi gian tuoi
  }
    else return 0;
} 
void lcdDisplay() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Temp: ");
  lcd.setCursor(5, 0);
  lcd.print(round(temperature));
  lcd.setCursor(8, 0);
  lcd.print("Hum: ");
  lcd.setCursor(12, 0);
  lcd.print(round(humidity));
  lcd.setCursor(0, 1);
  lcd.print("Soil: ");
  lcd.setCursor(5, 1);
  lcd.print(round(soilMoisture));
  lcd.setCursor(8, 1);
  lcd.print("Lux: ");
  lcd.setCursor(12, 1);
  lcd.print(round(lux));
}
void setupFirebase() {
  firebaseConfig.api_key = API_KEY;
  firebaseConfig.database_url = DATABASE_URL;
  if (Firebase.signUp(&firebaseConfig, &firebaseAuth, "", "")) {
    signupOK = true;
    Serial.println("connect firebase success");
  } else {
    Serial.printf("%s\n", firebaseConfig.signer.signupError.message.c_str());
  }
  Firebase.begin(&firebaseConfig, &firebaseAuth);
  Firebase.reconnectWiFi(true);
}