#include <Fuzzy.h>
#include <FuzzyComposition.h>
#include <FuzzyInput.h>
#include <FuzzyIO.h>
#include <FuzzyOutput.h>
#include <FuzzyRule.h>
#include <FuzzyRuleAntecedent.h>
#include <FuzzyRuleConsequent.h>
#include <FuzzySet.h>

#include <Wire.h>
#include <EEPROM.h>

// Paso 1 - Instanciando un objeto de la librería fuzzy
Fuzzy* fuzzy = new Fuzzy();

/*
FuzzySet* noExisteLuz = new FuzzySet(0, 0, 275, 425);
FuzzySet* existeLuz = new FuzzySet(275, 425, 800, 800);

//Etiquetas distancia (Multiplicado por 10)
FuzzySet* cocheAlto = new FuzzySet(0, 0, 127.5, 142.5);
FuzzySet* cocheBajo = new FuzzySet(127.5, 142.5, 155, 165);
FuzzySet* noExisteCoche = new FuzzySet(165, 170, 210, 210);
*/
//Etiquetas salida
//FuzzySet* noHayCoche = new FuzzySet(0, 0, 0, 0);
//FuzzySet* hayCoche = new FuzzySet(1, 1, 1, 1);
FuzzySet* noHayCoche = new FuzzySet(0, 0, 40, 60);
FuzzySet* hayCoche = new FuzzySet(40, 60, 100, 100);

FuzzySet* noExisteLuz;
FuzzySet* existeLuz;

//Etiquetas distancia (Multiplicado por 10)
FuzzySet* cocheAlto;
FuzzySet* cocheBajo;
FuzzySet* noExisteCoche;

//Conjunto de entrada
FuzzyInput* luz = new FuzzyInput(1);// Como parámetro necesita un ID
FuzzyInput* distancia = new FuzzyInput(2);// Como parámetro necesita un ID

//Conjunto de salida
FuzzyOutput* coche = new FuzzyOutput(1);// ID de parámetro

//variable para comprobar si ya se ha calibrado
char calibrado;

//definicion de las constantes utilizadas
const int echo=6; //pin de arduino donde se conecta echo
const int trigger=7;// pin de arduino donde se conecta trigger
const int offsetUltra=2; // offset calculado de forma manual 
const int ledVerde=8;// pin de arduino donde se conecta el led verde
const int ledAmarillo=10; //pin para representar la plaza ocupada usando un led amarillo a falta de naranja
const int ledRojo=12;// pin de arduino donde se conecta el led rojo

const int N_MEDIDAS=4; //numero de medidas para la calibracion
const int N_MODAS=200;//numero de medidas posibles en la calibracion
const int maxMediaMovil=190;//valor maximo correcto para el ultrasonido (media movil) //modificado
const int minMediaMovil=70; //valor minimo correcto para el ultrasonido (media movil) //modificado
const int tolMediaMovil=20;//valor de tolerancia para ultrasonido (media movil) //modificado

const int ESCENARIOS_CALIBRAR=3; //numero de escenarios para calibrar
//const char escenarios [ESCENARIOS_CALIBRAR][6]={"VACIO","BAJO","ALTO"}; //nombre de los escenarios a calibrar
int mediasFoto[ESCENARIOS_CALIBRAR];
int mediasUltra[ESCENARIOS_CALIBRAR];

// estructuras para la media movil para ultrasonido
int mediaMovilUltra=0;
int nMediaMovilUltra=0;

int medidas[N_MEDIDAS];//arreglo para calcular media y mediana
int media; 
//int valoresModa[N_MODAS]; //ya que los valores se encuentran entre 2 y 200, cada vez que se mida se inserta en este arreglo y al final en el procesamiento, se comprueba la moda.
//int moda;

//Para el LDR
const int foto=A0;//pin para la medicion del LDR
const int fotoOffset=40; //offset calculado experimentalmente para el LDR

int valorfoto=0;

const int NUM_VALORES_FOTO=8;
//creacion de la tabla look-up insertando los valores experimentales
int tablaLumen[NUM_VALORES_FOTO][2]={{306,423},{362,473},{471,491},{578,595},{647,611},{1033,656},{1303,699},{1343,719}};

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

//indica el estado de la plaza
char plaza = 'l';

