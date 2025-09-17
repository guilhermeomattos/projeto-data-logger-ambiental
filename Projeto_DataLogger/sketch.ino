#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <DHT.h>
#include <EEPROM.h>
#include <RTClib.h>

//===================================================================================================================
// Definições e variáveis globais
//===================================================================================================================

// Ajuste de fuso horário para UTC-3
#define UTC_OFFSET -3    

// Configuração da EEPROM
const int maxRecords = 100;
const int recordSize = 10; // Tamanho de cada registro em bytes
int startAddress = 0;
int endAddress = maxRecords * recordSize;
int currentAddress = 0;
int lastLoggedMinute = -1;

// RTC (Relógio de tempo real)
RTC_DS1307 RTC;
DateTime now;
int offsetSeconds;
DateTime adjustedTime;

// DHT (Sensor de temperatura e umidade)
#define DHTPIN 13
#define DHTTYPE DHT11
DHT dht(DHTPIN, DHTTYPE);

// LCD (Display I2C)
#define I2C_ADDR 0x27
LiquidCrystal_I2C lcd(I2C_ADDR, 16, 2);

// LDR (Sensor de luminosidade)
#define pinLDR A1
int valorLDR;
int intensidadeLuz;
float valorMax = 100;
float valorMin = 0;

// Potenciômetro
#define pinPot A0
int valorPot = 0;

// Botão
#define btn 12
int status = LOW;

// Buzzer
#define buzzer 2

// LEDs
#define red 8
#define green 7

// Limites iniciais para sensores
double t_min = 15.0; // Temperatura mínima (Celsius)
double t_max = 25.0; // Temperatura máxima (Celsius)
double i_min = 0.0;  // Iluminação mínima (%)
double i_max = 30.0; // Iluminação máxima (%)
double u_min = 30.0; // Umidade mínima (%)
double u_max = 70.0; // Umidade máxima (%)

// Variáveis auxiliares
bool aux = true;
int triggers[] = {0, 0}; // {minimo, maximo}
int numberTemp = 0;

//===================================================================================================================
// Strings dos menus
//===================================================================================================================
String Inicio[] =
{
  "      Menu     >", //0
  "<     Setup    >", //1
  "<     LOG       "  //2
};

String menu[] = 
{
  "      Data     >", //0
  "<    Horario   >", //1
  "<    Umidade   >", //2
  "< Temperatura  >", //3
  "< Luminosidade >", //4
  "<    Voltar     " //5
};

String configurar[] =
{
  "    Umidade    >", //0
  "< Temperatura  >", //1
  "< Luminosidade >", //2
  "<    Voltar     "  //3
};

String setupTemperatura[] =
{
  "    Limites    >", //0
  "<   Unidades   >", //1
  "<    Voltar     " //2
};

String Unidades[] = 
{
  "    Celsius    >", //0
  "<    Kelvin    >", //1
  "<  Fahrenheit   " //2
};

//===================================================================================================================
// Função de inicialização
//===================================================================================================================
void setup()
{
  dht.begin();
  RTC.begin();
  EEPROM.begin();
  Serial.begin(9600);

  // Ajusta o RTC para a data e hora de compilação
  RTC.adjust(DateTime(F(__DATE__), F(__TIME__)));

  lcd.init();
  lcd.backlight();

  // Configura os pinos
  pinMode(pinLDR, INPUT);
  pinMode(pinPot, INPUT);
  pinMode(btn, INPUT);
  pinMode(buzzer, OUTPUT);
  pinMode(red, OUTPUT);
  pinMode(green, OUTPUT);
  digitalWrite(green, LOW);
  digitalWrite(red, HIGH);

  // Calibração do LDR (máximo e mínimo)
  while(aux == true)
  {    
    lcd.setCursor(0, 0);
    lcd.print("==(Luz maxima)==");
    lcd.setCursor(0, 1);
    lcd.print(" Aperte o botao ");
    while(status == LOW) //max
    {
      delay(500);
      valorMax = analogRead(pinLDR);
      tone(buzzer, 500, 200);
      status = digitalRead(btn);
    }
    
    tone(buzzer, 1000, 200);
    delay(1000);
    status = LOW;
    delay(1000);
    
    lcd.setCursor(0, 0);
    lcd.print("==(Luz minima)==");
    lcd.setCursor(0, 1);
    lcd.print(" Aperte o botao ");
    while(status == LOW) //min
    {
      delay(500);
      valorMin = analogRead(pinLDR);
      tone(buzzer, 200, 200);
      status = digitalRead(btn);
    }
    
    tone(buzzer, 1000, 200);
    delay(1000);
    status = LOW;
    noTone(buzzer);
    
    if(valorMax != valorMin)
    {
      lcd.setCursor(0,0);
      lcd.print("==(Calibracao)==");
      lcd.setCursor(0, 1);
      lcd.print("===(Sucesso)===");
      aux = false;
      delay(5000);
    }
    else
    {
      lcd.setCursor(0,0);
      lcd.print("==(Calibracao)==");
      lcd.setCursor(0, 1);
      lcd.print("====(Falhou)====");
      
      tone(buzzer, 1000, 200);
      delay(500);
      tone(buzzer, 1000, 200);
      delay(5000);
    }
  }
}

