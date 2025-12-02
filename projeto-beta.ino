#include <LiquidCrystal.h>
#include <Wire.h>
#include <RTClib.h>

// LCD
LiquidCrystal lcd(12, 11, 5, 4, 3, 2);

// LEDS 
#define LED_VERDE 8  

// RTC  
RTC_DS3231 rtc; 

// LIMITE  
const int LIMITE_LUX = 80;

// SENSOR DE LUMINOSIDADE NO PINO 6 
#define LUX_SENSOR_PIN A6

void setup() {  
  Serial.begin(9600);  

  // Inicialização do RTC  
  if (!rtc.begin()) {  
    Serial.println("RTC não encontrado!");  
    while (1);  
  }  

  // Inicializa o LCD  
  lcd.begin(16, 2);  
  lcd.clear();  
  lcd.print("Iniciando...");  
  delay(1500);  
  lcd.clear();  

  // LEDs  
  pinMode(LED_VERDE, OUTPUT);  

  digitalWrite(LED_VERDE, LOW);  

  // Ajuste do RTC para a data e hora (Exemplo: 25/11/2025 19:26:00)  
  rtc.adjust(DateTime(2025, 11, 25, 19, 26, 0));  
}  

void loop() {  
  int luxValue = analogRead(LUX_SENSOR_PIN);

  // PRINT NO SERIAL 
  Serial.print("Lum: ");  
  Serial.println(luxValue);

  lcd.clear();  

  if (luxValue == -1) {
    lcd.setCursor(0, 1);  
    lcd.print("Erro no sensor");  
    return;  
  }  

  // LÓGICA DE ALERTA
  if (luxValue < LIMITE_LUX) {  
    
    digitalWrite(LED_VERDE, HIGH);  

    // Exibe a mensagem de alerta e a luminosidade no LCD  
    lcd.setCursor(0, 1);  
    lcd.print("ACESO! ");
    lcd.print(luxValue);  
    lcd.print("(lux)");  

    // Exibe a data e hora no LCD  
    DateTime now = rtc.now();  
    lcd.setCursor(0, 0);  
    lcd.print(now.day(), DEC);  
    lcd.print('/');  
    lcd.print(now.month(), DEC);  
    lcd.print('/');  
    lcd.print(now.year(), DEC);  
    lcd.print(" ");  
    lcd.print(now.hour(), DEC);  
    lcd.print(':');  
    lcd.print(now.minute(), DEC);  
    lcd.print(':');  
    lcd.print(now.second(), DEC);  
  } else {  

    digitalWrite(LED_VERDE, LOW);    

    
    lcd.setCursor(0, 1);  
    lcd.print("Lum: ");  
    lcd.print(luxValue);  


    // Exibe a data e hora no LCD  
    DateTime now = rtc.now();  
    lcd.setCursor(0, 0);  
    lcd.print(now.day(), DEC);  
    lcd.print('/');  
    lcd.print(now.month(), DEC);  
    lcd.print('/');  
    lcd.print(now.year(), DEC);  
    lcd.print(" ");  
    lcd.print(now.hour(), DEC);  
    lcd.print(':');  
    lcd.print(now.minute(), DEC);  
    lcd.print(':');  
    lcd.print(now.second(), DEC);  
  }  

  delay(2000); 
}  