void setup(){  
  //inicializamos la comunicacion a traves de I2C
  Wire.begin(5);
  Wire.onReceive(receiveEvent);
  Wire.onRequest(requestEvent);
  
  Serial.begin(9600);
  pinMode(trigger, OUTPUT); /*activación del pin 9 como salida: para el pulso ultrasónico*/
  pinMode(echo, INPUT); /*activación del pin 8 como entrada: tiempo del rebote del ultrasonido*/
  //calibrado=false; //indica que aun no se ha calibrado el sistema //modificado
  
  //Se recupera si ya se calibró el sistema o no junto con la información almacenada en caso positivo
  calibrado = loadEEPROM(dirCalibrado);
  
  if(calibrado == 's'){
    for(int i=0;i<ESCENARIOS_CALIBRAR;i++){
       mediasFoto[i] = loadEEPROM(dirLDR + i*4); //modificado
       mediasUltra[i] = loadEEPROM(dirUltra + i*4); //modificado
    }
    
    //Iniciar fuzzy
    initFuzzy();
  }else{
    Serial.println("Hay que calibrar la plaza");
  }

  pinMode(ledRojo,OUTPUT);
  pinMode(ledVerde,OUTPUT);
}

void loop(){
  /*for(int i=0;i<8;i++){
  for(int j=0;j<2;j++){
    Serial.print(tablaLumen[i][j]);
    Serial.print("---");
  }
  Serial.println();
}*/  
  int mediafoto;
  int mediaultra;
  char opcion;
  
  /*while(opcion!=1 && !calibrado){
    //MENSAJE DE QUE NECESITA CALIBRARSE PRIMERO
  }*/
  //Serial.println("INICIO");
  //opcion=Serial.read();
  espera(millis() + 3000);
  
  int ultrasonico = medidaUltrasonico();
  int luz = medidaLDR();
  
  Serial.print("Valor ultrasonico ");
  Serial.println(ultrasonico);
  Serial.print("Valor luz ");
  Serial.println(luz);
  
  int coche = getFuzzyValue(luz, ultrasonico);
  Serial.print("Despues getFuzzyValue ");
  Serial.println(coche);
  //int coche;
  
  if(coche >= 50){
    plaza = 'o';
    digitalWrite(ledVerde, LOW);
    digitalWrite(ledRojo, HIGH);
    digitalWrite(ledAmarillo, LOW);
  }else{
    if(plaza != 'r'){
      plaza = 'l';
      digitalWrite(ledRojo, LOW);
      digitalWrite(ledVerde, HIGH);
    }
  }
  Serial.print("Estado plaza ");
  Serial.println(plaza);
  //espera(millis()+3000);
  //Serial.println(calibrado);
  //verMedidasAlmacenadas();
}

void verMedidasAlmacenadas(){
  for(int i=0;i<ESCENARIOS_CALIBRAR;i++){
     Serial.print("medidas LDR: ");
     Serial.println(mediasFoto[i]);
     Serial.print("medidas ULTRA: ");
     Serial.println(mediasUltra[i]);
  }
}

int valorLookUp(int entrada){
  int resultado=0;
  int valorfiltrado=0;
  
  if(entrada<tablaLumen[0][1] || entrada>tablaLumen[NUM_VALORES_FOTO-1][1]){
    entrada=constrain(entrada,tablaLumen[0][1],tablaLumen[NUM_VALORES_FOTO-1][1]);
    if(entrada==tablaLumen[0][1]){
      resultado=tablaLumen[0][0];
    }else{
       resultado= tablaLumen[NUM_VALORES_FOTO-1][0];
    }
  }else{
    //La calibracion se realiza a traves de una regresion por cuadrados minimos.
    int i=0;
    boolean encontrado=false;
    while( i + 1 <NUM_VALORES_FOTO && !encontrado){
      if(tablaLumen[i + 1][1] >= entrada){
        float x=entrada;
        float x0=tablaLumen[i][1];
        float x1=tablaLumen[i+1][1];
        float y0=tablaLumen[i][0];
        float y1=tablaLumen[i+1][0];
        encontrado=true;
        resultado= y0+(((x-x0)/(x1-x0))*(y1-y0));
      }
      i++;
    }
  }
    
  return resultado;
}

