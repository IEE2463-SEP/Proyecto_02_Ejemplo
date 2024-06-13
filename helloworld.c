#include <stdio.h>
#include <sleep.h>
#include <time.h>
#include <unistd.h>
#include <math.h>
#include <stdlib.h>
#include <sys/time.h>
#include "platform.h"
#include "xil_printf.h"
#include "xparameters.h"
#include "xgpio.h"
#include "xstatus.h"
#include "Delay.h"
#include "LCD_SPI.h"
#include "LCD_Driver.h"
#include "LCD_GUI.h"
#include "ADC.h"
#include "I2C.h"
#include "xil_io.h"
#include "xtmrctr.h"
#include "xscugic.h"
#include "xil_exception.h"
#include "sdCard.h"

XScuGic INTCInst;
XTmrCtr TMRInst;
XTmrCtr TMRInst1;
XGpio gpio_luz;
static int tmr_count;
static int tmr_count1;

extern XGpio gpio0;
extern XSpi  SpiInstance;	 /* The instance of the SPI device */
extern XSpi  SpiInstance1;
extern const unsigned char font[] ;

#define BACKGROUND  WHITE
#define FOREGROUND BLUE
#define DELAY 1000

// Puntero
#define PAUSA 0x43C00000

// Estructura
struct MiEstructura {
    int TOP1;
    int TOP2;
    int TOP3;
	int TOP4;
	int TOP5;
};



//#define tamaño 3
#define limite_sup 1000
#define limite_inf 10
int posx[50] = {2};
int posy[50] = {24};
int TOP5[5] = {2000,1050,140,100,10};
char top1[16] = {};
char top2[16] = {};
char top3[16] = {};
char top4[16] = {};
char top5[16] = {};
int largo = 1;
int movx = 2;
int movy = 0;
int p = 1;
int nivel = 1;
int print_puntos = 0;
int seed = 1;
int existencia = 0;
int se_mueve = 0;
int Mx = 0;
int My = 0;
int score = 0;
int estado = 0;
int y_max = 650;
int counter0 = 0;
int counter1 = 0;
int una_vez = 0;
int stop = 0;

// -------Tarjeta SD -----------

FIL* fptr;
char dataBuffer[1024];
char *dataPntr = dataBuffer;
int logNum = 0;
#define MAX_LOG_NUM 5

/****** Macros ***********/
// Parameter definitions
#define INTC_DEVICE_ID 	XPAR_PS7_SCUGIC_0_DEVICE_ID
#define TMR_DEVICE_ID 	XPAR_TMRCTR_0_DEVICE_ID
#define TMR_DEVICE_ID_1 	XPAR_TMRCTR_1_DEVICE_ID
#define LUZ_DEVICE_ID       XPAR_AXI_GPIO_LUZ_DEVICE_ID
#define INTC_TMR_INTERRUPT_ID XPAR_FABRIC_AXI_TIMER_0_INTERRUPT_INTR
#define INTC_TMR_INTERRUPT_ID_1 XPAR_FABRIC_AXI_TIMER_1_INTERRUPT_INTR
#define INTC_LUZ_INTERRUPT_ID 	XPAR_FABRIC_AXI_GPIO_LUZ_IP2INTC_IRPT_INTR

#define LUZ_INT 	XGPIO_IR_CH1_MASK

uint32_t TMR_LOAD; // 1s Timer // 0xFFC82FFF
#define TMR_LOAD1 0xF8000000 // 1s Timer

/****** Function declaration ********/
//------------------------------------
//PROTOTYPE FUNCTIONS
//--------------------------------------------
static void TMR_Intr_Handler(void *baseaddr_p);
static void TMR_Intr_Handler1(void *baseaddr_p);
static void LUZ_Intr_Handler1(void *baseaddr_p);
static int InterruptSystemSetup(XScuGic *XScuGicInstancePtr);
static int IntcInitFunction(u16 DeviceId, XGpio *GpioInstancePtr, XTmrCtr *TmrInstancePtr,XTmrCtr *TmrInstancePtr1 );

void generarManzana();

void PRINT_SNAKE();

void JUEGO();

void MENU ();

void PUNTAJES();

void CHECK_STATE();

