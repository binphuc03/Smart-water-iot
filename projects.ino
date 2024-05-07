#include <DHT.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <WiFi.h>
#include <Firebase_ESP_Client.h>
#include <BH1750.h>
#include <Ticker.h>


#define DATABASE_URL "https://smartwater-fe007-default-rtdb.asia-southeast1.firebasedatabase.app"  // the project name address from firebase id
#define DATABASE_SECRET "c5d5bRuTW3b0EmhxYM13DOmyiYcUoFr5tOzwxweJ"                                 // the secret key generated from firebase
#define API_KEY "AIzaSyDgtSgrRwTQxLC75wZ0QanIoSjtg40bgwI"
int low_threshold=400;
int high_threshold= 700;
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
Ticker timer;
char ssid[] = "k1enttt";
char password[] = "tathuctrungkien";

DHT dht(DHTPIN, DHTTYPE);

void setup() {
  Serial.begin(115200);
  Wire.begin();
 /* lcd.init();
  //delay(100);
  lcd.backlight();
  lcd.clear();*/
  dht.begin();
  pinMode(relay, OUTPUT);
 /* for (int i = 0; i < 2; i++) {
    pinMode(LED[i], OUTPUT);
    digitalWrite(LED[i], LOW);
  }*/
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

  timer.attach(300000, CollectData);
}

void loop() {

   Firebase.RTDB.getInt(&firebaseData, path + "water_mode");  //mode 1-watering automatic; mode 2-watering handle; mode 3-watering schedule
  mode = firebaseData.to<int>();
  if (mode==1) automatic();
  else if (mode==2) handle();
  Serial.println(mode);
  /*currentTime = millis();
  lcd.display();
  if ((currentTime - lastTime) >= 1000) {
    lcdDisplay(temperature, humidity, soilMoisture, lux);
    lastTime = currentTime;
  }*/
  //delay(5000);
}
void automatic() 
{ 
  T = countT();CollectData();
  Serial.println(soilMoisture);
  if (T > 0)
  {
    digitalWrite(relay, HIGH);
    Firebase.RTDB.setBool(&firebaseData, path + "is_watered", true);
  }
  else
  {
    digitalWrite(relay, LOW);
    Firebase.RTDB.setBool(&firebaseData, path + "is_watered", false);
  }
  delay(2000);
}
void handle()
{
  T = countT();
  bool is_button = Firebase.RTDB.getBool(&firebaseData, path + "is_button");
  if (is_button == true && T > 0) 
  {
    digitalWrite(relay, HIGH);
    Firebase.RTDB.setBool(&firebaseData, path + "is_watered", true);
    delay(T*1000);
    digitalWrite(relay, LOW);
    Firebase.RTDB.setBool(&firebaseData, path + "is_watered", false);
  }
}
void CollectData() {
  humidity = dht.readHumidity();
  temperature = dht.readTemperature();
int value = analogRead(SOIL_MOISTURE_PIN);
soilMoisture = map(value, 0, 4095, 100, 0);
  lux = lightMeter.readLightLevel();
  if (!isnan(humidity) && !isnan(temperature) && !isnan(soilMoisture) && !isnan(lux)) {
    //Cập nhập dữ liệu lên firebase
    Firebase.RTDB.setFloat(&firebaseData, path + "humidity", humidity);
    Firebase.RTDB.setFloat(&firebaseData, path + "light", lux);  
    Firebase.RTDB.setFloat(&firebaseData, path + "moisture", soilMoisture);
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
  Firebase.RTDB.getInt(&firebaseData, path + "high_threshold");
  high_threshold = firebaseData.to<int>();
    if (soilMoisture < low_threshold) {  // Giả sử ngưỡng dưới màu đất khô là 400
      //digitalWrite(relay, HIGH);  // Bật relay
     // delay(5000);                // Bật trong 5 giây (hoặc thời gian tưới cây mong muốn)
      //digitalWrite(relay, LOW);   // Tắt relay sau khi tưới cây xong
      T=5;
    }
    else return 0;
} 
/*void lcdDisplay(float t, float h, float sm, float l) {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Nhiet do: ");
  lcd.setCursor(10, 0);
  lcd.print(round(t));
  lcd.setCursor(0, 1);
  lcd.print("Do am: ");
  lcd.setCursor(10, 1);
  lcd.print(round(h));
  lcd.setCursor(0, 2);
  lcd.print("Do am dat: ");
  lcd.setCursor(12, 2);
  lcd.print(round(sm));
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