#include <Arduino.h>
#include <FreeRTOS.h>
#include <Wire.h>
#include <SPI.h>
#include <SSD1306.h>
#include <Adafruit_MCP4725.h>


//Defines
#define PINO_INC_FREQ 4
#define PINO_DEC_FREQ 12
#define PINO_INVERTER 13
#define PINO_SENO_QUADRADO 14
#define PINO_AUTO 27
//Tasks
void taskOperacao(void *pvParameters);
void taskDisplay(void *pvParameters);
void taskTeclado(void *pvParameters);
void taskTeclas(void *pvParameters);
void taskReadAnalog(void *pvParameters);

float periodo=0;
float freq=0.1;
float amplitude=0;
bool inverter=false;
bool seno_quadrado=false;
int tela = 0;
bool automatico = false;
int saida;

enum {
  manual,
  senoidal,
  quadrada
};
int modo=0;

SSD1306 display(0x3c,21,22);
Adafruit_MCP4725 dac;


const PROGMEM uint16_t DACLookup_FullSine_7Bit[128] =
{
  2048, 2148, 2248, 2348, 2447, 2545, 2642, 2737,
  2831, 2923, 3013, 3100, 3185, 3267, 3346, 3423,
  3495, 3565, 3630, 3692, 3750, 3804, 3853, 3898,
  3939, 3975, 4007, 4034, 4056, 4073, 4085, 4093,
  4095, 4093, 4085, 4073, 4056, 4034, 4007, 3975,
  3939, 3898, 3853, 3804, 3750, 3692, 3630, 3565,
  3495, 3423, 3346, 3267, 3185, 3100, 3013, 2923,
  2831, 2737, 2642, 2545, 2447, 2348, 2248, 2148,
  2048, 1947, 1847, 1747, 1648, 1550, 1453, 1358,
  1264, 1172, 1082,  995,  910,  828,  749,  672,
   600,  530,  465,  403,  345,  291,  242,  197,
   156,  120,   88,   61,   39,   22,   10,    2,
     0,    2,   10,   22,   39,   61,   88,  120,
   156,  197,  242,  291,  345,  403,  465,  530,
   600,  672,  749,  828,  910,  995, 1082, 1172,
  1264, 1358, 1453, 1550, 1648, 1747, 1847, 1947
};

void setup() {
  // put your setup code here, to run once:
  
  
  xTaskCreatePinnedToCore(taskOperacao,"Task Operacao",configMINIMAL_STACK_SIZE+3000,NULL,10,NULL,1);
  xTaskCreatePinnedToCore(taskTeclas,"Task Teclas",configMINIMAL_STACK_SIZE+2000,NULL,5,NULL,1);
  xTaskCreatePinnedToCore(taskDisplay,"Task Display",configMINIMAL_STACK_SIZE+2000,NULL,1,NULL,0);
  xTaskCreatePinnedToCore(taskReadAnalog,"Task Analog",configMINIMAL_STACK_SIZE+2000,NULL,5,NULL,0);
}

void loop() {
  // put your main code here, to run repeatedly:
 
}

void taskOperacao(void *pvParameters){
  int tempoDelay=100;
  saida=2048;
  int index=0;
  pinMode(2,OUTPUT);
  Serial.begin(9600);
 
  while(true){
    if(freq > 0){
      if(automatico){
        modo = 1;
        periodo = 1/freq;
        tempoDelay = (int) (periodo * 1000 / 128);
        if (index >= 128) {
          index=0;
        }
        if(seno_quadrado){
          saida = pgm_read_word(&(DACLookup_FullSine_7Bit[index])) * amplitude/10;
        }else{
          if(index < 64){
            saida = 2048 + (int) amplitude * 2047 / 10; 
          }else{
            saida = 2048 - (int) amplitude * 2048 / 10; 
          }
        }
        index++;
      }else{
        modo = 0;
        tempoDelay = (int) 200;
        if(!inverter){
          saida = 2048 + (int) amplitude * 2047 / 10; 
        }else{
          saida = 2048 - (int) amplitude * 2048 / 10; 
        }
        
      }
      
    }
    
        
    Serial.print(modo);
    Serial.print("-");
    Serial.print(saida);
    Serial.print("-");
    Serial.println(amplitude);
  
    vTaskDelay(pdMS_TO_TICKS(tempoDelay));  

  }
}

void taskTeclas(void *pvParameters){
  bool incPeriodo=false;
  bool decPeriodo=false;
  bool aux=false;
  pinMode(PINO_INVERTER, INPUT_PULLDOWN);
  pinMode(PINO_SENO_QUADRADO, INPUT_PULLDOWN);
  pinMode(PINO_INC_FREQ, INPUT_PULLDOWN);
  pinMode(PINO_DEC_FREQ, INPUT_PULLDOWN);
  pinMode(PINO_AUTO, INPUT_PULLDOWN);
  while(true){
    inverter = digitalRead(PINO_INVERTER);
    incPeriodo = digitalRead(PINO_INC_FREQ);
    decPeriodo = digitalRead(PINO_DEC_FREQ);
    seno_quadrado = digitalRead(PINO_SENO_QUADRADO);
    automatico = digitalRead(PINO_AUTO);
    if(incPeriodo && !aux){
      aux = true;
      freq+=0.1;
    }
    if(decPeriodo && !aux && freq >= 0.2){
      aux = true;
      freq-=0.1;
    }
    if(!incPeriodo && !decPeriodo){
      aux=false;
    }
    vTaskDelay(pdMS_TO_TICKS(200));
  }
}

void taskDisplay(void *pvParameters){
  display.init();
  display.setFont(ArialMT_Plain_24);
  display.clear();
  display.display();
  dac.begin(0x60);
  while(true){
    display.clear();
    if(!inverter) {
      display.drawString(10,0,"+" + String(amplitude) + " V");
    }else{
      display.drawString(10,0,"-" + String(amplitude) + " V");
    }
    display.drawString(10,30,String(freq) + " Hz");
    display.display();
     dac.setVoltage(saida, false);
    vTaskDelay(pdMS_TO_TICKS(4));
  }
}

void taskReadAnalog(void *pvParameters){
  int analog=0;
  while(true){
    analog = analogRead(36);
    amplitude = (float) (analog * 10) / 4095;
    vTaskDelay(pdMS_TO_TICKS(500));
  }
}