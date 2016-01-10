//Archivo en proceso

#include <Wire.h>
#include <EEPROM.h>

#include <Fuzzy.h>
#include <FuzzyComposition.h>
#include <FuzzyInput.h>
#include <FuzzyIO.h>
#include <FuzzyOutput.h>
#include <FuzzyRule.h>
#include <FuzzyRuleAntecedent.h>
#include <FuzzyRuleConsequent.h>
#include <FuzzySet.h>

// Paso 1 - Instanciando un objeto de la librería fuzzy
Fuzzy* fuzzy = new Fuzzy();

//Paso 2 - Crear etiquetas
//Etiquetas luz
FuzzySet* noExisteLuz = new FuzzySet(0, 0, 275, 425);
FuzzySet* existeLuz = new FuzzySet(275, 425, 800, 800);

//Etiquetas distancia (Multiplicado por 10)
FuzzySet* cocheAlto = new FuzzySet(0, 0, 127.5, 142.5);
FuzzySet* cocheBajo = new FuzzySet(127.5, 142.5, 155, 165);
FuzzySet* noExisteDistancia = new FuzzySet(165, 170, 210, 210);

//Etiquetas salida
FuzzySet* noExisteCoche = new FuzzySet(0, 0, 0, 0);
FuzzySet* existeCoche = new FuzzySet(1, 1, 1, 1);

//variable para comprobar si ya se ha calibrado
char calibrado;

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
const int NMODAS=400;//numero de medidas posibles en la calibracion
const int maxmediamovil=190;//valor maximo correcto para el ultrasonido (media movil) //modificado
const int minmediamovil=70; //valor minimo correcto para el ultrasonido (media movil) //modificado
const int tolmediamovil=20;//valor de tolerancia para ultrasonido (media movil) //modificado

const int ESCENARIOSCALIBRAR=3; //numero de escenarios para calibrar
const char escenarios [ESCENARIOSCALIBRAR][20]={"VACIO","ALTO","BAJO"}; //nombre de los escenarios a calibrar
int mediasfoto[ESCENARIOSCALIBRAR];
int mediasultra[ESCENARIOSCALIBRAR];

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
const int NUMVALORESFOTO=8;

int valorfoto=0;

//creacion de la tabla look-up insertando los valores experimentales
int tablalu[NUMVALORESFOTO][2]={{306,423},{362,473},{471,491},{578,595},{647,611},{1033,656},{1303,699},{1343,719}};

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

void setup(){

  //Iniciar fuzzy
  initFuzzy();
  
  //inicializamos la comunicacion a traves de I2C
  
  Wire.begin(5);
  Wire.onReceive(receiveEvent);
  
  Serial.begin(9600);
  pinMode(trigger, OUTPUT); /*activación del pin 9 como salida: para el pulso ultrasónico*/
  pinMode(echo, INPUT); /*activación del pin 8 como entrada: tiempo del rebote del ultrasonido*/
  //calibrado=false; //indica que aun no se ha calibrado el sistema //modificado
  
  //Se recupera si ya se calibró el sistema o no junto con la información almacenada en caso positivo
  calibrado = loadEEPROM(dirCalibrado);
  if(calibrado == 's'){
    Serial.println("Recuperando calibracion");
    for(int i=0;i<ESCENARIOSCALIBRAR;i++)
     {
       Serial.println("calibrando LDR");
       mediasfoto[i] = loadEEPROM(dirLDR + i*4); //modificado
       Serial.println("calibrando ULTRA");;
       mediasultra[i] = loadEEPROM(dirUltra + i*4); //modificado
     }
  }

  pinMode(ledRojo,OUTPUT);
  pinMode(ledVerde,OUTPUT);
}

void loop(){
  /*for(int i=0;i<8;i++){
  for(int j=0;j<2;j++){
    Serial.print(tablalu[i][j]);
    Serial.print("---");
  }
  Serial.println();
}*/
  
  int mediafoto;
  int mediaultra;
  int opcion;
  
  /*while(opcion!=1 && !calibrado)
  {
    //MENSAJE DE QUE NECESITA CALIBRARSE PRIMERO
    
  }*/
  Serial.println("INICIO");
  opcion=Serial.read();

  switch(opcion)
  {    
    case '1':
     for(int i=0;i<ESCENARIOSCALIBRAR;i++)
     {
         while (Serial.read() !='0') {
             Serial.println("INTRODUZCA 0 PARA CONTINUAR");
         }
       Serial.println(escenarios[i]);
       
       
       digitalWrite(ledRojo,HIGH);
       digitalWrite(ledVerde,LOW);
       Serial.println("calibrando LDR");
       mediasfoto[i]=calibraLDR();
       saveEEPROM(mediasfoto[i], dirLDR+ i*4); //modificado
       Serial.println("calibrando ULTRA");
       mediasultra[i]=calibraUltrasonico();
       saveEEPROM(mediasultra[i], dirUltra+ i*4); //modificado
       digitalWrite(ledRojo,LOW);
       digitalWrite(ledVerde,HIGH);
     }
     
     calibrado='s'; //modificado
     saveEEPROM(calibrado, dirCalibrado);
     
    break;
  }
  
  delay(500);
}