void GAMEOVER();

void CALIENTE();

void MUSICA();

void SET_DIFICULTY();
//-----------------------------
// INTERRUPT HANDLER FUNCTIONS
// ---------------------------------------

void LUZ_Intr_Handler1(void *data){

	xil_printf("---------------------INTERRUPCION DE LUZ ACTIVADO ----------------------------------\n\r");

	//Disable GPIO interrupts
	XGpio_InterruptDisable(&gpio_luz, LUZ_INT);

	if ((XGpio_InterruptGetStatus(&gpio_luz) & LUZ_INT) != LUZ_INT){
		return;
	}
	xil_printf("Luz interrupt se leyo:%i\n\r", read_opt());
	if (estado == 1) {
		generarManzana();
	}

	(void)XGpio_InterruptClear(&gpio_luz, LUZ_INT);

	XGpio_InterruptEnable(&gpio_luz, LUZ_INT);
	xil_printf("---------------------INTERRUPCION DE LUZ TERMINADO ----------------------------------\n\r");
}
void TMR_Intr_Handler(void *data)
{
	if (XTmrCtr_IsExpired(&TMRInst,0)){
		//Once timer has expired 3 times, stop increment counter
		// reset timer and start running again
		if(tmr_count == 1){
			XTmrCtr_Stop(&TMRInst,0);
			int pot2 = read_POT2();
			if (pot2 > 900) {
				se_mueve = 1;
				stop = 0;
			}
			else {
				se_mueve = 0;
				stop = 1;
			}
			//xil_printf("Timer0 Interrupt \n\r");
			XTmrCtr_Reset(&TMRInst,0);
			XTmrCtr_Start(&TMRInst,0);
		}
		else {tmr_count++;}
	}
}
void TMR_Intr_Handler1(void *data)
{
	if (XTmrCtr_IsExpired(&TMRInst1,0)){
		//Once timer has expired 3 times, stop increment counter
		// reset timer and start running again
		if(tmr_count1 == 1){
			XTmrCtr_Stop(&TMRInst1,0);
			// Check if the new score is higher than any of the existing scores
			if (stop == 0){
				score += 1;
			}
			//xil_printf("Timer1 Interrupt \n\r");
			XTmrCtr_Reset(&TMRInst1,0);
			XTmrCtr_Start(&TMRInst1,0);
		}
		else{ tmr_count1++;}
	}
}

