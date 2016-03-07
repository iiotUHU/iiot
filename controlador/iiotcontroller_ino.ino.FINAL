
#include <Servo.h> 
#include <LiquidCrystal.h>
#include <LCDKeypad.h>
#include <Wire.h>
#include <FSM.h>
#include "myStates.h"

unsigned char myEvent;
char c;

//Modo manual
int ledManual=53;
int pulsadorManual=52;

int pos=4;
int MINVAL=0;
int MAXVAL=3;
int buttonPressed;
//opciones del controlador
const char opciones[4][20]={{"CALIBRAR"},{"ABRIR"},{"VER PLAZAS"},{"SOLICITAR PLAZA"}};
const char calibrados[3][20]={{"C.VACIO"},{"C.BAJO"},{"C.ALTO"}};

//numero de plazas totales del parking
const int NPLAZAS=2;
int plaza;
//inicializamos a libre
char estadoPlazas[NPLAZAS];
int i;

//numero de coches que hay actualmente dentro o que han reservado la plaza
int coches;
//numero de plazas que hay actualmente ocupadas
int plazasOcupadas;

const int ENTRADA=2;
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
  pinMode(pulsadorManual,INPUT);
  pinMode(ledManual,OUTPUT);
  pinMode(ENTRADA,INPUT);
  //inicializamos la comunicacion a traves de I2C
  Wire.begin(1);
  FSM.begin(FSM_NextState,nStateMachine,FSM_State,nStateFcn,INICIO);
  //cuando se inicia se supone que el parking esta vacio
  coches=0;
  plazasOcupadas=0;  
  for(i=0;i<NPLAZAS;i++){
    estadoPlazas[i]='l';
  }
  
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
  //PLAZA
  plaza=0;
  
  myEvent=0;
   c=0;
   //Serial.println("Comienza a ver eventos");

   if(digitalRead(pulsadorManual)==HIGH){
    Serial.println("activado modo manual");
    myEvent=ACTIVAMANUAL;
   }
   
 int espera=millis();
  Wire.requestFrom(5, 1); 
   if(Wire.available()){
     c=Wire.read();
     Serial.print(c);
   }
   Serial.print("Plazas ocupadas ");
   Serial.println(plazasOcupadas);
   Serial.print("Coches ");
   Serial.println(coches);
   Serial.print("POS");
   Serial.println(pos);
    Serial.print("estadoPlaza");
   Serial.println(estadoPlazas[plaza]);
   delay(5000);
   //while(espera+5000>millis()){};
 
  
  if(pos==0){
    myEvent=PETCALIBRADO;
  }else if(pos==1 && plazasOcupadas<2 ){
    myEvent=PETAPERTURA;
  }else if(pos==2){
    myEvent=PETCONSULTA;
  }else if(c=='o'){
    myEvent=OCUPADA;    
  }else if(c=='l'){
    myEvent=LIBRE;
  }else if(pos==3){
    myEvent=PIDEPLAZA;
  }
  
   FSM.AddEvent(myEvent);

  //delay(500);
  
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
 digitalWrite(ledManual,HIGH);
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
      pos=constrain((pos+1)%4,MINVAL,MAXVAL);
    }
    else if (buttonPressed==KEYPAD_DOWN)
    {
      pos=constrain((pos-1)%4,MINVAL,MAXVAL);
    }
    
  }
  while (buttonPressed!=KEYPAD_SELECT);

  digitalWrite(ledManual,LOW);
  
}

void pedirPlaza(void){
  
   int totalplazas=NPLAZAS-coches-plazasOcupadas;
  if(totalplazas>0){
    coches++;
    Wire.beginTransmission(5); // transmit to device #5
    Wire.write("r");              // sends one byte
    Wire.endTransmission();
    pos=4;
    
    
  }else{
    lcd.clear();
    lcd.print("Lo sentimos");
    lcd.setCursor(0,1);
    lcd.print("no hay plazas");
    long tiempo=millis();
     while(tiempo+5000>millis());
  }
   FSM.AddEvent(0);
}

void ocuparPlaza(void){
  if(estadoPlazas[plaza]=='l')  {
    estadoPlazas[plaza]='o';
     plazasOcupadas++;
  }
   
 
    c='0';
    FSM.AddEvent(0);
}

void liberarPlaza(void){
   if(estadoPlazas[plaza]=='o' || estadoPlazas[plaza]=='r'){
     plazasOcupadas--;
     estadoPlazas[plaza]='l';
   }
 
    c='0';
    FSM.AddEvent(0);
}

void calibrar(void){
  
  lcd.clear();
  lcd.print("  elige tipo");
  lcd.setCursor(0,1);
  lcd.print("  de calibrado ");
  
  lcd.print(" ");
  waitButton();
  waitReleaseButton();
  do
  {
    lcd.clear();

    lcd.print(calibrados[pos]);
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

  if(pos==0){
    Serial.println("calibrando vacio");
    Wire.beginTransmission(5);
    Wire.write( "v");
    Wire.endTransmission();
    
  }else if (pos==1){
   Serial.println("calibrando bajo");
    Wire.beginTransmission(5);
    Wire.write( "b");
    Wire.endTransmission();
    
  }else if(pos==2){
   Serial.println("calibrando alto");
    Wire.beginTransmission(5);
    Wire.write( "a");
    Wire.endTransmission();
    
  }

  lcd.clear();
  lcd.print(" CALIBRANDO");
  lcd.setCursor(0,1);
  lcd.print("espere por favor");
   //Wire.requestFrom(5, 1);    // espera 1 byte del esclavo #5
  long espera=millis();
  c='n';
  Serial.print(espera);
  while(espera+56000>millis()){int i=0;};
  Wire.requestFrom(5, 1);  
  c = Wire.read(); 
  Serial.print("valor de i2C: ");  
  Serial.println(c);     
 while ( c!='k') { //se espera a que llegue la confirmación
  Wire.requestFrom(5, 1);  
    c = Wire.read(); 
     Serial.print("valor de i2C: ");  
    Serial.println(c);       
    if(c!='k'){
      long espera=millis(); 
      Serial.print(espera);
      while(espera+56000>millis()){int i=0;};
    }
  }

  
  lcd.clear();
  lcd.print(" el calibrado");
  lcd.setCursor(0,1);
  lcd.print("  ha terminado ");
  while(espera+5000>millis()){};

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

void logo(void){
  lcd.clear();
  lcd.print("     IIOT");
  lcd.setCursor(0,1);
  lcd.print("   UHU ETSI");
}

