#include <Adafruit_SSD1306.h>
#include <DHT.h>
#include <Wire.h>

// OLED ekran tanımı
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

// DHT22 tanımı
#define DHTPIN 2
#define DHTTYPE DHT22
DHT dht(DHTPIN, DHTTYPE);

// Sensör pinleri
#define MQ135_PIN A0
#define WATER_SENSOR_PIN A1
#define ANEMOMETER_PIN A2 
#define BUTTON_PIN 3 

// MQ135 kalibrasyon değerleri
float R0 = 10.0;

// Buton değişkenleri
bool buttonPressed = false;
unsigned long lastDebounceTime = 0;
unsigned long debounceDelay = 20;    
int sensorIndex = 0;

float calculateResistance(int rawADC) {
  float Vout = (rawADC / 1023.0) * 5.0;
  return ((5.0 - Vout) / Vout) * R0;
}

float calculatePPM(float resistance, float a, float b) {
  return pow(10, (log10(resistance / R0) - b) / a);
}

float calculateCO2(float resistance) {
  float co2 = calculatePPM(resistance, 0.77, -2.05);
  return constrain(co2, 0, 1000);  
}

float calculateNH3(float resistance) {
  float nh3 = calculatePPM(resistance, 0.65, -2.85);
  return constrain(nh3, 0, 50); 
}

float calculateCO(float resistance) {
  float co = calculatePPM(resistance, 0.45, -3.10);
  return constrain(co, 0, 50); 
}

float calculateWindSpeed(int analogValue) {
  float voltage = (analogValue / 1023.0) * 5.0;
  return (voltage / 5.0) * 30.0;
}

void setup() {
  Serial.begin(9600);

  // OLED ekran başlatma
  if (!display.begin(SSD1306_PAGEADDR, 0x3C)) {
    Serial.println(F("OLED baslatilamadi."));
    for (;;);
  }
  display.clearDisplay();
  
  display.setTextSize(2);
  display.setTextColor(SSD1306_WHITE);

  dht.begin();
  pinMode(BUTTON_PIN, INPUT_PULLUP);
}

void loop() {
  // Buton durumu kontrolü
  int reading = digitalRead(BUTTON_PIN);
  if (reading == LOW && !buttonPressed && (millis() - lastDebounceTime > debounceDelay)) {
    lastDebounceTime = millis();
    buttonPressed = true;
    sensorIndex++;
    if (sensorIndex > 7) { 
      sensorIndex = 0;
    }
  } else if (reading == HIGH) {
    buttonPressed = false;
  }

  float temperature = dht.readTemperature();
  float humidity = dht.readHumidity();

  int mq135Raw = analogRead(MQ135_PIN);
  float resistance = calculateResistance(mq135Raw);
  float CO2 = calculateCO2(resistance);  
  float NH3 = calculateNH3(resistance); 
  float CO = calculateCO(resistance);
  
  float airQualityIndex = (CO2 * 0.4 + CO * 0.3 + NH3 * 0.2);
  String airQualityLabel = (airQualityIndex < 50) ? "Good" : "Bad";

  int waterSensorValue = analogRead(WATER_SENSOR_PIN);
  int windRaw = analogRead(ANEMOMETER_PIN);
  float windSpeed = calculateWindSpeed(windRaw);

  display.clearDisplay();

  switch (sensorIndex) {
    case 0:
      display.setTextSize(1);
      display.setCursor(0, 0);
      display.print("Temp:");
      display.setTextSize(2);
      display.setCursor(0, 16);
      display.print(temperature);
      display.println(" C");
      break;
    case 1:
      display.setTextSize(1);
      display.setCursor(0, 0);
      display.print("Humidity:");
      display.setTextSize(2);
      display.setCursor(0, 16);
      display.print(humidity);
      display.println(" %");
      break;
    case 2:
      display.setTextSize(1);
      display.setCursor(0, 0);
      display.print("CO2:");
      display.setTextSize(2);
      display.setCursor(0, 16);
      display.print(CO2);
      display.println(" ppm");
      break;
    case 3:
      display.setTextSize(1);
      display.setCursor(0, 0);
      display.print("NH3:");
      display.setTextSize(2);
      display.setCursor(0, 16);
      display.print(NH3);
      display.println(" ppm");
      break;
    case 4:
      display.setTextSize(1);
      display.setCursor(0, 0);
      display.print("CO:");
      display.setTextSize(2);
      display.setCursor(0, 16);
      display.print(CO);
      display.println(" ppm");
      break;
    case 5:
      display.setTextSize(1);
      display.setCursor(0, 0);
      display.print("AQI:");
      display.setTextSize(2);
      display.setCursor(0, 16);
      display.print(airQualityIndex);
      display.print(" (");
      display.print(airQualityLabel);
      display.println(")");
      break;
    case 6:
      display.setTextSize(1);
      display.setCursor(0, 0);
      display.print("Water:");
      display.setTextSize(2);
      display.setCursor(0, 16);
      display.print(waterSensorValue);
      display.println(" (Raw)");
      break;
    case 7:
      display.setTextSize(1);
      display.setCursor(0, 0);
      display.print("Wind Speed:");
      display.setTextSize(2);
      display.setCursor(0, 16);
      display.print(windSpeed);
      display.println(" m/s");
      break;
    default:
      break;
  }

  display.display();
  delay(1000);
}