//===================================================================================================================
// Loop principal
//===================================================================================================================
void loop()
{
  lcd.setCursor(0, 0);
  // Seleciona a opção do menu inicial conforme o potenciômetro
  valorPot = map(analogRead(pinPot), 0, 1024, 0, sizeof(Inicio)/sizeof(Inicio[0]));
  lcd.print(Inicio[valorPot]);
  
  lcd.setCursor(0, 1);
  lcd.print(" Aperte o botao ");

  status = digitalRead(btn);

  if(status == HIGH)
  {
    tone(buzzer, 1000, 200);
    delay(1000);
    status == LOW;
    switch (valorPot)
    {
      case 0:
        subMenu();      // Acessa submenu de leituras
        break;

      case 1:
        menuSetup();    // Acessa menu de configuração
        break;

      case 2:
        get_log();      // Mostra logs salvos na EEPROM
        break;
    }
  }
}

//===================================================================================================================
// Submenu de leituras e exibição dos sensores
//===================================================================================================================
void subMenu()
{
  lcd.clear();
  aux = true;
  while(aux == true)
  {
    digitalWrite(red, LOW);
    digitalWrite(green, HIGH);
    
    // Atualiza data/hora com fuso horário
    DateTime now = RTC.now();
    int offsetSeconds = UTC_OFFSET * 3600;
    now = now.unixtime() + offsetSeconds;
    DateTime adjustedTime = DateTime(now);

    // Lê sensores
    float temperatura = dht.readTemperature();
    float umidade = dht.readHumidity();
    float valorLDR = analogRead(pinLDR);
    float iluminacao = map(valorLDR, valorMin, valorMax, 0, 100);

    // Salva na EEPROM se houve mudança de minuto e algum valor está fora dos limites
    if(adjustedTime.minute() != lastLoggedMinute)
    {
      lastLoggedMinute = adjustedTime.minute();
      if 
      ( 
        temperatura < t_min || temperatura > t_max || 
        umidade < u_min || umidade > u_max ||
        iluminacao < i_min || iluminacao > i_max
      ) 
      {
        tone(buzzer, 1000);
        digitalWrite(green, LOW);
        delay(1000);
        digitalWrite(green, HIGH);

        // Converter valores para int para armazenamento
        int tempInt = (int)(temperatura * 100);
        int humiInt = (int)(umidade * 100);
        int ilumiInt = (int)(iluminacao);

        // Escrever dados na EEPROM
        EEPROM.put(currentAddress, now.unixtime());
        EEPROM.put(currentAddress + 4, tempInt);
        EEPROM.put(currentAddress + 6, humiInt);
        EEPROM.put(currentAddress + 8, ilumiInt);

        // Atualiza o endereço para o próximo registro
        getNextAddress();
      }
      else
      {
        noTone(buzzer);
      }
    }

    lcd.setCursor(0, 0);
    valorPot = map(analogRead(pinPot), 0, 1024, 0, sizeof(menu)/sizeof(menu[0]));
    lcd.print(menu[valorPot]);

    lcd.setCursor(0, 1);
    lcd.print("                ");
    lcd.setCursor(0, 1);

    // Exibe informações conforme opção do menu
    switch (valorPot)
    {
      case 0: // Data
        lcd.setCursor(3,1);
        lcd.print(adjustedTime.day() < 10 ? "0" : "");
        lcd.print(adjustedTime.day());
        lcd.print("/");
        lcd.print(adjustedTime.month() < 10 ? "0" : "");
        lcd.print(adjustedTime.month());
        lcd.print("/");
        lcd.print(adjustedTime.year());
        break;

      case 1: // Horário
        lcd.setCursor(4,1);
        lcd.print(adjustedTime.hour() < 10 ? "0" : "");
        lcd.print(adjustedTime.hour());
        lcd.print(":");
        lcd.print(adjustedTime.minute() < 10 ? "0" : "");
        lcd.print(adjustedTime.minute());
        lcd.print(":");
        lcd.print(adjustedTime.second() < 10 ? "0" : "");
        lcd.print(adjustedTime.second());
        break;

      case 2: // Umidade
        lcd.setCursor(5, 1);
        lcd.print(dht.readHumidity());
        lcd.print("%");
        break;

      case 3: // Temperatura
        lcd.setCursor(5, 1);
        switch (numberTemp)
        {
          case 0:
            lcd.print(dht.readTemperature());
            lcd.print(" C");
            break;

          case 1:
            lcd.print((double) (dht.readTemperature() + 273.15));
            lcd.print(" K");
            break;

          case 2:
            lcd.print((dht.readTemperature() * 1.8) + 32);
            lcd.print(" F");
            break;
        }
        break;

      case 4: // Luminosidade
        lcd.setCursor(6, 1);
        valorLDR = analogRead(pinLDR);
        intensidadeLuz = map(valorLDR, valorMin, valorMax, 0, 100);
        lcd.print(intensidadeLuz);
        lcd.print("%");
        break;

      case 5: // Voltar
        lcd.print(" Aperte o botao ");
        status = digitalRead(btn);
        if(status == HIGH)
        {
          tone(buzzer, 1000, 200);
          delay(1000);
          status = LOW;
          aux = false;
        }
        break;
    }
    if(aux == false)
    {
      break;
    }
    else
    {
      delay(1000);
    }
  }
  digitalWrite(green, LOW);
  digitalWrite(red, HIGH);
}

