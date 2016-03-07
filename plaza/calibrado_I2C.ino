
#include <Wire.h>
#include <EEPROM.h>

#define VACIO 0
#define BAJO 1
#define ALTO 2

//variable para comprobar si ya se ha calibrado
int calibrado;

//variables donde se almacenan la distancia en cm y el tiempo en us
long distancia;
long tiempo;

//definicion de las constantes utilizadas
const int echo=6; //pin de arduino donde se conecta echo
const int trigger=7;// pin de arduino donde se conecta trigger
const int offsetultra=2; // offset calculado de forma manual 
const int ledVerde=13;// pin de arduino donde se conecta el led verde
const int ledRojo=12;// pin de arduino donde se conecta el led rojo
const int NMEDIDAS=10; //numero de medidas para la calibracion
const int NMODAS=200;//numero de medidas posibles en la calibracion
const int maxmediamovil=170;//valor maximo correcto para el ultrasonido (media movil)
const int minmediamovil=90;//valor minimo correcto para el ultrasonido (media movil)
const int tolmediamovil=20;//valor de tolerancia para ultrasonido (media movil)

const int ESCENARIOS_CALIBRAR=3; //numero de escenarios para calibrar
const char escenarios [ESCENARIOS_CALIBRAR][6]={"VACIO","BAJO","ALTO"}; //nombre de los escenarios a calibrar
int mediasfoto[ESCENARIOS_CALIBRAR];
int mediasultra[ESCENARIOS_CALIBRAR];

// estructuras para la media movil para ultrasonido
int mediamovilultra=0;
int nmediamovilultra=0;

int medidas [NMEDIDAS];//arreglo para calcular media y mediana
int media; 
int valoresmoda[NMODAS]; //ya que los valores se encuentran entre 2 y 400, cada vez que se mida se inserta en este arreglo y al final en el procesamiento, se comprueba la moda.
int moda;

//Para el LDR
const int foto=A0;//pin para la medicion del LDR
const int fotooffset=40; //offset calculado experimentalmente para el LDR
const int NUMVALORESFOTO = 8;

int numEscenario = 0;

int valorfoto=0;

//creacion de la tabla look-up insertando los valores experimentales
int tablalu[NUMVALORESFOTO][2]={{306,423},{362,473},{471,491},{578,595},{647,611},{1033,656},{1303,699},{1343,719}};
//int tablalu[NUMVALORESFOTO][2]={{306, 423},{362,473},{471,491},{578,595},{647,611},{1033,656},{1303,699},{1343,719}};

//Para el almacenamiento en EEPROM
//Se intenta dejar espacio entre los elementos a almacenar para guardar más información en un futuro
const int dirCalibrado = 0;
//posición en EEPROM para almacenar valores para 5 escenarios a calibrar en ldr
//en principio se usa int (2 bytes), pero se deja espacio para long (4 bytes)
const int dirLDR = 10;

//posición en EEPROM para almacenar valores para 5 escenarios a calibrar en Ultrasonido
//en principio se usa int (2 bytes), pero se deja espacio para long (4 bytes)
//Se deja espacio por si se aumenta los escenarios a calibrar
const int dirUltra = 50;

char opcionCalibrado = '0';

bool terminado = false;

void setup(){
  
  //inicializamos la comunicacion a traves de I2C
  
  Wire.begin(5);
  Wire.onReceive(receiveEvent);
  Wire.onRequest(requestEvent);
  
  Serial.begin(9600);
  pinMode(trigger, OUTPUT); /*activación del pin 9 como salida: para el pulso ultrasónico*/
  pinMode(echo, INPUT); /*activación del pin 8 como entrada: tiempo del rebote del ultrasonido*/
  calibrado=false; //indica que aun no se ha calibrado el sistema
  pinMode(ledRojo,OUTPUT);
  pinMode(ledVerde,OUTPUT);
  
  Serial.println("Esperando orden de calibrado");
}


void loop(){
  switch(opcionCalibrado){
    case 'v':
      //Serial.println("Escenario Vacio");
      escenario(VACIO);
      Serial.print("Calibrado Escenario ");
      Serial.println(escenarios[VACIO]);
      break;
    case 'b':
      //Serial.println("Escenario Bajo");
      escenario(BAJO);
      Serial.print("Calibrado Escenario ");
      Serial.println(escenarios[BAJO]);
      break;
    case 'a':
      //Serial.println("Escenario Alto");
      escenario(ALTO);
      Serial.print("Calibrado Escenario ");
      Serial.println(escenarios[ALTO]);
      break;
  }
  opcionCalibrado = '0';
  //calibraUltrasonico();
  delay(500);
}

