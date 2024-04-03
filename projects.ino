#include <DHT.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <WiFi.h>
#include <Firebase_ESP_Client.h>
#include <BH1750.h>

#define DATABASE_URL "https://smartwater-fe007-default-rtdb.asia-southeast1.firebasedatabase.app"  // the project name address from firebase id
#define DATABASE_SECRET "c5d5bRuTW3b0EmhxYM13DOmyiYcUoFr5tOzwxweJ"          // the secret key generated from firebase
#define API_KEY "AIzaSyDgtSgrRwTQxLC75wZ0QanIoSjtg40bgwI"
LiquidCrystal_I2C lcd(0x27, 16, 2);

// Khởi tạo Firebase
FirebaseConfig firebaseConfig;
FirebaseAuth firebaseAuth;
FirebaseData firebaseData;
String path = "Noti/";
bool signupOK = false;

// DHT11 và cảm biến độ ẩm đất
#define DHTPIN 15 
#define DHTTYPE DHT11
#define SOIL_MOISTURE_PIN A0 // Chân kết nối cảm biến độ ẩm đất

int LED[2] = {14, 12}; // Chân LED cho ESP32
int relay = 13; // Chân relay cho ESP32
unsigned long currentTime;
unsigned long lastTime;
BH1750 lightMeter;

char ssid[] = "wifi";
char password[] = "deni3194";

DHT dht(DHTPIN, DHTTYPE);

void setup() {
  Serial.begin(115200);
  Wire.begin();
  lcd.init();
  //delay(100);
  lcd.backlight();
  lcd.clear();
  dht.begin();
  pinMode(relay, OUTPUT);
  for (int i = 0; i < 2; i++) {
    pinMode(LED[i], OUTPUT);
    digitalWrite(LED[i], LOW);
  }
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
}

void loop() {
  float humidity = dht.readHumidity();
  float temperature = dht.readTemperature();
  float soilMoisture = analogRead(SOIL_MOISTURE_PIN);
  float lux = lightMeter.readLightLevel();

  // Cập nhật dữ liệu đến Firebase (nếu cần)
  // ...
  //  readAndSendSensorData(temperature, humidity, soilMoisture);
  currentTime = millis();
  lcd.display();
  if ((currentTime - lastTime) >= 1000) {
    lcdDisplay(temperature, humidity, soilMoisture);
    lastTime = currentTime;
  }
  delay(1000);
}

/*void readAndSendSensorData(float temperature, float humidity, float soilMoisture) {

  if (!isnan(humidity) && !isnan(temperature) && !isnan(soilMoisture)) {
    Serial.print("Humidity: ");
    Serial.print(humidity);
    Serial.print("%  Temperature: ");
    Serial.println(temperature);
    Serial.print("soilMoisture: ");
    Serial.print(soilMoisture);

    .virtualWrite( , temperature);        cap nhap du lieu len app
    .virtualWrite( , humidity);
    .virtualWrite( , soilMoisture);                 
    if (humidity>75||humidity>65)                       RULE
      digitalWrite(LED[0], HIGH);
    else if (temperature<2 || temperature>20)
      digitalWrite(LED[0], HIGH);
    else digitalWrite (LED[0], LOW);
    // Kiểm tra giá trị độ ẩm đất để quyết định có bật hay tắt bơm nước/van nước
    if (soilMoisture < 500) { // Giả sử ngưỡng dưới màu đất khô là 500
      digitalWrite(RELAY_PIN, HIGH); // Bật relay
     delay(5000); // Bật trong 5 giây (hoặc thời gian tưới cây mong muốn)
      digitalWrite(RELAY_PIN, LOW); // Tắt relay sau khi tưới cây xong
  }
  delay(1000); // Chờ 1 giây trước khi đọc lại giá trị từ cảm biến
  } 
  else {
    Serial.println("Failed to read from DHT sensor!");
  }
} */

void control(int pin, int value) {
  digitalWrite(pin, value);
}

void lcdDisplay(float t, float h, float sm) {
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
}
void setupFirebase() {
  firebaseConfig.api_key = API_KEY;
  firebaseConfig.database_url = DATABASE_URL;
  if (Firebase.signUp(&firebaseConfig, &firebaseAuth, "", "")) {
    signupOK = true;
  } else {
    Serial.printf("%s\n", firebaseConfig.signer.signupError.message.c_str());
  }
  Firebase.begin(&firebaseConfig, &firebaseAuth);
  Firebase.reconnectWiFi(true);
}