int main()
{
	int Status;
	int status;
	TMR_LOAD = 0xF8000000;

	//Initialize the UART
	    init_platform();
	    //--------------------------------
	    // SETUP SD
	    //---------------------------------
	    SD_Init();
	    //----------------------------------------------
		// SETUP GPIO_luz
		// ------------------------------------------------


	    Status = XGpio_Initialize(&gpio_luz, LUZ_DEVICE_ID);
		if (Status != XST_SUCCESS) {
			xil_printf("Gpio luz Initialization Failed\r\n");
		return XST_FAILURE;
		}

		xil_printf("Se inicializo correctamente el gpio luz!\n");
		XGpio_SetDataDirection(&gpio_luz,1,0xFF);

		 // Inicializando una instancia de la estructura con valores
		    struct MiEstructura TOP = {2000,1050,140,100,10};
	    //----------------------------------------------
		xil_printf("Ingrese NIVEL (1: FACIL, 2: MEDIO, 3:DIFICIL: \n\r");
		scanf("%d", &nivel);
		SET_DIFICULTY();
		// SETUP TIMER 0
		// ------------------------------------------------
		status = XTmrCtr_Initialize(&TMRInst, TMR_DEVICE_ID);
		if(status != XST_SUCCESS) return XST_FAILURE;
		XTmrCtr_SetHandler(&TMRInst, (XTmrCtr_Handler) TMR_Intr_Handler, &TMRInst);
		XTmrCtr_SetResetValue(&TMRInst, 0, TMR_LOAD);
		XTmrCtr_SetOptions(&TMRInst, 0, XTC_INT_MODE_OPTION | XTC_AUTO_RELOAD_OPTION);

		//----------------------------------------------
		// SETUP TIMER 1
		// ------------------------------------------------
		status = XTmrCtr_Initialize(&TMRInst1, TMR_DEVICE_ID_1);
		if(status != XST_SUCCESS) return XST_FAILURE;
		XTmrCtr_SetHandler(&TMRInst1, (XTmrCtr_Handler) TMR_Intr_Handler1, &TMRInst1);
		XTmrCtr_SetResetValue(&TMRInst1, 0, TMR_LOAD1);
		XTmrCtr_SetOptions(&TMRInst1, 0, XTC_INT_MODE_OPTION | XTC_AUTO_RELOAD_OPTION);

		//--------------------------------------------------
		// Initialize interrupt controller
		//----------------------------------------------------------------
		status = IntcInitFunction(INTC_DEVICE_ID, &gpio_luz, &TMRInst, &TMRInst1);
		if(status != XST_SUCCESS) return XST_FAILURE;

		XTmrCtr_Start(&TMRInst, 0);
		XTmrCtr_Start(&TMRInst1, 0);

	/* Initialize the GPIO 0 driver */
	Status = XGpio_Initialize(&gpio0, XPAR_AXI_GPIO_0_DEVICE_ID);
	if (Status != XST_SUCCESS) {
		xil_printf("Gpio 0 Initialization Failed\r\n");
		return XST_FAILURE;
	}

	// Set up the AXI SPI Controller 0 (Screen)
	Status = XSpi_Init(&SpiInstance,SPI_DEVICE_ID);
	if (Status != XST_SUCCESS) {
		xil_printf("SPI Mode Failed\r\n");
		return XST_FAILURE;
	}
	// Set up the AXI SPI Controller 0 (Joystick(x,y), accelerometer, potentiometer, mic)
	Status = init_adc(&SpiInstance1, SPI_DEVICE_ID_1);
	if (Status != XST_SUCCESS) {
		xil_printf("SPI-ADC Mode Failed\r\n");
		return XST_FAILURE;
	}
	// Set up the AXI IIC Controller 0 (temperature sensor, light sensor)
	Status = init_IIC();
	if (Status != XST_SUCCESS) {
		xil_printf("IIC Mode Failed\r\n");
		return XST_FAILURE;
	}
	//Write through UART to PC
	xil_printf("TFT initialized \r\n");
	xil_printf("****Init LCD****\r\n");
	// Init screen
	LCD_SCAN_DIR LCD_ScanDir = SCAN_DIR_DFT;//SCAN_DIR_DFT = D2U_L2R
	LCD_Init(LCD_ScanDir );
	// Default intro image from screen company
	xil_printf("LCD Show \r\n");

	GUI_Show();
	delay_ms(500);
	LCD_Clear(GUI_BACKGROUND);
	// Default intro image given by SEP course.
	GUI_INTRO();
	delay_ms(500);
	LCD_Clear(GUI_BACKGROUND);

	int inicio = 0;
	char puntos[16] = {};
	fptr = openFile("logData.csv", 'a');
	if(fptr == 0){
	printf("File opening failed \n\r");}
	xil_printf("Ingrese 1 para Pausar la música o 0 para continuar: \n\r");
	scanf("%d", &p);
	int cambio = 0;
	int tintineo = 0;
	while(1){
		MUSICA();
		if (estado == 0){
			if (una_vez == 0){
				MENU();
				una_vez = 1;
			}

			if (tintineo <= 15) {
				tintineo += 1;
			}
			else {
				tintineo = 0;
				if (cambio == 0){
					cambio = 1;
					GUI_DisString_EN(15,92,"Spin to start",&Font12,GUI_BACKGROUND,WHITE);
				}
				else {
					cambio = 0;
					GUI_DisString_EN(15,92,"Spin to start",&Font12,GUI_BACKGROUND,GUI_BACKGROUND);
				}
			}

			int luz = read_opt();
			//xil_printf("LUZ: %d\n", luz);
			//int temp = read_tmp();
			//xil_printf("LUZ: %d\n", temp);
			int acy = read_acy();
			if (acy >= y_max){		// Cambio de estado
				delay_ms(2000);
				estado = 1;
			}
		}
		//-------------------------------------------------------------------
		//---------------------------- JUEGO---------------------------------

		else if (estado == 1){ // JUEGOS
			una_vez = 0;
			if (inicio == 0) {
				score = 0;
				LCD_Clear(GUI_BACKGROUND);
				GUI_DrawLine(0,22,127,22, WHITE,0, 1);
				GUI_DisString_EN(5,10,"Score:",&Font12,GUI_BACKGROUND,CYAN);
				inicio = 1;
			}
			JUEGO();
			CHECK_STATE();// VER CAMBIO DE ESTADO y temperatura
			GUI_DisString_EN(95,10,puntos, &Font12,GUI_BACKGROUND,GUI_BACKGROUND);
			sprintf(puntos, "%d", score);
			GUI_DisString_EN(95,10,puntos, &Font12,GUI_BACKGROUND,YELLOW);
		}
		else if (estado == 2) { // Game Over
				inicio = 0;
				GAMEOVER();
			}
		else if (estado == 3) { // PUNTAJES
	 //  ------------------- PRUEBA TARJETA SD ---------------------------------------------
				if (logNum < MAX_LOG_NUM){
					int TempData = TOP5[logNum];
					printf("Puntaje: %d\n", TempData);
					sprintf(dataPntr, "%d\n",TempData);
					xil_printf("Updating SD card... \n\r");
					writeFile(fptr, 80, (u32)dataBuffer);
					xil_printf("Writing value number: %d\n", logNum);
					dataPntr = (char *)dataBuffer;
				}
				else if (logNum == MAX_LOG_NUM) {
					closeFile(fptr);
					SD_Eject();
					xil_printf("Safe to remove SD Card... \n\r");
				}
				logNum ++;
			if (print_puntos == 0) {
				PUNTAJES();
			}
			int acy = read_acy();
				if (acy >= y_max){		// Cambio de estado
					delay_ms(1000);
					estado = 0;
				}
		}
		else { // CALIENTE
			CALIENTE();
		}


//GUI_DisString_EN(95,10,punto, &Font12,GUI_BACKGROUND,GUI_BACKGROUND);
//GUI_DisString_EN(95,10,punto, &Font12,GUI_BACKGROUND,YELLOW);
	//We send through UART the data for each reading. This allows us to see what is happening inside the uP.

	delay_ms(20);
	}
    return 0;
}
/****** Function Definition *******/

