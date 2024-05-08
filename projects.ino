#include <DHT.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <WiFi.h>
#include <Firebase_ESP_Client.h>
#include <BH1750.h>
#include <WiFiUdp.h>
#include <NTPClient.h>
#include <TimeLib.h>
#define DATABASE_URL "https://smartwater-fe007-default-rtdb.asia-southeast1.firebasedatabase.app"  // the project name address from firebase id
#define DATABASE_SECRET "c5d5bRuTW3b0EmhxYM13DOmyiYcUoFr5tOzwxweJ"                                 // the secret key generated from firebase
#define API_KEY "AIzaSyDgtSgrRwTQxLC75wZ0QanIoSjtg40bgwI"
int low_threshold=40;
int high_threshold= 70;
//LiquidCrystal_I2C lcd(0x27, 16, 2);

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

//int LED[2] = { 14, 12 };  // Chân LED cho ESP32
int relay = 33;           // Chân relay cho ESP32
int water_mililit = 700;
float humidity;
float temperature ;
float soilMoisture ;
float lux ;
BH1750 lightMeter;
int T=0;   //thời gian tưới
char ssid[] = "MANG DAY KTX H1-511";
char password[] = "123456789a";

DHT dht(DHTPIN, DHTTYPE);
WiFiUDP ntpUDP;

NTPClient timeClient(ntpUDP, "asia.pool.ntp.org", 25200, 60000);

char Time[] = "TIME:00:00:00";
char Date[] = "DATE:00/00/2000";
byte last_second, second_, minute_, hour_, day_, month_;
int year_;

void setup() {
  Serial.begin(115200);
  Wire.begin();
 /* lcd.init();
  //delay(100);
  lcd.backlight();
  lcd.clear();*/
  dht.begin();
  pinMode(relay, OUTPUT);
  lightMeter.begin();

  // Kết nối với Wi-Fi
  WiFi.begin(ssid, password);

  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(300);
  }

  Serial.println("\nWiFi connected");
  Serial.println("IP Address: " + WiFi.localIP().toString());

  setupFirebase();
  timeClient.begin();
}

void loop() {

   Firebase.RTDB.getInt(&firebaseData, path + "water_mode");  //mode 1-watering automatic; mode 2-watering handle; mode 3-watering schedule
  mode = firebaseData.to<int>();
  if (mode==1) automatic();
  else if (mode==2) handle();
 // Serial.println(mode);
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

    Time[12] = second_ % 10 + 48;
    Time[11] = second_ / 10 + 48;
    Time[9] = minute_ % 10 + 48;
    Time[8] = minute_ / 10 + 48;
    Time[6] = hour_ % 10 + 48;
    Time[5] = hour_ / 10 + 48;

    Date[5] = day_ / 10 + 48;
    Date[6] = day_ % 10 + 48;
    Date[8] = month_ / 10 + 48;
    Date[9] = month_ % 10 + 48;
    Date[13] = (year_ / 10) % 10 + 48;
    Date[14] = year_ % 10 % 10 + 48;

    Serial.println(Time);
    Serial.println(Date);
  }
  delay(1000);
 // lcd.display();
}
void automatic() {
  CollectData();
  T = countT();
  if (T > 0) {
    digitalWrite(relay, HIGH);
    Firebase.RTDB.setBool(&firebaseData, path + "is_watered", true);
  } else {
    digitalWrite(relay, LOW);
    Firebase.RTDB.setBool(&firebaseData, path + "is_watered", false);
  }
  delay(2000);
}

void handle() {
   CollectData();
  T = countT();
 Firebase.RTDB.getBool(&firebaseData, path + "is_button");
   bool check = firebaseData.to<bool>();
  if (check == true && T > 0) {
    digitalWrite(relay, HIGH);
    Firebase.RTDB.setBool(&firebaseData, path + "is_watered", true);
  }
  else {
    digitalWrite(relay, LOW);
    Firebase.RTDB.setBool(&firebaseData, path + "is_watered", false);
  }
  delay(2000);
}
void CollectData() {
  humidity = dht.readHumidity();
  temperature = dht.readTemperature();
int value = analogRead(SOIL_MOISTURE_PIN);
soilMoisture = map(value, 0, 4095, 100, 0);
  lux = lightMeter.readLightLevel();
//  if (!isnan(humidity) && !isnan(temperature) && !isnan(soilMoisture) && !isnan(lux)) {
    //Cập nhập dữ liệu lên firebase
    Firebase.RTDB.setFloat(&firebaseData, path + "humidity", humidity);
    Firebase.RTDB.setFloat(&firebaseData, path + "light", lux);  
    Firebase.RTDB.setFloat(&firebaseData, path + "moisture", soilMoisture);
    Firebase.RTDB.setFloat(&firebaseData, path + "temperature", temperature);
//  } 
//  else {
//    Serial.println("Failed to read from DHT sensor!");
//  }
}

//Tính thời gian tưới 
int countT( )
{    
  // Kiểm tra giá trị độ ẩm đất để quyết định có bật hay tắt bơm nước
  Firebase.RTDB.getInt(&firebaseData, path + "low_threshold");
  low_threshold = firebaseData.to<int>();
 // Firebase.RTDB.getInt(&firebaseData, path + "high_threshold");
 // high_threshold = firebaseData.to<int>();
    if (soilMoisture < low_threshold) return 5;
    else return 0;
} 
/*void lcdDisplay() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Temp: ");
  lcd.setCursor(5, 0);
  lcd.print(round(temperature));
  lcd.setCursor(8, 0);
  lcd.print("Hum: ");
  lcd.setCursor(12, 0);
  lcd.print(round(h));
  lcd.setCursor(0, 1);
  lcd.print("Soil: ");
  lcd.setCursor(5, 1);
  lcd.print(round(soilMoisture));
  lcd.setCursor(8, 1);
  lcd.print("Lux: ");
  lcd.setCursor(12, 1);
  lcd.print(round(lux));
}*/
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