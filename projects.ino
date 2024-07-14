#include <DHT.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <WiFi.h>
#include <Firebase_ESP_Client.h>
#include <BH1750.h>
#include <WiFiUdp.h>
#include <NTPClient.h>
#include <Arduino.h>
#include <WiFiManager.h>
#include <TimeLib.h>

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
int max_duration = 200;   
int water_velocity = 20;
float humidity, temperature, soilMoisture, lux;
int duration; //thời gian tưới
BH1750 lightMeter;
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
  if (WiFi.status() != WL_CONNECTED) {
        Serial.println("WiFi lost connection. Reconnecting...");
        WiFi.begin(); // Thử kết nối lại WiFi
        delay(5000);  // Chờ 5 giây trước khi thử lại
        return;
    }

    // Kiểm tra kết nối Firebase
    if (!Firebase.ready()) {
        Serial.println("Firebase not ready. Reconnecting...");
        setupFirebase(); // Thiết lập lại Firebase
        delay(5000);     // Chờ 5 giây trước khi thử lại
        return;
    }

  Firebase.RTDB.getInt(&firebaseData, path + "water_mode");  //mode 1-watering automatic; mode 2-watering handle; mode 3-watering schedule
  mode = firebaseData.to<int>();
  timeClient.update();
  unsigned long unix_epoch = timeClient.getEpochTime();
  second_ = second(unix_epoch);
  CollectData();
  if (last_second != second_)
  {
    minute_ = minute(unix_epoch);
    hour_ = hour(unix_epoch);
    day_ = day(unix_epoch);
    month_ = month(unix_epoch);
    year_ = year(unix_epoch);

    updateTimeDisplay();

    //5 phut cap nhap thong tin 1 lan
    int minute = (Time[3] - '0') * 10 + (Time[4] - '0');
    int second = (Time[6] - '0') * 10 + (Time[7] - '0');
    int hour = (Time[0] - '0') * 10 + (Time[1] - '0');

  if(minute/5 != prev_minute)
      {
        SendData();prev_minute=minute/5;
      }
  if (mode==1) automatic();
  else if (mode==2) handle();

    FirebaseJson dayLog;
    dayLog.add("temperature", temperature);
    dayLog.add("humidity", humidity);
    dayLog.add("light", lux);
    dayLog.add("moisture", soilMoisture);

    String time = String(Time) + String (Date);
    dayLog.add("time",time);

    Serial.println("Constructed JSON:");
    Serial.println(dayLog.raw());

    if (prev_hour != hour && Date[8]!='7') {
      prev_hour = hour;
      Firebase.RTDB.pushJSON(&firebaseData, path + "daylogs/", &dayLog);
      Firebase.RTDB.setInt(&firebaseData, path + "updated_log_time", hour);
    }
  }
  lcdDisplay();
  delay(5000);
}
void automatic() {
  duration = countT();
  if (duration > 0) {
    digitalWrite(relay, HIGH);
    Firebase.RTDB.setInt(&firebaseData, path + "automatic_mode/duration", duration);
    delay(duration*1000);
    SendData();
  } else {
    digitalWrite(relay, LOW);
    Firebase.RTDB.setInt(&firebaseData, path + "automatic_mode/duration", 0);
  }
}

void handle() {
  Firebase.RTDB.getInt(&firebaseData, path + "manual_mode/server");
  int check = firebaseData.to<int>();
  if (check == 1) {
    Firebase.RTDB.setInt(&firebaseData, path + "manual_mode/device", 1);
    delay(1000);
    digitalWrite(relay, HIGH);
    SendData();
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
}
void SendData()
{
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
  Firebase.RTDB.getInt(&firebaseData, path + "max_duration");
  max_duration = firebaseData.to<int>();
  int T = max_duration * (threshold - soilMoisture) / threshold ;          // luong nuoc can tuoi
  return T;         //thoi gian tuoi
  }
    else return 0;
} 
void lcdDisplay() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("T:");
  lcd.setCursor(2, 0);
  lcd.print(round(temperature));
  lcd.setCursor(6, 0);
  lcd.print("H:");
  lcd.setCursor(8, 0);
  lcd.print(round(humidity));
  lcd.setCursor(0, 1);
  lcd.print("S:");
  lcd.setCursor(2, 1);
  lcd.print(round(soilMoisture));
  lcd.setCursor(5, 1);
  lcd.print("L:");
  lcd.setCursor(7, 1);
  lcd.print(round(lux));
}
void updateTimeDisplay() {
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