int medidaLDR(){
  Serial.println("LDR");
  //inicializacion de media,mediana y moda al comenzar el ciclo de medicion.
  media=0;
  //moda=0;

  for(int i=0;i<N_MEDIDAS;i++){
    medidas[i]=0;
  }
  
  //comienzo del ciclo de medicion
  for(int i=0;i < N_MEDIDAS;i++){
    ////Serial.print("Es la vez: ");
    Serial.println(i+1);
      
    valorfoto=analogRead(foto);
    Serial.print("comienza lookup con ");
    Serial.println(valorfoto);
    int valor=valorLookUp(valorfoto);
    Serial.print("termina look up ");
    //vamos preparando la media
    Serial.println(valor);
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
    //delay(4000);
    long espera = millis();
    while(espera + 4000 > millis()){};
  }

  /*for(int i=0;i<NMEDIDAS;i++){
    Serial.print(medidas[i]);
     Serial.print("-");
  }  */
  Serial.println();
  Serial.print("MEDIANA LDR: ");
  Serial.print(medidas[N_MEDIDAS/2]);
  int resultado = medidas[N_MEDIDAS/2];
  Serial.println(" lux");
  Serial.print("MEDIA LDR: ");
  //Serial.print(media);
  //int resultado=media/N_MEDIDAS;
  Serial.print(resultado);
  Serial.println(" lux");
  
  return resultado;
}

