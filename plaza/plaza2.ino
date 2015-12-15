//Archivo en proceso

#include <Wire.h>

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
const int maxmediamovil=17;//valor maximo correcto para el ultrasonido (media movil)
const int maxmediamovil=190;//valor maximo correcto para el ultrasonido (media movil) //modificado
const int minmediamovil=70; //valor minimo correcto para el ultrasonido (media movil) //modificado

const int ESCENARIOSCALIBRAR=5; //numero de escenarios para calibrar
const char escenarios [ESCENARIOSCALIBRAR][200]={"VACIO","CORTO","LARGO","ALTO","BAJO"}; //nombre de los escenarios a calibrar
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
const int foto1=A0;//pin para la medicion del LDR
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
  
  //inicializamos la comunicacion a traves de I2C
  
  Wire.begin(5);
  Wire.onReceive(receiveEvent);
  
  Serial.begin(9600);
  pinMode(trigger, OUTPUT); /*activación del pin 9 como salida: para el pulso ultrasónico*/
  pinMode(echo, INPUT); /*activación del pin 8 como entrada: tiempo del rebote del ultrasonido*/
  //calibrado=false; //indica que aun no se ha calibrado el sistema //modificado
  
  //Se recupera si ya se calibró el sistema o no junto con la información almacenada en caso positivo
  calibrado = loadEEPROM(calibrado, dirCalibrado);
  if(calibrado == 's'){
    Serial.println("Recuperando calibracion");
    for(int i=0;i<ESCENARIOSCALIBRAR;i++)
     {
       Serial.println("calibrando LDR");
       mediasfoto[i] = loadEEPROM(mediasfoto[i], dirLDR + i*5); //modificado
       Serial.println("calibrando ULTRA");;
       mediasultra[i] = loadEEPROM(mediasultra[i], dirUltra + i*5); //modificado
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
       saveEEPROM(mediasfoto[i], dirLDR+ i*5); //modificado
       Serial.println("calibrando ULTRA");
       mediasultra[i]=calibraUltrasonico();
       saveEEPROM(mediasultra[i], dirUltra+ i*5); //modificado
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

  for(int i=0;i<NMEDIDAS;i++)
  {
    medidas[i]=0;
  }
  
  
  //comienzo del ciclo de medicion
  
  for(int i=0;i<NMEDIDAS;i++)
  {
   // Serial.print("es la vez: ");
   // Serial.println(i+1);
      
      valorfoto=analogRead(foto1);
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

int saveEEPROM( int data, int start){
	int aux = data;
	byte b;
	Serial.begin(9600);
   
   //sizeof devuelve el numero de bytes
   for (int i = start; i < sizeof(data); i++){
         //Se guarda el byte más significativo
         b = highByte(aux);
         //EEPROM.write(i, b);
    
         Serial.println(b);
         //Se desplazan 8 bits hacia la izquierda de modo que los 8 primeros
         //bits se pierden y los 8 siguientes pasan a ser los primeros
         aux = aux << 8;
   }
   return aux;
}

int loadEEPROM(int data, int start){
	int result = 0;
	int aux = 0;
	Serial.begin(9600);
	for (int i = sizeof(data) + start; i > 0 ; i--){
    		//Se almacena el valor de la EEPROM en los bits menos significativos.
    		 aux = EEPROM.read(i-1);
    		 //Se desplazan los bits tantos bytes como sea necesario, teniendo en
    		 //cuenta que el byte final no debe ser desplazado.
    		 if (i != 1){
                    aux = aux << (8*i);
    		 }
    		 //Se almacena en la variable final el resultado de la operación OR de
    		 //ambos datos. Al realizar la operación OR con una variable que solo
    		 //contiene ceros (variable Dato), el resultado es el valor de la otra
    		 //variable. OR implica => 0+0=0, 0+1=1, 1+0=1, 1+1=1.
    		 result = result | aux;
    		 //Se limpia la variable auxiliar para que en las siguientes iteraciones
    		 //no se cambie el resultado de los otros bytes.
    		 aux = 0;
	}
	     Serial.println(result);
	return result;
}