/*********************/
void MENU() {
	LCD_Clear(GUI_BACKGROUND);
	PRINT_SNAKE();
	GUI_DisString_EN(45, 72, "SNAKE", &Font12, GUI_BACKGROUND, WHITE);
}

/*********************/
void JUEGO() {
	if (existencia == 0){
		generarManzana();
		existencia = 1;
	}
	for(int i = 0; i < largo; i++) {
		GUI_DrawPoint(posx[i], posy[i], BLACK,2,1);
	}
	int jx = read_joyx();
	int jy = read_joyy();

	if (jx > 1000) {
		movx = 2;
		movy = 0;
	}
	else if (jx < 10) {
		movx = -2;
		movy = 0;
	}
	else if (jy > 1000) {
		movx = 0;
		movy = -2;
	}
	else if (jy < 10) {
		movx = 0;
		movy = 2;
	}
	if (se_mueve == 1){
		se_mueve = 0;
		for(int i = largo - 1; i >= 0; --i) {
				if (i==0) {
					posx[0] = posx[0] + movx;
					posy[0] = posy[0] + movy;
				}
				else {
					posx[i] = posx[i - 1];
					posy[i] = posy[i - 1];
				}
			}
	}
	if (posx[0] == Mx && posy[0] == My) {
		posx[largo] = posx[0];
		posy[largo] = posy[0];
		largo++;
		existencia = 0;
		score += 5;
	}
	for(int i = 0; i < largo; i++) {
		GUI_DrawPoint(posx[i], posy[i], GREEN,2,1);
	}

}

