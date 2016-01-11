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

//Declaracion del nombre de ESTADOS y de EVENTOS
#define STATE1  	0x01
#define STATE2  	0x02
#define STATE3 	  0x03
#define STATE4    0x04
#define STATE5    0x05


#define PETICION0    	0x21
#define PETICION1     0x23
#define PETICION2     0x21

// Estructuras descriptivas de mi diagrama de flujo
const FSMClass::FSM_State_t FSM_State[] PROGMEM= {
// STATE,STATE_FUNC
{STATE1,inicio},
{STATE2,calibrar},
{STATE3,abrir},
{STATE4,cerrar},
{STATE5,verPlazas},
};

//Transiciones entre estados
const FSMClass::FSM_NextState_t FSM_NextState[] PROGMEM= {
// STATE,EVENT,NEXT_STATE
{STATE1,PETICION0,STATE2},
{STATE2,0,STATE1},
{STATE1,PETICION1,STATE3},
{STATE3,0,STATE4},
{STATE4,0,STATE1},
{STATE1,PETICION2,STATE5},
{STATE5,0,STATE1},
};


//Macros para el cálculo de los tamaños de las estructuras
//NO MODIFICAR
#define nStateFcn		sizeof(FSM_State)/sizeof(FSMClass::FSM_State_t)
#define nStateMachine		sizeof(FSM_NextState)/sizeof(FSMClass::FSM_NextState_t)

#endif
