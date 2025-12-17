#include <LiquidCrystal.h>
#include <Wire.h>
#include <RTClib.h>
#include <Keypad.h>
#include <NewPing.h>
#include <IRremote.hpp> // Biblioteca IRremote (versão 4.x)

// --- CONFIGURAÇÃO DO HARDWARE ---

// LCD
LiquidCrystal lcd(12, 11, 5, 4, 3, 2);

// LED
#define LED_VERDE 8

// RTC
RTC_DS3231 rtc;

// SENSOR IR (Controle Remoto)
#define IR_RECEIVE_PIN 51

// CÓDIGOS DO CONTROLE REMOTO (Padrão NEC Genérico)
#define IR_BTN_1     0x45
#define IR_BTN_2     0x46
#define IR_BTN_3     0x47
#define IR_BTN_4     0x44
#define IR_BTN_5     0x40
#define IR_BTN_6     0x43
#define IR_BTN_7     0x07
#define IR_BTN_8     0x15
#define IR_BTN_9     0x09
#define IR_BTN_0     0x19
#define IR_BTN_ASTERISCO 0x16 
#define IR_BTN_HASHTAG   0x0D 
#define IR_BTN_OK        0x1C 
#define IR_BTN_UP        0x18 
#define IR_BTN_LEFT      0x08 
#define IR_BTN_RIGHT     0x5A 
#define IR_BTN_DOWN      0x52 

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

// --- VARIÁVEIS GLOBAIS ---
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

// --- PROTÓTIPOS ---
char obterTecla();
void verificarSenha();
void acionarAlarme();   // Função de ligar
void desligarAlarme();  // Função de desligar
void exibirSenha();
void mudarDataHora();
int esperarNumero();

void setup() {
  Serial.begin(9600);

  // Inicialização do IR
  IrReceiver.begin(IR_RECEIVE_PIN, ENABLE_LED_FEEDBACK);
  Serial.println(F("Sistema Iniciado. Use o Controle Remoto."));

  if (!rtc.begin()) {
    Serial.println("RTC falhou!");
    while (1);
  }

  lcd.begin(16, 2);
  lcd.clear();
  lcd.print("Iniciando...");
  delay(1500);
  lcd.clear();

  pinMode(LED_VERDE, OUTPUT);
  pinMode(RELE_PIN, OUTPUT);
  digitalWrite(LED_VERDE, LOW);
  digitalWrite(RELE_PIN, LOW);
  
  // rtc.adjust(DateTime(2025, 11, 25, 19, 26, 0)); // Descomente se precisar ajustar hora
}

void loop() {
  char tecla = obterTecla();

  if (tecla) {
    Serial.print("Comando: "); Serial.println(tecla);

    // --- COMANDOS DO CONTROLE REMOTO ---
    if (tecla == 'O') { // Comando "O" de ON (Botão OK)
      acionarAlarme();
    } 
    else if (tecla == 'F') { // Comando "F" de OFF (AGORA NA SETA DIREITA)
      desligarAlarme();
    }
    // --- COMANDOS DO TECLADO / GERAIS ---
    else if (tecla == '#') {
      verificarSenha();
    } else if (tecla == '*') {
      // * do Teclado Físico limpa a senha
      senhaDigitada = "";
      lcd.clear();
      lcd.print("Senha Limpa");
      delay(1000);
      lcd.clear();
      lcd.print("Digite a senha");
    } else if (tecla == 'D') {
      lcd.clear();
      lcd.print("Digite a senha");
      senhaDigitada = "";
    } else if (tecla == 'A') {
      lampadaLigada = !lampadaLigada;
      digitalWrite(LED_VERDE, lampadaLigada ? HIGH : LOW);
      lcd.clear();
      lcd.print(lampadaLigada ? "Luz Ligada" : "Luz Desligada");
    } else if (tecla == 'B') {
      // Exibe info
      lcd.clear();
      if (senhaDigitada == senhaCorreta) {
        lcd.print("Senha: "); lcd.print(senhaDigitada);
      } else {
        DateTime now = rtc.now();
        lcd.print(now.hour()); lcd.print(':'); lcd.print(now.minute());
        lcd.setCursor(0, 1); lcd.print("Lum: "); lcd.print(analogRead(LUX_SENSOR_PIN));
      }
      delay(2000);
    } else if (tecla == 'C') {
      mudarDataHora();
    } else {
      // Se for número, adiciona à senha
      if (isDigit(tecla)) {
        senhaDigitada += tecla;
        exibirSenha();
      }
    }
  }

  // --- LÓGICA DE SENSORES (SÓ FUNCIONA SE ATIVADO) ---
  if (sistemaAtivado) {
    int luxValue = analogRead(LUX_SENSOR_PIN);
    unsigned int distancia = sonar.ping_cm();

    if (distancia > 0 && distancia < 10) {
      digitalWrite(RELE_PIN, HIGH);
    } else {
      digitalWrite(RELE_PIN, LOW);
    }

    if (luxValue < LIMITE_LUX) {
      digitalWrite(LED_VERDE, HIGH);
    } else {
      digitalWrite(LED_VERDE, LOW);
    }
  } else {
    // Garante que tudo desliga se o sistema estiver desativado
    digitalWrite(RELE_PIN, LOW);
  }
  
  delay(100);
}