int medidaUltrasonico(){
  long distancia = 0;
  long tiempo = 0;
  mediaMovilUltra = 0;
  nMediaMovilUltra = 0;
   //inicializacion de media,mediana y moda al comenzar el ciclo de medicion.
  media=0;
  //moda=0;
  //for(int i=0;i<N_MODAS;i++){ valoresModa[i]=0; }
  
  for(int i=0;i<N_MEDIDAS;i++){
    medidas[i]=0;
  }
  
  //comienzo del ciclo de medicion
  for(int i=0; i < N_MEDIDAS; i++){
    digitalWrite(trigger,LOW); /* Por cuestión de estabilización del sensor*/
    delayMicroseconds(10);// ya que tarda segun las especificaciones unos 10us en pasar de un estado al otro
    digitalWrite(trigger, HIGH); /* envío del pulso ultrasónico*/
    delayMicroseconds(10); // ya que tarda segun las especificaciones unos 10us en pasar de un estado al otro
    tiempo = pulseIn(echo, HIGH); /* Función para medir la longitud del pulso entrante. Mide el tiempo que transcurrido entre el envío
    del pulso ultrasónico y cuando el sensor recibe el rebote, es decir: desde que el pin 12 empieza a recibir el rebote, HIGH, hasta que
    deja de hacerlo, LOW, la longitud del pulso entrante*/
    distancia = 10*((int(tiempo/58))+offsetUltra);//distancia= int(0.017*tiempo); /*fórmula para calcular la distancia obteniendo un valor entero se trata de una aproximacin de us/58*/
    //definimos que se encuentre dentro del rango establecido
    Serial.print("Distancia antes ");
    Serial.println(distancia);
    distancia=constrain(distancia, minMediaMovil, maxMediaMovil);
    Serial.print("Distancia despues ");
    Serial.println(distancia);
    //vamos preparando la media
    //Serial.println(distancia);
    media += distancia;
    
    //Se adquieren los datos para una media movil 
    mediaMovilUltra += distancia;
    nMediaMovilUltra++;

    //preparamos la moda
    //valoresModa[distancia]++;
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
  //moda=0;
  //valoresModa[0]=1;
  //for(int i=0;i<N_MODAS;i++){ if(valoresModa[i]>moda){ moda=i; } }
  
  Serial.println();
  Serial.print("MEDIANA: distancia ");
  Serial.print(medidas[N_MEDIDAS/2]);
  Serial.println(" cm");
  Serial.print("MEDIA: distancia ");
 // Serial.print(media);
  int resultado=media/N_MEDIDAS;
  Serial.print(resultado);
  Serial.println(" cm");
  Serial.print("MEDIA MOVIL: distancia ");
 // Serial.print(media);
  resultado=mediaMovilUltra/nMediaMovilUltra;
  Serial.print(resultado);
  Serial.println(" cm");
  //Serial.print("MODA: distancia ");
  //Serial.print(moda);
  Serial.println(" cm");
  
  return resultado;
}

/////////////////////////////////Reservar plaza
void reservar(){
  plaza = 'r';
  digitalWrite(ledAmarillo, HIGH);
}

///////////////////////////////// Eventos I2C

//Metodo receiveEvent
void receiveEvent(int howMany){
  char c='a';
  Serial.println("Recibe evento ");
  while(Wire.available()> 0 ){
    Serial.println("entra ");
    c = Wire.read();//con el read se coge 1 B, otro metodo para leeer X B de golpe
    Serial.print(" I2C: ");
    Serial.println(c);   
  }
  
  if(c=='r'){
    Serial.println("Reservar");
    reservar();
  }
}

void requestEvent(){
  Serial.print("------------------------------------Request Event: Estado plaza ");
  Serial.println(plaza);
  Wire.write(plaza);
}

//////////////////////////////////////////////// ESPERA ////////////////////////////////////////////////
void espera(long tiempoEspera){
  while(tiempoEspera > millis()){}
}

/////////////////////////////////////////////////////////////// Memoria EEPROM ///////////////////////////
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

/////////////////////////////////////////////////// Integración sensores Fuzzy ///////////////////////////////////
void initFuzzy(){
  //Paso 2 - Crear etiquetas
  //Etiquetas luz
  int cuarto = (mediasFoto[1] - mediasFoto[0])/4;
  int punto1 = mediasFoto[0] - cuarto;
  int punto2 = mediasFoto[1] + cuarto;
  //noExisteLuz = new FuzzySet(0, 0, punto1, punto2);
  existeLuz = new FuzzySet(0, 0, punto1, punto2);
  Serial.print("Puntos etiqueta existe luz ");
  Serial.print(0);
  Serial.print(", ");
  Serial.print(0);
  Serial.print(", ");
  Serial.print(punto1);
  Serial.print(", ");
  Serial.print(punto2);
  Serial.println();
  //existeLuz = new FuzzySet(punto1, punto2, 1000, 1000);
  noExisteLuz = new FuzzySet(punto1, punto2, 2000, 2000);
  Serial.print("Puntos etiqueta no existe luz ");
  Serial.print(punto1);
  Serial.print(", ");
  Serial.print(punto2);
  Serial.print(", ");
  Serial.print(1000);
  Serial.print(", ");
  Serial.print(1000);
  Serial.println();

  //Etiquetas distancia
  cuarto = (mediasUltra[1] - mediasUltra[2])/4;
  punto1 = mediasUltra[2] + cuarto;
  punto2 = mediasUltra[1] - cuarto;
  
  cocheAlto = new FuzzySet(0, 0, punto1, punto2);
  Serial.print("Puntos etiqueta coche alto ");
  Serial.print(0);
  Serial.print(", ");
  Serial.print(0);
  Serial.print(", ");
  Serial.print(punto1);
  Serial.print(", ");
  Serial.print(punto2);
  Serial.println();

  cuarto = (mediasUltra[0] - mediasUltra[1])/4;
  int punto3 = mediasUltra[1] + cuarto;
  int punto4 = mediasUltra[0] - cuarto;
  
  cocheBajo = new FuzzySet(punto1, punto2, punto3, punto4);
  Serial.print("Puntos etiqueta coche bajo ");
  Serial.print(punto1);
  Serial.print(", ");
  Serial.print(punto2);
  Serial.print(", ");
  Serial.print(punto3);
  Serial.print(", ");
  Serial.print(punto4);
  Serial.println();
  noExisteCoche = new FuzzySet(punto3, punto4, 210, 210);
  Serial.print("Puntos etiqueta sin coche ");
  Serial.print(punto3);
  Serial.print(", ");
  Serial.print(punto4);
  Serial.print(", ");
  Serial.print(210);
  Serial.print(", ");
  Serial.print(210);
  Serial.println();

  Serial.print("Foto 0 ");
  Serial.println(mediasFoto[0]);
  Serial.print("Foto 1 ");
  Serial.println(mediasFoto[1]);
  Serial.print("Foto 2 ");
  Serial.println(mediasFoto[2]);
  
  // Paso 3 - Creando conjunto de entrada de luz
 luz->addFuzzySet(noExisteLuz); // Se añade la etiqueta
 luz->addFuzzySet(existeLuz);

 fuzzy->addFuzzyInput(luz);

 //Igual para la distancia
 distancia->addFuzzySet(noExisteCoche); // Se añade la etiqueta
 distancia->addFuzzySet(cocheBajo);
 distancia->addFuzzySet(cocheAlto);
 fuzzy->addFuzzyInput(distancia); // Se añade el conjunto al objeto fuzzy
 
 // Paso 4 - Creando conjunto de salida
 coche->addFuzzySet(noHayCoche); // Añadir etiqueta al conjunto de salida
 coche->addFuzzySet(hayCoche);
 
 fuzzy->addFuzzyOutput(coche); // Añadir el conjunto de salida al objeto fuzzy
 
 //Paso 5 - Conjunto de reglas
 //Consecuentes
 FuzzyRuleConsequent* thenHayCoche = new FuzzyRuleConsequent(); //Creación de consecuente
 thenHayCoche->addOutput(hayCoche);// Juntando consecuente con salida

 FuzzyRuleConsequent* thenNoHayCoche = new FuzzyRuleConsequent();
 thenNoHayCoche->addOutput(noHayCoche);
 
 // Regla 1 - Si existe luz y no existe distancia, existe un coche
 FuzzyRuleAntecedent* siNoExisteLuzYnoExisteCoche = new FuzzyRuleAntecedent(); //Creando antecedente
 siNoExisteLuzYnoExisteCoche->joinWithAND(noExisteLuz, noExisteCoche); //Añadiendo etiquetas al antecedente
 
 FuzzyRule* fuzzyRule01 = new FuzzyRule(1, siNoExisteLuzYnoExisteCoche, thenHayCoche); // Creando la regla
 fuzzy->addFuzzyRule(fuzzyRule01); // Añadiendo regla al conjunto de reglas
  
 // Regla 2 - Si existe luz y coche alto, existe un coche
 FuzzyRuleAntecedent* siNoExisteLuzYcocheAlto = new FuzzyRuleAntecedent();
 siNoExisteLuzYcocheAlto->joinWithAND(noExisteLuz, cocheAlto);
 
 FuzzyRule* fuzzyRule02 = new FuzzyRule(2, siNoExisteLuzYcocheAlto, thenHayCoche);
 fuzzy->addFuzzyRule(fuzzyRule02);

 // Regla 3 - Si existe luz y coche bajo, existe un coche
 FuzzyRuleAntecedent* siNoExisteLuzYcocheBajo = new FuzzyRuleAntecedent();
 siNoExisteLuzYcocheBajo->joinWithAND(noExisteLuz, cocheBajo);
 
 FuzzyRule* fuzzyRule03 = new FuzzyRule(3, siNoExisteLuzYcocheBajo, thenHayCoche);
 fuzzy->addFuzzyRule(fuzzyRule03);

 // Regla 4 - Si no existe luz y no existe distancia, no existe un coche
 FuzzyRuleAntecedent* siExisteLuzYnoExisteCoche = new FuzzyRuleAntecedent();
 siExisteLuzYnoExisteCoche->joinWithAND(existeLuz, noExisteCoche);
 
 FuzzyRule* fuzzyRule04 = new FuzzyRule(4, siExisteLuzYnoExisteCoche, thenNoHayCoche);
 fuzzy->addFuzzyRule(fuzzyRule04);

 // Regla 5 - Si no existe luz y coche Alto, existe un coche
 FuzzyRuleAntecedent* siExisteLuzYcocheAlto = new FuzzyRuleAntecedent();
 siExisteLuzYcocheAlto->joinWithAND(existeLuz, cocheAlto);
 
 FuzzyRule* fuzzyRule05 = new FuzzyRule(5, siExisteLuzYcocheAlto, thenHayCoche);
 fuzzy->addFuzzyRule(fuzzyRule05);

 // Regla 6 - Si no existe luz y coche bajo, no existe un coche
 FuzzyRuleAntecedent* siExisteLuzYcocheBajo = new FuzzyRuleAntecedent();
 siExisteLuzYcocheBajo->joinWithAND(existeLuz, cocheBajo);
 
 FuzzyRule* fuzzyRule06 = new FuzzyRule(6, siExisteLuzYcocheBajo, thenNoHayCoche);
 fuzzy->addFuzzyRule(fuzzyRule06); 
}

//Usar el fuzzy para coger los valores
int getFuzzyValue(float luz, int distancia){
  //Paso 7 - Iniciar entradas y fuzzificar
  /*
  Serial.print("Datos que llegan: Luz ");
  Serial.print(luz);
  Serial.print(" distancia ");
  Serial.println(distancia);
  */
  
  fuzzy->setInput(1, luz);
  fuzzy->setInput(2, distancia);
 
  fuzzy->fuzzify();

  //Ver pertenencia a cada etiqueta
  Serial.print("Luz: ");
  Serial.print(" No existe luz: ");
  Serial.print(noExisteLuz->getPertinence());
  Serial.print(", Existe luz: ");
  Serial.println(existeLuz->getPertinence());
   
  Serial.print("Distancia: ");
  Serial.print("No existe distancia: ");
  Serial.print(noExisteCoche->getPertinence());
  Serial.print(", Coche alto: ");
  Serial.print(cocheAlto->getPertinence());
  Serial.print(", coche bajo: ");
  Serial.println(cocheBajo->getPertinence());

 //Paso 8 - Defuzzificar y coger valor salida
  int coche = 0;
  coche = fuzzy->defuzzify(1);
   
  Serial.print("Existe coche? ");
  Serial.println(coche);

  return coche;
}




