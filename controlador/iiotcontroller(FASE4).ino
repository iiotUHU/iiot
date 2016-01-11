
#include <Servo.h> 
#include <LiquidCrystal.h>
#include <LCDKeypad.h>
#include <Wire.h>
#include <FSM.h>
#include "myStates.h"

unsigned char myEvent;

int pos=0;
int MINVAL=0;
int MAXVAL=2;
int buttonPressed;
//opciones del controlador
char opciones[3][20]={{"CALIBRAR"},{"ABRIR"},{"VER PLAZAS"}};

//numero de plazas totales del parking
const int NPLAZAS=2;
//numero de coches que hay actualmente dentro o que han reservado la plaza
int coches;
//numero de plazas que hay actualmente ocupadas
int plazasOcupadas;

//se  crea un objeto Servo para el control de la barrera
Servo barrera;
int valorBarrera;

LCDKeypad lcd;

byte c_up[8] = {
  B00100,
  B01110,
  B10101,
  B00100,
  B00100,
  B00100,
  B00100,
  B00100,
};

byte c_down[8] = {
  B00100,
  B00100,
  B00100,
  B00100,
  B00100,
  B10101,
  B01110,
  B00100,
};

byte c_select[8] = {
  B00000,
  B01110,
  B11111,
  B11111,
  B11111,
  B11111,
  B01110,
  B00000,
};

void setup()
{
  //inicializamos la comunicacion a traves de I2C
  Wire.begin();
  FSM.begin(FSM_NextState,nStateMachine,FSM_State,nStateFcn,STATE1);
  //cuando se inicia se supone que el parking esta vacio
  coches=0;
  plazasOcupadas=0;  
  
  //Se añade la barrera al pin 3 por ser PWM
   barrera.attach(3);
   
   //inicializacion de serial
   Serial.begin(9600);
  
  int i,k;

  lcd.createChar(1,c_select);
  lcd.createChar(2,c_up);
  lcd.createChar(3,c_down);
  lcd.begin(16, 2);
  lcd.clear();
  lcd.print("     IIOT");
  lcd.setCursor(0,1);
  lcd.print("   UHU ETSI");
  delay(3000);

}

void loop()
{
 
 
  ReadEvents();
  
  FSM.Update();
   
}

void ReadEvents(void){
  myEvent=0;
  
  if(pos==0){
    myEvent=PETICION0;
    
  }else if(pos==1 && plazasOcupadas<2 ){
   
    myEvent=PETICION1;
  

  }else if(pos==2){
 
    myEvent=PETICION2;
     
  }
  
   FSM.AddEvent(myEvent);
  
}


int waitButton()
{
  int buttonPressed; 
  waitReleaseButton;
  lcd.blink();
  while((buttonPressed=lcd.button())==KEYPAD_NONE)
  {
  }
  delay(50);  
  lcd.noBlink();
  return buttonPressed;
}

void waitReleaseButton()
{
  delay(50);
  while(lcd.button()!=KEYPAD_NONE)
  {
  }
  delay(50);
}

//METODOS DE LAS FUNCIONES DEL CONTROLADOR

void inicio(void){
  lcd.clear();
  lcd.print("  pulsa un boton");
  lcd.setCursor(0,1);
  lcd.print("  para comenzar ");
  
  lcd.print(" ");
  waitButton();
  waitReleaseButton();
  do
  {
    lcd.clear();

    lcd.print(opciones[pos]);
    lcd.print("?");
    lcd.setCursor(0,1);
    lcd.write(1);
    lcd.write(' ');
    lcd.write(2);
    lcd.write(' ');
    lcd.write(3);
    lcd.write(' ');
    do
    {
      buttonPressed=waitButton();
    }
    while(!(buttonPressed==KEYPAD_SELECT || buttonPressed==KEYPAD_UP || buttonPressed==KEYPAD_DOWN));
    lcd.setCursor(0,1);
    lcd.write(buttonPressed==KEYPAD_SELECT?1:' ');
    lcd.write(' ');
    lcd.write(buttonPressed==KEYPAD_UP?2:' ');
    lcd.write(' ');
    lcd.write(buttonPressed==KEYPAD_DOWN?3:' ');
    delay(100);
    waitReleaseButton();
    
    if (buttonPressed==KEYPAD_UP)
    {
      pos=constrain((pos+1)%3,MINVAL,MAXVAL);
    }
    else if (buttonPressed==KEYPAD_DOWN)
    {
      pos=constrain((pos-1)%3,MINVAL,MAXVAL);
    }
    
  }
  while (buttonPressed!=KEYPAD_SELECT);
  
}

void calibrar(void){
  for(int i=0;i<1;i++)
  {
    //manda la accion de calibrado
     Wire.beginTransmission(5);
     Wire.write("c");
     Wire.endTransmission();
  }
   pos=4;
   FSM.AddEvent(0);
}

void abrir(void){
  valorBarrera = map(valorBarrera, 0, 1023, 0, 179);     // scale it to use it with the servo (value between 0 and 180) 
  barrera.write(90);                  // sets the servo position according to the scaled value 
  long tiempo=millis();
  while(tiempo+5000>millis()); 
   pos=4;
  FSM.AddEvent(0);
}

void cerrar(void){
   valorBarrera = map(valorBarrera, 0, 1023, 0, 179);     // scale it to use it with the servo (value between 0 and 180) 
   barrera.write(0);                  // sets the servo position according to the scaled value      
   long tiempo=millis();
   while(tiempo+5000>millis()); 
    pos=4;
   FSM.AddEvent(0);
}

void verPlazas(void){
  
  lcd.clear();
  lcd.print("Hay un total de");
  lcd.setCursor(0,1);
  int totalplazas=NPLAZAS-coches-plazasOcupadas;
  lcd.print(totalplazas);
  lcd.print(" plazas libres");
  long tiempo=millis();
   while(tiempo+5000>millis()){}; 
   pos=4; 
   FSM.AddEvent(0);
  
}