/*********************/
void generarManzana() {
	// Generar posiciones "aleatorias" para la manzana
	int manzanaEnCuerpo = 1;
	while (manzanaEnCuerpo){
		manzanaEnCuerpo = 0;
		Mx = 2*(rand() %63) + 2;
		My = 2*(rand() %56) + 26;
		for (int i = 0; i < largo; ++i) {
			if (Mx == posx[i] && My == posy[i]) {
				manzanaEnCuerpo = 1;  // Verdadero, la manzana está en el cuerpo
				break;
			}
		}
	}
	xil_printf("X%d\n",Mx);
	xil_printf("Y:%d\n",My);

	xil_printf("\n");

	// Dibujar la manzana en la pantalla
	GUI_DrawPoint(Mx, My, RED, 2, 1);

}
/*********************/
void PUNTAJES() {
	sprintf(top1, "%d", TOP5[0]);
	sprintf(top2, "%d", TOP5[1]);
	sprintf(top3, "%d", TOP5[2]);
	sprintf(top4, "%d", TOP5[3]);
	sprintf(top5, "%d", TOP5[4]);
	LCD_Clear(GUI_BACKGROUND);
	GUI_DisString_EN(30,10,"PUNTAJES",&Font12,GUI_BACKGROUND,CYAN);
	GUI_DisString_EN(30,30,top1,&Font12,GUI_BACKGROUND,CYAN);
	GUI_DisString_EN(30,50,top2,&Font12,GUI_BACKGROUND,CYAN);
	GUI_DisString_EN(30,70,top3,&Font12,GUI_BACKGROUND,CYAN);
	GUI_DisString_EN(30,90,top4,&Font12,GUI_BACKGROUND,CYAN);
	GUI_DisString_EN(30,110,top5,&Font12,GUI_BACKGROUND,CYAN);
	print_puntos = 1;

}

/*********************/
void CHECK_STATE(){
	if (posx[0] <= 0) {
		estado = 2;
	}
	else if (posx[0] >= 127) {
		estado = 2;
	}
	if (posy[0] <= 23) {
		estado = 2;
	}
	else if (posy[0] >= 127) {
		estado = 2;
	}

	int temp = read_tmp(); // REVISAR TEMPERATURA
	if (temp > 102){
		CALIENTE();
	}
}

/*********************/
void GAMEOVER(){
	for (int i = 0; i < 5; i++){
		if (score > TOP5[i]){
			for (int j = 4; j > i; j--) {
				TOP5[j] = TOP5[j - 1];
			}

			// Inserta el nuevo valor en la posición correcta
			TOP5[i] = score;
			break;
		}
	}
	for(int i = 0; i < largo; i++) {
		if (i == 0) {
			posx[i] = 2;
			posy[i] = 24;
			movx = 3;
			movy = 0;
			largo = 1;
			score = 0;
			existencia = 0;
			print_puntos = 0;
		}
		else {
			posx[i] = 0;
			posy[i] = 0;
		}
	}
	GUI_DisString_EN(28,60,"GAME OVER",&Font12,GUI_BACKGROUND,RED);
	delay_ms(3000);
	estado = 3;
}

/*********************/
void CALIENTE(){

	LCD_Clear(GUI_BACKGROUND);
	GUI_DisString_EN(30,10,"CALIENTE",&Font12,GUI_BACKGROUND,CYAN);
	una_vez = 0;
	estado = 0;
	delay_ms(200);
}

/*********************/
void MUSICA(){
	int pot1 = read_POT1(); //APAGAR O PRENDER MUSICA CON POTENCIOMETRO
	//xil_printf("POT1: %d\n",pot1);
	if (pot1 > 900){
		p = 1;
	}

	else{p=0;}

	Xil_Out32(PAUSA, p);
}

void SET_DIFICULTY(){
	if (nivel == 1){
		TMR_LOAD = 0xF8000000; //4160749568
		xil_printf("Dificultad seteada a: FACIL \n\r");
	}
	else if (nivel == 2){
		TMR_LOAD = 0xFCCD3F7F;
		xil_printf("Dificultad seteada a: MEDIO \n\r");
	}
	else{
		TMR_LOAD = 0xFFC82FFF; // 4291309567
		xil_printf("Dificultad seteada a: DIFICIL \n\r");
		}
}

