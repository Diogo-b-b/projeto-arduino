#include <LiquidCrystal.h>
#include <Wire.h>
#include <RTClib.h>
#include <Keypad.h>  
#include <NewPing.h> 

// LCD
LiquidCrystal lcd(12, 11, 5, 4, 3, 2);

// LED
#define LED_VERDE 8

// RTC
RTC_DS3231 rtc;

// LIMITE LUMINOSIDADE
const int LIMITE_LUX = 80;

// SENSOR DE LUMINOSIDADE NO PINO A6
#define LUX_SENSOR_PIN A6

// SENSOR HC-SR04
#define TRIGGER_PIN 39
#define ECHO_PIN 45
#define MAX_DISTANCE 200 
NewPing sonar(TRIGGER_PIN, ECHO_PIN, MAX_DISTANCE);

// RELÉ
#define RELE_PIN 40

// SENHA FIXA (no numpad)
const String senhaCorreta = "4442";  
String senhaDigitada = "";  
bool sistemaAtivado = true;
bool lampadaLigada = false; 

// Variáveis para alterar data/hora
int novoDia, novoMes, novoAno, novaHora, novoMinuto, novoSegundo;

// Configuração do teclado matricial (Numpad)
const byte LINHAS = 4;  
const byte COLUNAS = 4; 
char teclas[LINHAS][COLUNAS] = {
  {'1', '2', '3', 'A'},
  {'4', '5', '6', 'B'},
  {'7', '8', '9', 'C'},
  {'*', '0', '#', 'D'}
};
byte pinosLinhas[LINHAS] = {22, 24, 26, 28};
byte pinosColunas[COLUNAS] = {30, 32, 34, 36};
Keypad keypad = Keypad(makeKeymap(teclas), pinosLinhas, pinosColunas, LINHAS, COLUNAS);

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
  pinMode(RELE_PIN, OUTPUT);

  digitalWrite(LED_VERDE, LOW);
  digitalWrite(RELE_PIN, LOW);

  
  rtc.adjust(DateTime(2025, 11, 25, 19, 26, 0));
}

void loop() {
  // Leitura do teclado
  char tecla = keypad.getKey();
  if (tecla) {
    // Exibe a tecla pressionada no Serial Monitor
    Serial.print("Tecla pressionada: ");
    Serial.println(tecla);

    if (tecla == '#') { 
      verificarSenha();
    } else if (tecla == '*') {  
      senhaDigitada = "";
      lcd.clear();
      lcd.print("Digite a senha");
      Serial.println("Senha limpa.");
    } else if (tecla == 'D') {  
      lcd.clear();  
      lcd.setCursor(0, 0);
      lcd.print("Digite a senha");
      senhaDigitada = ""; 
      Serial.println("Tela limpa, aguardando senha...");
    } else if (tecla == 'A') { 
      lampadaLigada = !lampadaLigada;
      digitalWrite(LED_VERDE, lampadaLigada ? HIGH : LOW);
      lcd.clear();
      lcd.print(lampadaLigada ? "Luz Ligada" : "Luz Desligada");
    } else if (tecla == 'B') { 
      lcd.clear();
      if (senhaDigitada == senhaCorreta) {
        lcd.print("Senha: ");
        lcd.print(senhaDigitada);
      } else {
        int luxValue = analogRead(LUX_SENSOR_PIN);
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

        lcd.setCursor(0, 1);
        lcd.print("Lum: ");
        lcd.print(luxValue);
      }
      delay(2000); 
    } else if (tecla == 'C') { 
      mudarDataHora();
    } else {
      senhaDigitada += tecla;
      exibirSenha();  
      Serial.print("Senha digitada até agora: ");
      Serial.println(senhaDigitada); 
    }
  }

  if (sistemaAtivado) {
    // Leitura do sensor de luminosidade
    int luxValue = analogRead(LUX_SENSOR_PIN);

    // Leitura da distância do HC-SR04 (sensor de movimento)
    unsigned int distancia = sonar.ping_cm();

    // Lógica de controle do relé com base no sensor de distância
    if (distancia > 0 && distancia < 10) {
      digitalWrite(RELE_PIN, HIGH);  
    } else {
      digitalWrite(RELE_PIN, LOW);  
    }

    // Lógica para acionar o LED se estiver escuro
    if (luxValue < LIMITE_LUX) {
      digitalWrite(LED_VERDE, HIGH);
    } else {
      digitalWrite(LED_VERDE, LOW); 
    }
  }

  delay(200);
}

void verificarSenha() {
  if (senhaDigitada == senhaCorreta) {
    // Alterna o estado do sistema
    sistemaAtivado = !sistemaAtivado;

    if (sistemaAtivado) {
      digitalWrite(RELE_PIN, LOW);
      digitalWrite(LED_VERDE, LOW); 
      lcd.clear();
      lcd.print("Sistema Ligado");
      Serial.println("Sistema ATIVADO.");
      delay(2000);
    } else {
      digitalWrite(RELE_PIN, LOW); 
      digitalWrite(LED_VERDE, LOW);  
      lcd.clear();
      lcd.print("Sistema Desligado");
      Serial.println("Sistema DESATIVADO.");
      delay(2000);
    }
  } else {
    lcd.clear();
    lcd.print("Senha Errada");
    delay(2000);  
    Serial.println("Senha incorreta.");
  }
  senhaDigitada = "";  
  lcd.clear();
  lcd.print("Digite a senha");
  delay(2000);
}

// Função para exibir a senha digitada como asteriscos no LCD
void exibirSenha() {
  lcd.setCursor(0, 1);
  lcd.print("Senha: ");
  for (int i = 0; i < senhaDigitada.length(); i++) {
    lcd.print('*'); 
  }
}

// Função para mudar a data e hora
void mudarDataHora() {
  lcd.clear();
  lcd.print("Novo Dia (DD): ");
  novoDia = esperarNumero();

  lcd.clear();
  lcd.print("Novo Mês (MM): ");
  novoMes = esperarNumero();

  lcd.clear();
  lcd.print("Novo Ano (YYYY): ");
  novoAno = esperarNumero();

  lcd.clear();
  lcd.print("Nova Hora (HH): ");
  novaHora = esperarNumero();

  lcd.clear();
  lcd.print("Novo Minuto (MM): ");
  novoMinuto = esperarNumero();

  lcd.clear();
  lcd.print("Novo Segundo (SS): ");
  novoSegundo = esperarNumero();

  // Atualiza a data e hora no RTC
  rtc.adjust(DateTime(novoAno, novoMes, novoDia, novaHora, novoMinuto, novoSegundo));
  lcd.clear();
  lcd.print("Data e Hora Atual");
  delay(2000);
}

// Função para esperar a digitação de um número de 2 dígitos
int esperarNumero() {
  String num = "";
  char tecla;
  while (true) {
    tecla = keypad.getKey();
    if (tecla) {
      if (tecla == '#') {
        return num.toInt(); 
      } else {
        num += tecla;
        lcd.setCursor(0, 1);
        lcd.print(num); 
      }
    }
  }
}