//===================================================================================================================
// Menu de configuração dos limites e unidades
//===================================================================================================================
void menuSetup()
{
  lcd.clear();
  aux = true;
  while(aux == true)
  {
    lcd.setCursor(0, 0);
    lcd.print("====(Setup)====");
    lcd.setCursor(0, 1);
    valorPot = map(analogRead(pinPot), 0, 1024, 0, sizeof(configurar)/sizeof(configurar[0]));
    lcd.print(configurar[valorPot]);

    status = digitalRead(btn);

    if(status == HIGH)
    {
      tone(buzzer, 1000, 200);
      delay(1000);
      status = LOW;
      switch (valorPot)
      {
        case 0: // Umidade
          Limites(0);
          u_min = triggers[0];
          u_max = triggers[1];
          break;

        case 1: // Temperatura
          menuTemperatura();
          aux = true;
          break;

        case 2: // Luminosidade
          Limites(0);
          i_min = triggers[0];
          i_max = triggers[1];
          break;

        case 3: // Voltar
          aux = false;
          break;
      }
    }
  }
}

//===================================================================================================================
// Menu de configuração de temperatura (limites e unidade)
//===================================================================================================================
void menuTemperatura()
{
  lcd.clear();
  aux = true;
  while(aux == true)
  { 
    lcd.setCursor(0, 0);
    lcd.print("=(Temperatura)=");

    lcd.setCursor(0, 1);
    valorPot = map(analogRead(pinPot), 0, 1024, 0, sizeof(setupTemperatura)/sizeof(setupTemperatura[0]));
    lcd.print(setupTemperatura[valorPot]);

    status = digitalRead(btn);

    if(status == HIGH)
    {
      tone(buzzer, 1000, 200);
      delay(1000);
      status = LOW;
      switch (valorPot)
      {
        case 0: // Limites
          Limites(1);
          t_min = triggers[0];
          t_max = triggers[1];
          break;

        case 1: // Unidade
          while(status == LOW)
          {
            lcd.setCursor(0, 0);
            lcd.print("===(Unidades)===");

            lcd.setCursor(0, 1);
            valorPot = map(analogRead(pinPot), 0, 1024, 0, sizeof(Unidades)/sizeof(Unidades[0]));
            lcd.print(Unidades[valorPot]);
            status = digitalRead(btn);
            delay(150);
          }
          numberTemp = valorPot;
          tone(buzzer, 1000, 200);
          delay(1000);
          status = LOW;
          break;

        case 2: // Voltar
          aux = false;
          break;
      }
    }
  }
}