int valorLookUp(int entrada){
  int resultado=0;
  
  int valorfiltrado=0;
  
  if(entrada<tablalu[0][1] || entrada>tablalu[NUMVALORESFOTO-1][1]){
    
    entrada=constrain(entrada,tablalu[0][1],tablalu[NUMVALORESFOTO-1][1]);
    if(entrada==tablalu[0][1]){
      resultado=tablalu[0][0];
    }else{
       resultado= tablalu[NUMVALORESFOTO-1][0];
    }
  }else{
    
     //La calibracion se realiza a traves de una regresion por cuadrados minimos.
      int i=0;
      boolean encontrado=false;
      while(i+1<NUMVALORESFOTO && !encontrado){
        
        if(i+1>=entrada)
        {
          
          int x=entrada;
          int x0=tablalu[i][1];
          int x1=tablalu[i+1][1];
          int y0=tablalu[i][0];
          int y1=tablalu[i+1][0];
          encontrado=true;
          resultado= y0+(((x-x0)/(x1-x0))*(y1-y0));
        }
        i++;
        
        
        
      }
  }
    
  return resultado;
}

int calibraLDR(){
  
  //inicializacion de media,mediana y moda al comenzar el ciclo de medicion.
  media=0;
  //moda=0;

  for(int i=0;i<NMEDIDAS;i++){
    medidas[i]=0;
  }
  //comienzo del ciclo de medicion
  
  for(int i=0;i<NMEDIDAS;i++){
   // Serial.print("es la vez: ");
   // Serial.println(i+1);
      
      valorfoto=analogRead(foto);
    //  Serial.print("comienza lookup con");
    //  Serial.println(valorfoto);
      int valor=valorLookUp(valorfoto);
   //   Serial.println("termina look up");
      //vamos preparando la media
      //Serial.println(valor);
      media+=valor;
    
      //insertamos de forma ordenada
        int j=0;
        int k;
  
        while( j<i && valor<medidas[j] ){
            j++;  
         };
         
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
    Serial.print("MEDIANA: distancia ");
    Serial.print(medidas[NMEDIDAS/2]);
    Serial.println(" lux");
    Serial.print("MEDIA: distancia ");
   // Serial.print(media);
   int resultado=media/NMEDIDAS;
    Serial.print(resultado);
    Serial.println(" lux");
    
    return resultado;
}

int calibraUltrasonico(){
   //inicializacion de media,mediana y moda al comenzar el ciclo de medicion.
  media=0;
  //moda=0;
  for(int i=0;i<NMODAS;i++)
  {
    valoresmoda[i]=0;
  }
  for(int i=0;i<NMEDIDAS;i++)
  {
    medidas[i]=0;
  }
  
  //comienzo del ciclo de medicion
  
  for(int i=0;i<NMEDIDAS;i++)
  {
      digitalWrite(trigger,LOW); /* Por cuestión de estabilización del sensor*/
      delayMicroseconds(10);// ya que tarda segun las especificaciones unos 10us en pasar de un estado al otro
      digitalWrite(trigger, HIGH); /* envío del pulso ultrasónico*/
      delayMicroseconds(10); // ya que tarda segun las especificaciones unos 10us en pasar de un estado al otro
      tiempo=pulseIn(echo, HIGH); /* Función para medir la longitud del pulso entrante. Mide el tiempo que transcurrido entre el envío
      del pulso ultrasónico y cuando el sensor recibe el rebote, es decir: desde que el pin 12 empieza a recibir el rebote, HIGH, hasta que
      deja de hacerlo, LOW, la longitud del pulso entrante*/
      distancia= 10*((int(tiempo/58))+offsetultra);//distancia= int(0.017*tiempo); /*fórmula para calcular la distancia obteniendo un valor entero se trata de una aproximacin de us/58*/ //modificado
      //definimos que se encuentre dentro del rango establecido
      distancia=constrain(distancia,30,4000); //modificado
      //vamos preparando la media
      //Serial.println(distancia);
      media+=distancia;
      //Se adquieren los datos para una media movil 
      if(minmediamovil-tolmediamovil>distancia && distancia<maxmediamovil+tolmediamovil){
          mediamovilultra=mediamovilultra+distancia;
          nmediamovilultra=nmediamovilultra+1;
      }
      int mediamovilultra[ESCENARIOSCALIBRAR];

      //preparamos la moda
      valoresmoda[distancia]++;
      //insertamos de forma ordenada
        
        int j=0;
        int k;
  
        while( j<i && distancia<medidas[j] ){
            j++;  
         };
         
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
    int resultado = 0;
 
    Serial.println();
    Serial.print("MEDIANA: distancia ");
    Serial.print(medidas[NMEDIDAS/2]);
    resultado = medidas[NMEDIDAS/2]; //modificado
    Serial.println(" cm");
    Serial.print("MEDIA: distancia ");
   // Serial.print(media);
    //resultado=media/NMEDIDAS; //modificado
    Serial.print(resultado);
    Serial.println(" cm");
    Serial.print("MEDIA MOVIL: distancia ");
   // Serial.print(media);
    //resultado=mediamovilultra/nmediamovilultra; //modificado
    Serial.print(resultado);
    Serial.println(" cm");
    Serial.print("MODA: distancia ");
    Serial.print(moda);
    Serial.println(" cm");
    
    return resultado;
}


//Metodo receiveEvent

void receiveEvent(int opt)
{
  Serial.print("al menos llega");
  while(0<Wire.available())
  {
    char c=Wire.read();//con el read se coge 1 B, otro metodo para leeer X B de golpe
    Serial.print("recibo a traves de I2C: ");
    Serial.println(c);
    if(c=='c'){
     
      for(int i=0;i<ESCENARIOSCALIBRAR;i++)
     {
       Serial.print("escenario: ");
       Serial.println(i);
       c='0';
         Serial.print("*********************************************************");
       Serial.print("termina el ");
       Serial.println(i);
       //probar con ack/currentmillis 
       delay(15000);
      /* Wire.write("ok");
       char c=Wire.read();
        while (c !='c') {
          char c=Wire.read();
         }*/
       Serial.println(escenarios[i]);
       
       
       digitalWrite(ledRojo,HIGH);
        digitalWrite(ledVerde,LOW);
       Serial.println("calibrando LDR");
       mediasfoto[i]=calibraLDR();
       Serial.println("calibrando ULTRA");
       mediasultra[i]=calibraUltrasonico();  
       digitalWrite(ledRojo,LOW);
       digitalWrite(ledVerde,HIGH);
       
     }
      
    }
  }  
}

/////////////////////////////////////////////////////////////// Memoria EEPROM ///////////////////////////

void saveEEPROM( int data, int start){
  //Serial.print("Guardo ");
  //Serial.println(data);
  byte L;
  byte H;
  
  //Se almacenan los bytes más y menos significativos.
  H = highByte(data);
  //Serial.print("Parte alta ");
  //Serial.println(H);
  
  L = lowByte(data);
  //Serial.print("Parte baja ");
  //Serial.println(L);
  
  //Se escriben los bytes en direcciones de memoria contiguas.
  EEPROM.write(start, H);
  EEPROM.write(start + 1, L);
}

int loadEEPROM(int start){
  byte H = EEPROM.read(start);
  //Serial.print("Parte alta ");
  //Serial.println(H);
  
  byte L = EEPROM.read(start + 1);
  //Serial.print("Parte baja ");
  //Serial.println(L);
  
  //Variable que almacenará el resultado final.
  //int data = H * 256 + L;
  
  return H * 256 + L;
}

/////////////////////////////////////////////////// Integración sensores Fuzzy ///////////////////////////////////

void initFuzzy(){
  // Paso 3 - Creando conjunto de entrada de luz
  FuzzyInput* luz = new FuzzyInput(1);// Como parámetro necesita un ID
 luz->addFuzzySet(noExisteLuz); // Se añade la etiqueta
 luz->addFuzzySet(existeLuz);

 fuzzy->addFuzzyInput(luz);

 //Igual para la distancia
 FuzzyInput* distancia = new FuzzyInput(2);// Como parámetro necesita un ID
 distancia->addFuzzySet(noExisteDistancia); // Se añade la etiqueta
 distancia->addFuzzySet(cocheBajo);
 distancia->addFuzzySet(cocheAlto);
 fuzzy->addFuzzyInput(distancia); // Se añade el conjunto al objeto fuzzy
 
 // Paso 4 - Creando conjunto de salida
 FuzzyOutput* coche = new FuzzyOutput(1);// ID de parámetro
 coche->addFuzzySet(noExisteCoche); // Añadir etiqueta al conjunto de salida
 coche->addFuzzySet(existeCoche);
 
 fuzzy->addFuzzyOutput(coche); // Añadir el conjunto de salida al objeto fuzzy
 
 //Paso 5 - Conjunto de reglas

 //Consecuentes
 FuzzyRuleConsequent* thenExisteCoche = new FuzzyRuleConsequent(); //Creación de consecuente
 thenExisteCoche->addOutput(existeCoche);// Juntando consecuente con salida

 FuzzyRuleConsequent* thenNoExisteCoche = new FuzzyRuleConsequent();
 thenNoExisteCoche->addOutput(noExisteCoche);
 
 // Regla 1 - Si existe luz y no existe distancia, existe un coche
 FuzzyRuleAntecedent* siNoExisteLuzYnoExisteDistancia = new FuzzyRuleAntecedent(); //Creando antecedente
 siNoExisteLuzYnoExisteDistancia->joinWithAND(noExisteLuz, noExisteDistancia); //Añadiendo etiquetas al antecedente
 
 FuzzyRule* fuzzyRule01 = new FuzzyRule(1, siNoExisteLuzYnoExisteDistancia, thenExisteCoche); // Creando la regla
 fuzzy->addFuzzyRule(fuzzyRule01); // Añadiendo regla al conjunto de reglas
  
 // Regla 2 - Si existe luz y coche alto, existe un coche
 FuzzyRuleAntecedent* siNoExisteLuzYcocheAlto = new FuzzyRuleAntecedent();
 siNoExisteLuzYcocheAlto->joinWithAND(noExisteLuz, cocheAlto);
 
 FuzzyRule* fuzzyRule02 = new FuzzyRule(2, siNoExisteLuzYcocheAlto, thenExisteCoche);
 fuzzy->addFuzzyRule(fuzzyRule02);

 // Regla 3 - Si existe luz y coche bajo, existe un coche
 FuzzyRuleAntecedent* siNoExisteLuzYcocheBajo = new FuzzyRuleAntecedent();
 siNoExisteLuzYcocheBajo->joinWithAND(noExisteLuz, cocheBajo);
 
 FuzzyRule* fuzzyRule03 = new FuzzyRule(3, siNoExisteLuzYcocheBajo, thenExisteCoche);
 fuzzy->addFuzzyRule(fuzzyRule03);

 // Regla 4 - Si no existe luz y no existe distancia, no existe un coche
 FuzzyRuleAntecedent* siExisteLuzYnoExisteDistancia = new FuzzyRuleAntecedent();
 siExisteLuzYnoExisteDistancia->joinWithAND(existeLuz, noExisteDistancia);
 
 FuzzyRule* fuzzyRule04 = new FuzzyRule(4, siExisteLuzYnoExisteDistancia, thenNoExisteCoche);
 fuzzy->addFuzzyRule(fuzzyRule04);

 // Regla 5 - Si no existe luz y coche Alto, existe un coche
 FuzzyRuleAntecedent* siExisteLuzYcocheAlto = new FuzzyRuleAntecedent();
 siExisteLuzYcocheAlto->joinWithAND(existeLuz, cocheAlto);
 
 FuzzyRule* fuzzyRule05 = new FuzzyRule(5, siExisteLuzYcocheAlto, thenExisteCoche);
 fuzzy->addFuzzyRule(fuzzyRule05);

 // Regla 6 - Si no existe luz y coche bajo, no existe un coche
 FuzzyRuleAntecedent* siExisteLuzYcocheBajo = new FuzzyRuleAntecedent();
 siExisteLuzYcocheBajo->joinWithAND(existeLuz, cocheBajo);
 
 FuzzyRule* fuzzyRule06 = new FuzzyRule(6, siExisteLuzYcocheBajo, thenNoExisteCoche);
 fuzzy->addFuzzyRule(fuzzyRule06); 
}

//Usar el fuzzy para coger los valores
int getFuzzyValue(float luz, float distancia){
  //Paso 7 - Iniciar entradas y fuzzificar
  fuzzy->setInput(1, luz);
  fuzzy->setInput(2, distancia);
 
  fuzzy->fuzzify();

  //Ver pertenencia a cada etiqueta
  Serial.print("Luz: ");
  Serial.print("no existe luz: ");
  Serial.println(noExisteLuz->getPertinence());
  Serial.print(", existe luz");
  Serial.print(existeLuz->getPertinence());
   
  Serial.print("Distancia: ");
  Serial.print("No existe distancia: ");
  Serial.print(noExisteDistancia->getPertinence());
  Serial.print(", Coche alto: ");
  Serial.print(cocheAlto->getPertinence());
  Serial.print(", cocheBajo");
  Serial.println(cocheBajo->getPertinence());

 //Paso 8 - Defuzzificar y coger valor salida
  int coche = fuzzy->defuzzify(1);
   
  Serial.print("Existe coche? ");
  Serial.println(coche);

  return coche;
}