// --- FUNÇÃO CENTRAL DE ENTRADA (ALTERADA) ---
char obterTecla() {
  // 1. Teclado Físico (Prioridade)
  char key = keypad.getKey();
  if (key) return key;

  // 2. Controle Remoto
  if (IrReceiver.decode()) {
    uint16_t command = IrReceiver.decodedIRData.command;
    char irKey = 0;

    switch (command) {
      // --- MAPEAMENTO DE COMANDOS ---
      case IR_BTN_OK:        irKey = 'O'; break; // OK -> Liga Alarme (On)
      
      // ALTERAÇÃO AQUI:
      case IR_BTN_RIGHT:     irKey = 'F'; break; // Seta Direita -> Desliga Alarme (Off)
      
      case IR_BTN_ASTERISCO: irKey = 'C'; break; // Asterisco -> Mudar Data/Hora (troca com a seta)

      // Mapeamento Numérico e Navegação restante
      case IR_BTN_1: irKey = '1'; break;
      case IR_BTN_2: irKey = '2'; break;
      case IR_BTN_3: irKey = '3'; break;
      case IR_BTN_4: irKey = '4'; break;
      case IR_BTN_5: irKey = '5'; break;
      case IR_BTN_6: irKey = '6'; break;
      case IR_BTN_7: irKey = '7'; break;
      case IR_BTN_8: irKey = '8'; break;
      case IR_BTN_9: irKey = '9'; break;
      case IR_BTN_0: irKey = '0'; break;
      case IR_BTN_HASHTAG:   irKey = '#'; break; 
      case IR_BTN_UP:        irKey = 'A'; break;
      case IR_BTN_LEFT:      irKey = 'B'; break;
      // case IR_BTN_DOWN:   irKey = 'D'; break; // Pode mapear se quiser
      default: break;
    }
    IrReceiver.resume();
    return irKey;
  }
  return 0;
}

// --- FUNÇÕES DE CONTROLE DO ALARME ---

void acionarAlarme() {
  sistemaAtivado = true;
  lcd.clear();
  lcd.print("ALARME ATIVADO");
  lcd.setCursor(0, 1);
  lcd.print("Via Controle");
  Serial.println("ALARME LIGADO via Controle");
  delay(2000);
  lcd.clear();
}

void desligarAlarme() {
  sistemaAtivado = false;
  lcd.clear();
  lcd.print("ALARME DESLIGADO");
  lcd.setCursor(0, 1);
  lcd.print("Via Controle");
  Serial.println("ALARME DESLIGADO via Controle");
  
  // Desliga atuadores imediatamente por segurança
  digitalWrite(RELE_PIN, LOW);
  
  delay(2000);
  lcd.clear();
}

void verificarSenha() {
  if (senhaDigitada == senhaCorreta) {
    sistemaAtivado = !sistemaAtivado; // Inverte o estado
    
    lcd.clear();
    if (sistemaAtivado) {
      lcd.print("Sistema Ligado");
    } else {
      lcd.print("Sistema Desligado");
      digitalWrite(RELE_PIN, LOW);
    }
    delay(2000);
  } else {
    lcd.clear();
    lcd.print("Senha Errada");
    delay(2000);
  }
  senhaDigitada = "";
  lcd.clear();
  lcd.print("Digite a senha");
}

void exibirSenha() {
  lcd.setCursor(0, 1);
  lcd.print("Senha: ");
  for (unsigned int i = 0; i < senhaDigitada.length(); i++) {
    lcd.print('*');
  }
}

void mudarDataHora() {
  lcd.clear(); lcd.print("Novo Dia:"); novoDia = esperarNumero();
  lcd.clear(); lcd.print("Novo Mes:"); novoMes = esperarNumero();
  lcd.clear(); lcd.print("Novo Ano:"); novoAno = esperarNumero();
  lcd.clear(); lcd.print("Nova Hora:"); novaHora = esperarNumero();
  lcd.clear(); lcd.print("Novo Min:"); novoMinuto = esperarNumero();
  
  rtc.adjust(DateTime(novoAno, novoMes, novoDia, novaHora, novoMinuto, 0));
  lcd.clear();
  lcd.print("Hora Ajustada!");
  delay(1500);
}

int esperarNumero() {
  String num = "";
  char tecla;
  while (true) {
    tecla = obterTecla();
    if (tecla) {
      if (tecla == '#' || tecla == 'O') { 
        return num.toInt();
      } else if (isDigit(tecla)) {
        num += tecla;
        lcd.setCursor(0, 1);
        lcd.print(num);
      }
    }
    delay(50);
  }
}