void escenario(int num){
  Serial.print("Escenario: ");
  Serial.println(escenarios[num]);
  digitalWrite(ledRojo,HIGH);
  digitalWrite(ledVerde,LOW);
  Serial.println("calibrando LDR");
  mediasfoto[num]=calibraLDR();
  saveEEPROM(mediasfoto[num], dirLDR+ num*4);
  Serial.println("calibrando ULTRA");
  mediasultra[num]=calibraUltrasonico();  
  saveEEPROM(mediasultra[num], dirUltra+ num*4);
  digitalWrite(ledRojo,LOW);
  digitalWrite(ledVerde,HIGH);
  Serial.println("*********************************************************");
  saveEEPROM('s', dirCalibrado);

  //Para el I2C
  terminado = true;
}

int valorLookUp(int entrada){
  Serial.print("Valor entrada lookup--------------------> ");
  Serial.println(entrada);
  int resultado=0;
  int valorfiltrado=0;
  
  if(entrada<tablalu[0][1] || entrada>tablalu[NUMVALORESFOTO-1][1]){
    //Serial.println("Entro en extremo ");
    entrada=constrain(entrada,tablalu[0][1],tablalu[NUMVALORESFOTO-1][1]);
    if(entrada==tablalu[0][1]){
      resultado=tablalu[0][0];
    }else{
       resultado= tablalu[NUMVALORESFOTO-1][0];
    }
  }else{
    //Serial.println("No entro en extremo ");
    //La calibracion se realiza a traves de una regresion por cuadrados minimos.
    int i=0;
    boolean encontrado=false;
    while( i + 1 <NUMVALORESFOTO && !encontrado){
      if(tablalu[i + 1][1] >= entrada){
        float x=entrada;
        float x0=tablalu[i][1];
        float x1=tablalu[i+1][1];
        float y0=tablalu[i][0];
        float y1=tablalu[i+1][0];
        encontrado=true;
        resultado= y0+(((x-x0)/(x1-x0))*(y1-y0));
      }
      i++;
    }
  }
  Serial.print("Valor salida lookup  ");
  Serial.println(resultado);
  return resultado;
}

int calibraLDR(){
  //inicializacion de media,mediana y moda al comenzar el ciclo de medicion.
  media=0;
  moda=0;

  for(int i=0;i<NMEDIDAS;i++){
    medidas[i]=0;
  }
  
  //comienzo del ciclo de medicion
  for(int i=0;i<NMEDIDAS;i++){
    ////Serial.print("Es la vez: ");
    Serial.println(i+1);
      
    valorfoto=analogRead(foto);
    //Serial.print("comienza lookup con ");
    //Serial.println(valorfoto);
    int valor=valorLookUp(valorfoto);
    //Serial.print("termina look up ");
    //vamos preparando la media
    //Serial.println(valor);
    media+=valor;
  
    //insertamos de forma ordenada
    int j=0;
    int k;

    while( j<i && valor<medidas[j] ){
      j++;
    }
    if(j==i){
      medidas[j]=valor;
    }else{
      k=i;
      while(k>=j){
        medidas[k]=medidas[k-1];
        k--;
      }
      medidas[j]=valor;
    }
    delay(5000);
  }

  /*for(int i=0;i<NMEDIDAS;i++){
    Serial.print(medidas[i]);
     Serial.print("-");
  }  */
  Serial.println();
  Serial.print("MEDIANA LDR: ");
  Serial.print(medidas[NMEDIDAS/2]);
  int resultado = medidas[NMEDIDAS/2];
  Serial.println(" lux");
  Serial.print("MEDIA LDR: ");
  //Serial.print(media);
  //int resultado=media/NMEDIDAS;
  Serial.print(resultado);
  Serial.println(" lux");
  
  return resultado;
}