//===================================================================================================================
// Função para definir limites mínimo e máximo usando potenciômetro
//===================================================================================================================
void Limites(int type)
{
  lcd.clear();
  status = LOW;
  lcd.setCursor(0, 0);
  lcd.print("====(Minimo)====");
  
  while(status == LOW)
  {
    
    if(type == 1)
    {
      lcd.setCursor(0, 1);
      valorPot = map(analogRead(pinPot), 0, 1024, -20, 80);
      lcd.print("      ");
      lcd.print(valorPot);
      lcd.print(" C");
      lcd.print("    ");
    }
    else
    { 
      lcd.setCursor(6, 1);
      valorPot = map(analogRead(pinPot), 0, 1024, 0, 100);
      lcd.print(valorPot < 10 ? "0" : "");
      lcd.print(valorPot);
      lcd.print(" %");
    }

    delay(150);
    status = digitalRead(btn);
    delay(150);
  }
  tone(buzzer, 1000, 200);
  delay(1000);
  triggers[0] = valorPot;

  status = LOW;
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("====(Maximo)====");
  
  while(status == LOW)
  {
    if(type == 1)
    {
      lcd.setCursor(0, 1);
      valorPot = map(analogRead(pinPot), 0, 1024, -20, 80);
      lcd.print("      ");
      lcd.print(valorPot);
      lcd.print(" C");
      lcd.print("    ");
    }
    else
    { 
      lcd.setCursor(6, 1);
      valorPot = map(analogRead(pinPot), 0, 1024, 0, 100);
      lcd.print(valorPot < 10 ? "0" : "");
      lcd.print(valorPot);
      lcd.print(" %");
    }
    delay(150);
    status = digitalRead(btn);
    delay(150);
  }
  tone(buzzer, 1000, 200);
  delay(1000);
  triggers[1] = valorPot;
}

//===================================================================================================================
// Atualiza o endereço do próximo registro na EEPROM
//===================================================================================================================
void getNextAddress() 
{
    currentAddress += recordSize;
    if (currentAddress >= endAddress) 
    {
      currentAddress = 0; // Volta para o começo se atingir o limite
    }
}

//===================================================================================================================
// Função para exibir os logs salvos na EEPROM via Serial
//===================================================================================================================
void get_log() {
    Serial.println("Data stored in EEPROM:");
    Serial.println("Timestamp\t\tTemperatura\tUmidade\t\tIluminacao");

    for (int address = startAddress; address < endAddress; address += recordSize) {
        long timeStamp;
        int tempInt, humiInt, ilumiInt;

        // Ler dados da EEPROM
        EEPROM.get(address, timeStamp);
        EEPROM.get(address + 4, tempInt);
        EEPROM.get(address + 6, humiInt);
        EEPROM.get(address + 8, ilumiInt);

        // Converter valores
        float temperature = tempInt / 100.0;
        float humidity = humiInt / 100.0;
        float iluminacao = ilumiInt;

        // Verificar se os dados são válidos antes de imprimir
        if (timeStamp != 0xFFFFFFFF) { // 0xFFFFFFFF é o valor padrão de uma EEPROM não inicializada
            DateTime dt = DateTime(timeStamp);
            Serial.print(dt.timestamp(DateTime::TIMESTAMP_FULL));
            Serial.print("\t");
            Serial.print(temperature);
            Serial.print(" C\t\t");
            Serial.print(humidity);
            Serial.print(" %\t\t");
            Serial.print(iluminacao);
            Serial.println(" %");
        }
    }
}
