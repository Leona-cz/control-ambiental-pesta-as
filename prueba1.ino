#include <WiFi.h>
#include <HTTPClient.h>
#include <DHT.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

//WIFI
  const char* ssid = "RIUAEMFI";
  const char* password = "";

//GOOGLE SHEETS
  String googleScriptURL = "https://script.google.com/macros/s/AKfycbxatjwVLIQT3PFMHAGbPrx5B-Ig9_eVRWC0r0qDDLDMFk3jKllC7mDrtiD9MidaGFFZLw/exec";

// SENSORES
#define DHTPIN 4
#define DHTTYPE DHT22
#define MQ135_PIN 34

// LEDS
#define LED_VERDE 18
#define LED_AMARILLO 19
#define LED_ROJO 23

// RELES
#define RELE_VENTILADOR 26
#define RELE_HUMIDIFICADOR 25

#define RELE_ON HIGH
#define RELE_OFF LOW

// OLED
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
#define OLED_ADDR 0x3C

DHT dht(DHTPIN, DHTTYPE);
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

unsigned long tiempoAnterior = 0;
const unsigned long intervaloEnvio = 60000; // 1 minuto

 void conectarWiFi() {
  WiFi.begin(ssid, password);
  Serial.print("Conectando WiFi");

  while (WiFi.status() != WL_CONNECTED) {
    digitalWrite(LED_AMARILLO, HIGH);
    delay(500);
    Serial.print(".");
  }

  digitalWrite(LED_AMARILLO, LOW);
  Serial.println("\nWiFi conectado");
}

void enviarGoogleSheets(float temperatura, float humedad, int aire, String estado) {
  if (WiFi.status() != WL_CONNECTED) {
    conectarWiFi();
  }

  HTTPClient http;

  String url = googleScriptURL +
               "?temperatura=" + String(temperatura, 1) +
               "&humedad=" + String(humedad, 1) +
               "&aire=" + String(aire) +
               "&estado=" + estado;

  http.begin(url);
  int codigo = http.GET();

  Serial.print("Respuesta Google Sheets: ");
  Serial.println(codigo);

  http.end();
}

void setup() {
  Serial.begin(115200);
  delay(2000);

  dht.begin();
  Wire.begin(21, 22);

  pinMode(LED_VERDE, OUTPUT);
  pinMode(LED_AMARILLO, OUTPUT);
  pinMode(LED_ROJO, OUTPUT);

  pinMode(RELE_VENTILADOR, OUTPUT);
  pinMode(RELE_HUMIDIFICADOR, OUTPUT);

  digitalWrite(RELE_VENTILADOR, RELE_OFF);
  digitalWrite(RELE_HUMIDIFICADOR, RELE_OFF);

  if (!display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDR)) {
    Serial.println("No se encontro OLED");
    while (true);
  }

  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.println("Iniciando...");
  display.display();

  conectarWiFi();
}

void loop() {
  float humedad = dht.readHumidity();
  float temperatura = dht.readTemperature();
  int aire = analogRead(MQ135_PIN);

  String estado = "";

  digitalWrite(LED_VERDE, LOW);
  digitalWrite(LED_AMARILLO, LOW);
  digitalWrite(LED_ROJO, LOW);

  digitalWrite(RELE_VENTILADOR, RELE_OFF);
  digitalWrite(RELE_HUMIDIFICADOR, RELE_OFF);

  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);

  if (isnan(humedad) || isnan(temperatura)) {
    estado = "ERROR";

    digitalWrite(LED_ROJO, HIGH);

    display.setCursor(0, 0);
    display.println("Error DHT22");

    Serial.println("Error al leer DHT22");
  } else {
    bool tempIdeal = temperatura >= 15 && temperatura <= 17;
    bool humIdeal = humedad >= 50 && humedad <= 55;
    bool aireBueno = aire < 900;
    bool aireRegular = aire >= 900 && aire < 1300;
    bool aireMalo = aire >= 1300;

    if (tempIdeal && humIdeal && aireBueno) {
      estado = "BUENO";
      digitalWrite(LED_VERDE, HIGH);
    } 
    else if (temperatura <= 28 && humedad <= 65 && aireRegular) {
      estado = "REGULAR";
      digitalWrite(LED_AMARILLO, HIGH);
    } 
    else {
      estado = "MALO";
      digitalWrite(LED_ROJO, HIGH);
    }

    if (temperatura > 15 || aireMalo) {
      digitalWrite(RELE_VENTILADOR, RELE_ON);
    }

    if (humedad < 70) {
      digitalWrite(RELE_HUMIDIFICADOR, RELE_ON);
    }

    Serial.print("Temp: ");
    Serial.print(temperatura);
    Serial.print(" C | Hum: ");
    Serial.print(humedad);
    Serial.print(" % | MQ135: ");
    Serial.print(aire);
    Serial.print(" | Estado: ");
    Serial.println(estado);

    display.setTextSize(1);
    display.setCursor(0, 0);
    display.println("Control Ambiental");

    display.setCursor(0, 13);
    display.print("Temp: ");
    display.print(temperatura, 1);
    display.println(" C");

    display.setCursor(0, 25);
    display.print("Hum: ");
    display.print(humedad, 1);
    display.println(" %");

    display.setCursor(0, 37);
    display.print("Aire: ");
    display.println(aire);

    display.setCursor(0, 52);
    display.print("Estado: ");
    display.print(estado);
  }

  display.display();

  if (millis() - tiempoAnterior >= intervaloEnvio) {
    tiempoAnterior = millis();
    enviarGoogleSheets(temperatura, humedad, aire, estado);
  }

  delay(2000);
}