int calibraUltrasonico(){
  mediamovilultra = 0;
  nmediamovilultra = 0;
   //inicializacion de media,mediana y moda al comenzar el ciclo de medicion.
  media=0;
  //moda=0;
  for(int i=0;i<NMODAS;i++){
    valoresmoda[i]=0;
  }
  
  for(int i=0;i<NMEDIDAS;i++){
    medidas[i]=0;
  }
  
  //comienzo del ciclo de medicion
  for(int i=0;i<NMEDIDAS;i++){
    digitalWrite(trigger,LOW); /* Por cuestión de estabilización del sensor*/
    delayMicroseconds(10);// ya que tarda segun las especificaciones unos 10us en pasar de un estado al otro
    digitalWrite(trigger, HIGH); /* envío del pulso ultrasónico*/
    delayMicroseconds(10); // ya que tarda segun las especificaciones unos 10us en pasar de un estado al otro
    tiempo=pulseIn(echo, HIGH); /* Función para medir la longitud del pulso entrante. Mide el tiempo que transcurrido entre el envío
    del pulso ultrasónico y cuando el sensor recibe el rebote, es decir: desde que el pin 12 empieza a recibir el rebote, HIGH, hasta que
    deja de hacerlo, LOW, la longitud del pulso entrante*/
    distancia= 10*((int(tiempo/58))+offsetultra);//distancia= int(0.017*tiempo); /*fórmula para calcular la distancia obteniendo un valor entero se trata de una aproximacin de us/58*/
    //definimos que se encuentre dentro del rango establecido
    Serial.print("Distancia antes ");
    Serial.println(distancia);
    distancia=constrain(distancia, minmediamovil, maxmediamovil);
    Serial.print("Distancia despues ");
    Serial.println(distancia);
    //vamos preparando la media
    //Serial.println(distancia);
    media+=distancia;
    
    //Se adquieren los datos para una media movil 
    mediamovilultra=mediamovilultra+distancia;
    nmediamovilultra++;

    //preparamos la moda
    valoresmoda[distancia]++;
    //insertamos de forma ordenada
    int j=0;
    int k;

    while( j<i && distancia<medidas[j] ){
      j++;  
    }
    if(j==i){
      medidas[j]=distancia;
    }else{
      k=i;
      while(k>=j){
        medidas[k]=medidas[k-1];
        k--;
      }
      medidas[j]=distancia;
    }
    delay(50);
  }
  moda=0;
  valoresmoda[0]=1;
  for(int i=0;i<NMODAS;i++){
    if(valoresmoda[i]>moda){
      moda=i;
    }
  }
  
  Serial.println();
  Serial.print("MEDIANA: distancia ");
  Serial.print(medidas[NMEDIDAS/2]);
  Serial.println(" cm");
  Serial.print("MEDIA: distancia ");
 // Serial.print(media);
  int resultado=media/NMEDIDAS;
  Serial.print(resultado);
  Serial.println(" cm");
  Serial.print("MEDIA MOVIL: distancia ");
 // Serial.print(media);
  resultado=mediamovilultra/nmediamovilultra;
  Serial.print(resultado);
  Serial.println(" cm");
  Serial.print("MODA: distancia ");
  Serial.print(moda);
  Serial.println(" cm");
  
  return resultado;
}

//Metodo receiveEvent
void receiveEvent(int opt){
  //Serial.println("al menos llega");
  while(Wire.available() > 0){
    opcionCalibrado = Wire.read();//con el read se coge 1 B, otro metodo para leeer X B de golpe
    delay(20);
    terminado = false;
  }
  Serial.print("recibo a traves de I2C: ");
  Serial.println(opcionCalibrado);
}

//Metodo requestEvent
void requestEvent(){
  Serial.println("Solicitud respuesta");
  if (terminado){
    Serial.println("Respondiendo Si");
    Wire.write('k');
    terminado = false;
  }else{
    Serial.println("Respondiendo No");
    Wire.write('n');
  }
}

void saveEEPROM( int data, int start){
  Serial.print("Guardar EEPROM ");
  Serial.println(data);
  byte L;
  byte H;
  
  //Se almacenan los bytes más y menos significativos.
  H = highByte(data);
  Serial.print("Parte alta ");
  Serial.println(H);
  
  L = lowByte(data);
  Serial.print("Parte baja ");
  Serial.println(L);
  
  //Se escriben los bytes en direcciones de memoria contiguas.
  EEPROM.write(start, H);
  EEPROM.write(start + 1, L);
}

int loadEEPROM(int start){
  Serial.print("Leer EEPROM ");
  byte H = EEPROM.read(start);
  Serial.print("Parte alta ");
  Serial.println(H);
  
  byte L = EEPROM.read(start + 1);
  Serial.print("Parte baja ");
  Serial.println(L);
  
  //Variable que almacenará el resultado final.
  //int data = H * 256 + L;
  
  return H * 256 + L;
}