void PRINT_SNAKE(){
	GUI_DrawRectangle(32, 9, 94, 70, BROWN, 1 , 1 );
	//1
	GUI_DrawRectangle(48, 10, 77, 12, BLACK, 1 , 1 );
	//2
	GUI_DrawRectangle(43, 13, 47, 15, BLACK, 1 , 1 );
	GUI_DrawRectangle(48, 13, 77, 15, GREEN, 1 , 1 );
	GUI_DrawRectangle(78, 13, 84, 15, BLACK, 1 , 1 );

	//3
	GUI_DrawRectangle(39, 16, 41, 18, BLACK, 1 , 1 );
	GUI_DrawRectangle(42, 16, 54, 18, GREEN, 1 , 1 );
	GUI_DrawRectangle(55, 16, 57, 18, BLACK, 1 , 1 );
	GUI_DrawRectangle(58, 16, 60, 18, RED, 1 , 1 );
	GUI_DrawRectangle(61, 16, 65, 18, GREEN, 1 , 1 );
	GUI_DrawRectangle(66, 16, 68, 18, RED, 1 , 1 );
	GUI_DrawRectangle(69, 16, 71, 18, BLACK, 1 , 1 );
	GUI_DrawRectangle(72, 16, 84, 18, GREEN, 1 , 1 );
	GUI_DrawRectangle(85, 16, 87, 18, BLACK, 1 , 1 );

	//4
	GUI_DrawRectangle(39, 19, 41, 21, BLACK, 1 , 1 );
	GUI_DrawRectangle(42, 19, 44, 21, GREEN, 1 , 1 );
	GUI_DrawRectangle(45, 19, 57, 21, BLACK, 1 , 1 );
	GUI_DrawRectangle(58, 19, 68, 21, GREEN, 1 , 1 );
	GUI_DrawRectangle(69, 19, 81, 21, BLACK, 1 , 1 );
	GUI_DrawRectangle(82, 19, 84, 21, GREEN, 1 , 1 );
	GUI_DrawRectangle(85, 19, 87, 21, BLACK, 1 , 1 );

	//5
	GUI_DrawRectangle(36, 22, 38, 24, BLACK, 1, 1);
	GUI_DrawRectangle(39, 22, 44, 24, GREEN, 1, 1);
	GUI_DrawRectangle(45, 22, 47, 24, BLACK, 1, 1);
	GUI_DrawRectangle(48, 22, 57, 24, YELLOW, 1, 1);
	GUI_DrawRectangle(58, 22, 61, 24, BLACK, 1, 1);
	GUI_DrawRectangle(62, 22, 65, 24, GREEN, 1, 1);
	GUI_DrawRectangle(66, 22, 68, 24, BLACK, 1, 1);
	GUI_DrawRectangle(69, 22, 77, 24, YELLOW, 1, 1);
	GUI_DrawRectangle(78, 22, 81, 24, BLACK, 1, 1);
	GUI_DrawRectangle(82, 22, 87, 24, GREEN, 1, 1);
	GUI_DrawRectangle(88, 22, 90, 24, BLACK, 1, 1);

	//6
	GUI_DrawRectangle(36, 25, 38, 27, BLACK, 1, 1);
	GUI_DrawRectangle(39, 25, 44, 27, GREEN, 1, 1);
	GUI_DrawRectangle(45, 25, 60, 27, BROWN, 1, 1);
	GUI_DrawRectangle(61, 25, 65, 27, BLACK, 1, 1);
	GUI_DrawRectangle(66, 25, 81, 27, BROWN, 1, 1);
	GUI_DrawRectangle(82, 25, 87, 27, GREEN, 1, 1);
	GUI_DrawRectangle(88, 25, 90, 27, BLACK, 1, 1);

	//7
	GUI_DrawRectangle(36, 28, 38, 30, BLACK, 1, 1);
	GUI_DrawRectangle(39, 28, 48, 30, GREEN, 1, 1);
	GUI_DrawRectangle(48, 28, 77, 30, YELLOW, 1, 1);
	GUI_DrawRectangle(78, 28, 87, 30, GREEN, 1, 1);
	GUI_DrawRectangle(88, 28, 90, 30, BLACK, 1, 1);

	//8
	GUI_DrawRectangle(39, 31, 41, 33, BLACK, 1, 1);
	GUI_DrawRectangle(42, 31, 51, 33, GREEN, 1, 1);
	GUI_DrawRectangle(52, 31, 74, 33, BROWN, 1, 1);
	GUI_DrawRectangle(75, 31, 84, 33, GREEN, 1, 1);
	GUI_DrawRectangle(85, 31, 87, 33, BLACK, 1, 1);

	//9
	GUI_DrawRectangle(42, 34, 48, 36, BLACK, 1, 1);
	GUI_DrawRectangle(49, 34, 54, 36, GREEN, 1, 1);
	GUI_DrawRectangle(55, 34, 71, 36, YELLOW, 1, 1);
	GUI_DrawRectangle(72, 34, 77, 36, GREEN, 1, 1);
	GUI_DrawRectangle(78, 34, 84, 36, BLACK, 1, 1);

	//10
	GUI_DrawRectangle(49, 37, 54, 39, BLACK, 1, 1);
	GUI_DrawRectangle(55, 37, 57, 39, GREEN, 1, 1);
	GUI_DrawRectangle(58, 37, 68, 39, BROWN, 1, 1);
	GUI_DrawRectangle(69, 37, 71, 39, GREEN, 1, 1);
	GUI_DrawRectangle(72, 37, 77, 39, BLACK, 1, 1);

	//11
	GUI_DrawRectangle(55, 40, 57, 42, BLACK, 1, 1);
	GUI_DrawRectangle(58, 40, 60, 42, GREEN, 1, 1);
	GUI_DrawRectangle(61, 40, 65, 42, YELLOW, 1, 1);
	GUI_DrawRectangle(66, 40, 68, 42, GREEN, 1, 1);
	GUI_DrawRectangle(69, 40, 71, 42, BLACK, 1, 1);

	//12
	GUI_DrawRectangle(55, 43, 57, 45, BLACK, 1, 1);
	GUI_DrawRectangle(58, 43, 60, 45, GREEN, 1, 1);
	GUI_DrawRectangle(61, 43, 65, 45, BROWN, 1, 1);
	GUI_DrawRectangle(66, 43, 68, 45, GREEN, 1, 1);
	GUI_DrawRectangle(69, 43, 71, 45, BLACK, 1, 1);
	GUI_DrawRectangle(87, 43, 90, 45, BLACK, 1, 1);

	//13
	GUI_DrawRectangle(58, 46, 60, 48, BLACK, 1, 1);
	GUI_DrawRectangle(61, 46, 65, 48, YELLOW, 1, 1);
	GUI_DrawRectangle(66, 46, 68, 48, BLACK, 1, 1);
	GUI_DrawRectangle(84, 46, 86, 48, BLACK, 1, 1);
	GUI_DrawRectangle(87, 46, 90, 48, BROWN, 1, 1);
	GUI_DrawRectangle(91, 46, 93, 48, BLACK, 1, 1);

	//14
	GUI_DrawRectangle(58, 49, 60, 51, BLACK, 1, 1);
	GUI_DrawRectangle(61, 49, 65, 51, BROWN, 1, 1);
	GUI_DrawRectangle(66, 49, 68, 51, BLACK, 1, 1);
	GUI_DrawRectangle(84, 49, 86, 51, BLACK, 1, 1);
	GUI_DrawRectangle(87, 49, 90, 51, YELLOW, 1, 1);
	GUI_DrawRectangle(91, 49, 93, 51, BLACK, 1, 1);

	//15
	GUI_DrawRectangle(55, 52, 57, 54, BLACK, 1, 1);
	GUI_DrawRectangle(58, 52, 62, 54, YELLOW, 1, 1);
	GUI_DrawRectangle(63, 52, 65, 54, BLACK, 1, 1);
	GUI_DrawRectangle(84, 52, 86, 54, BLACK, 1, 1);
	GUI_DrawRectangle(87, 52, 90, 54, GREEN, 1, 1);
	GUI_DrawRectangle(91, 52, 93, 54, BLACK, 1, 1);

	//16
	GUI_DrawRectangle(39, 55, 84, 57, BLACK, 1, 1);
	GUI_DrawRectangle(85, 55, 90, 57, GREEN, 1, 1);
	GUI_DrawRectangle(91, 55, 93, 57, BLACK, 1, 1);

	//17
	GUI_DrawRectangle(36, 58, 38, 60, BLACK, 1, 1);
	GUI_DrawRectangle(39, 58, 81, 60, GREEN, 1, 1);
	GUI_DrawRectangle(82, 58, 84, 60, BLACK, 1, 1);
	GUI_DrawRectangle(85, 58, 87, 60, GREEN, 1, 1);
	GUI_DrawRectangle(88, 58, 90, 60, BLACK, 1, 1);

	//18
	GUI_DrawRectangle(33, 61, 35, 66, BLACK, 1, 1);
	GUI_DrawRectangle(36, 61, 84, 66, GREEN, 1, 1);
	GUI_DrawRectangle(85, 61, 87, 66, BLACK, 1, 1);

	//19
	GUI_DrawRectangle(36, 67, 84, 69, BLACK, 1, 1);

}
/*********************/
// INTERRUPT HANDLER FUNCTION
// ---------------------------------------------
// INITIAL SETUP FUNCTIONS
// -------------------------------------------------

