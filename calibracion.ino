//variables donde se almacenan la distancia en cm y el tiempo en us
long distancia;
long tiempo;

//definicion de las constantes utilizadas
const int echo=6; //pin de arduino donde se conecta echo
const int trigger=7;// pin de arduino donde se conecta trigger
const int offsetultra=2; // offset calculado de forma manual 
const int NMEDIDAS=10; //numero de medidas para la calibracion
const int NMODAS=400;//numero de medidas posibles en la calibracion
const int maxmediamovil=17;//valor maximo correcto para el ultrasonido (media movil)
const int minmediamovil=9;//valor minimo correcto para el ultrasonido (media movil)

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

void setup(){
  
  
  Serial.begin(9600);
  pinMode(trigger, OUTPUT); /*activación del pin 9 como salida: para el pulso ultrasónico*/
  pinMode(echo, INPUT); /*activación del pin 8 como entrada: tiempo del rebote del ultrasonido*/

}

void loop(){
  
  int mediafoto;
  int mediaultra;
  int opcion;
      
     for(int i=0;i<ESCENARIOSCALIBRAR;i++)
     {
         while (Serial.read() !='0') {
             Serial.println("INTRODUZCA 0 PARA CONTINUAR");

         }
       Serial.println(escenarios[i]);
       Serial.println("calibrando LDR");
       mediasfoto[i]=calibraLDR();
       Serial.println("calibrando ULTRA");
       mediasultra[i]=calibraUltrasonico();  
   
     }
     
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

  for(int i=0;i<NMEDIDAS;i++)
  {
    medidas[i]=0;
  }
  
  
  //comienzo del ciclo de medicion
  
  for(int i=0;i<NMEDIDAS;i++)
  {
      
      valorfoto=analogRead(foto1);
      int valor=valorLookUp(valorfoto);
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

    Serial.println();
    Serial.print("MEDIANA: distancia ");
    Serial.print(medidas[NMEDIDAS/2]);
    Serial.println(" lux");
    Serial.print("MEDIA: distancia ");
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
      digitalWrite(trigger,LOW); /* se estabiliza el sensor*/
      delayMicroseconds(10);// ya que tarda segun las especificaciones unos 10us en pasar de un estado al otro
      digitalWrite(trigger, HIGH); /* envío del pulso ultrasónico*/
      delayMicroseconds(10); // ya que tarda segun las especificaciones unos 10us en pasar de un estado al otro
      tiempo=pulseIn(echo, HIGH); /* Función para medir la longitud del pulso entrante. Mide el tiempo que transcurrido entre el envío
      del pulso ultrasónico y cuando el sensor recibe el rebote, es decir: desde que el pin 12 empieza a recibir el rebote, HIGH, hasta que
      deja de hacerlo, LOW, la longitud del pulso entrante*/
      distancia= ((int(tiempo/58))+offsetultra);//distancia= int(0.017*tiempo); /*fórmula para calcular la distancia obteniendo un valor entero se trata de una aproximacin de us/58*/
      //definimos que se encuentre dentro del rango establecido
      distancia=constrain(distancia,3,400);
      //vamos preparando la media
      //Serial.println(distancia);
      media+=distancia;
      //Se adquieren los datos para una media movil simplificada
      if(minmediamovil>distancia && distancia<maxmediamovil){
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
    
    Serial.println();
    Serial.print("MEDIANA: distancia ");
    Serial.print(medidas[NMEDIDAS/2]);
    Serial.println(" cm");
    Serial.print("MEDIA: distancia ");
    int resultado=media/NMEDIDAS;
    Serial.print(resultado);
    Serial.println(" cm");
    Serial.print("MEDIA MOVIL: distancia ");
    resultado=mediamovilultra/nmediamovilultra;
    Serial.print(resultado);
    Serial.println(" cm");
    Serial.print("MODA: distancia ");
    Serial.print(moda);
    Serial.println(" cm");
    
    return resultado;
}

