//-----------------------------------------------------
//ESTE ES EL ÚNICO FICHERO QUE HAY QUE MODIFICAR
//-----------------------------------------------------

#ifndef myStates_H
#define myStates_H


//Declaracion de las funciones
extern void abrir(void);
extern void cerrar(void);
extern void verPlazas(void);
extern void calibrar(void);
extern void inicio(void);
extern void ocuparPlaza(void);
extern void liberarPlaza(void);
extern void pedirPlaza(void);
extern void logo(void);

//Declaracion del nombre de ESTADOS y de EVENTOS
#define INICIO    0x01
#define MANUAL  	0x02
#define CALIBRADO  	0x03
#define APERTURA 	  0x04
#define CIERRE    0x05
#define CONSULTAPLAZAS    0x06
#define OCUPAPLAZA    0x07
#define LIBERAPLAZA    0x08
#define PEDIRPLAZA    0x09



#define PETCALIBRADO    	0x21
#define PETAPERTURA     0x23
#define PETCONSULTA     0x25
#define LIBRE         0x27
#define OCUPADA       0x29
#define PIDEPLAZA     0x31
#define ACTIVAMANUAL  0x33

// Estructuras descriptivas de mi diagrama de flujo
const FSMClass::FSM_State_t FSM_State[] PROGMEM= {
// STATE,STATE_FUNC
{INICIO,logo},
{MANUAL,inicio},
{CALIBRADO,calibrar},
{APERTURA,abrir},
{CIERRE,cerrar},
{CONSULTAPLAZAS,verPlazas},
{OCUPAPLAZA,ocuparPlaza},
{LIBERAPLAZA,liberarPlaza},
{PEDIRPLAZA,pedirPlaza},
};

//Transiciones entre estados
const FSMClass::FSM_NextState_t FSM_NextState[] PROGMEM= {
// STATE,EVENT,NEXT_STATE
//{MANUAL,0,MANUAL},
{INICIO,0,INICIO},
{INICIO,ACTIVAMANUAL,MANUAL},
{MANUAL,PETCALIBRADO,CALIBRADO},
{CALIBRADO,0,INICIO},
{MANUAL,PETAPERTURA,APERTURA},
{APERTURA,0,CIERRE},
{CIERRE,0,INICIO},
{MANUAL,PETCONSULTA,CONSULTAPLAZAS},
{CONSULTAPLAZAS,0,INICIO},
{INICIO,OCUPADA,OCUPAPLAZA},
{OCUPAPLAZA,0,INICIO},
{INICIO,LIBRE,LIBERAPLAZA},
{LIBERAPLAZA,0,INICIO},
{MANUAL,PIDEPLAZA,PEDIRPLAZA},
{PEDIRPLAZA,0,INICIO},
};


//Macros para el cálculo de los tamaños de las estructuras
//NO MODIFICAR
#define nStateFcn		sizeof(FSM_State)/sizeof(FSMClass::FSM_State_t)
#define nStateMachine		sizeof(FSM_NextState)/sizeof(FSMClass::FSM_NextState_t)

#endif