int InterruptSystemSetup(XScuGic *XScuGicInstancePtr)
{
	// Enable Interrupt
	XGpio_InterruptEnable(&gpio_luz, LUZ_INT);
	XGpio_InterruptGlobalEnable(&gpio_luz);

	Xil_ExceptionRegisterHandler(XIL_EXCEPTION_ID_INT,
	(Xil_ExceptionHandler)XScuGic_InterruptHandler,
	XScuGicInstancePtr);

	Xil_ExceptionEnable();

	return XST_SUCCESS;

}

int IntcInitFunction(u16 DeviceId,XGpio *GpioInstancePtr, XTmrCtr *TmrInstancePtr, XTmrCtr *TmrInstancePtr1)
{
	XScuGic_Config *IntcConfig;
	int status;

	// Interrupt controller initialization
	IntcConfig = XScuGic_LookupConfig(DeviceId);
	status = XScuGic_CfgInitialize(&INTCInst, IntcConfig, IntcConfig->CpuBaseAddress);
	if(status != XST_SUCCESS) return XST_FAILURE;

	// Call to interrupt Controller setup
	status = InterruptSystemSetup(&INTCInst);
	if(status != XST_SUCCESS) return XST_FAILURE;

	// ASIGNACION DE PRIORIDADES

	XScuGic_SetPriorityTriggerType(&INTCInst, INTC_LUZ_INTERRUPT_ID, 0x18, 0x1);
	XScuGic_SetPriorityTriggerType(&INTCInst, INTC_TMR_INTERRUPT_ID, 0x20, 0x1);
	XScuGic_SetPriorityTriggerType(&INTCInst, INTC_TMR_INTERRUPT_ID_1, 0x28, 0x1);


	// Connect timer 0 interrupt to handler
	status = XScuGic_Connect(&INTCInst,INTC_TMR_INTERRUPT_ID,(Xil_ExceptionHandler)TMR_Intr_Handler,(void *) TmrInstancePtr);
	if(status != XST_SUCCESS) return XST_FAILURE;

	// Connect timer 1 interrupt to handler
	status = XScuGic_Connect(&INTCInst,INTC_TMR_INTERRUPT_ID_1,(Xil_ExceptionHandler)TMR_Intr_Handler1,(void *) TmrInstancePtr1);
	if(status != XST_SUCCESS) return XST_FAILURE;

	// Connect interrupt light
	status = XScuGic_Connect(&INTCInst,INTC_LUZ_INTERRUPT_ID,(Xil_ExceptionHandler)LUZ_Intr_Handler1,(void *)GpioInstancePtr);
	if(status != XST_SUCCESS) return XST_FAILURE;

	// Enable GPIO interrupts
	XGpio_InterruptEnable(GpioInstancePtr,1);
	XGpio_InterruptGlobalEnable(GpioInstancePtr);

	// Enable timer interrupts in the controller

	XScuGic_Enable(&INTCInst, INTC_TMR_INTERRUPT_ID);
	XScuGic_Enable(&INTCInst, INTC_TMR_INTERRUPT_ID_1);
	XScuGic_Enable(&INTCInst, INTC_LUZ_INTERRUPT_ID);

	return XST_SUCCESS;
}


