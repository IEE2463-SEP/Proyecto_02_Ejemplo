#include "I2C.h"
XIic  iic;
u8 SendBuffer[2];
u8 SendBuffer1[2];
u8 SendBuffer_low[2];
u8 SendBuffer_high[2];
u8 RecvBuffer[2];
u8 config[3];
u8 config1[3];
u8 config2[3];
u8 config3[3];
int Lux;
int16_t val;
int temp;
u8 writeBuffer[3];

// OPT3001 Register Addresses
#define OPT3001_REG_INTERRUPT 0x01
#define OPT3001_REG_THRESHOLD_HIGH 0x03
#define OPT3001_REG_THRESHOLD_LOW 0x02

int init_IIC() {
		XIic_Config *iic_conf;

	    init_platform();

	    iic_conf = XIic_LookupConfig(IIC_dev);
	    XIic_CfgInitialize(&iic, iic_conf, iic_conf->BaseAddress);
	    XIic_Start(&iic);

		SendBuffer[0] = 0xfe;
		XIic_Send(iic.BaseAddress,TMP_ADDR,(u8 *)&SendBuffer, 1,XIIC_REPEATED_START);
		XIic_Recv(iic.BaseAddress,TMP_ADDR,(u8 *)&RecvBuffer, 2,XIIC_STOP);


		SendBuffer[0] = 0x02;
		XIic_Send(iic.BaseAddress,TMP_ADDR,(u8 *)&SendBuffer, 1,XIIC_STOP);

		SendBuffer[0] = 0x80;
		XIic_Send(iic.BaseAddress,TMP_ADDR,(u8 *)&SendBuffer, 1,XIIC_REPEATED_START);
		SendBuffer[0] = 0x82;
		XIic_Send(iic.BaseAddress,TMP_ADDR,(u8 *)&SendBuffer, 1,XIIC_REPEATED_START);

    return XST_SUCCESS;
}

int read_tmp(){
	SendBuffer[0] = 0x01; // envia para leer temp
	XIic_Send(iic.BaseAddress, TMP_ADDR, (u8 *)&SendBuffer, 1, XIIC_REPEATED_START);
	XIic_Recv(iic.BaseAddress, TMP_ADDR, (u8 *)&RecvBuffer, 2, XIIC_STOP);
	val = (RecvBuffer[0] << 8) | (RecvBuffer[1]);
	temp = val / 128;
	return temp;
}
int read_opt(){
	u8 config3[3] = {0x01, 0xc4, 0x18};//{0x01, 0xc4, 0x10}
	XIic_Send(iic.BaseAddress,OPT_ADDR,(u8 *)&config3, 3, XIIC_STOP);

	// LIMITE SUPERIOR: LUX = 18586
	u8 config[3] = {0x03, 0x48, 0x9A};
	XIic_Send(iic.BaseAddress, OPT_ADDR, (u8 *)&config, 3, XIIC_STOP);

	// LIMITE INFERIOR: LUX = 6810
	u8 config1[3] = {0x02, 0x1A, 0x9A};
	XIic_Send(iic.BaseAddress, OPT_ADDR, (u8 *)&config1, 3, XIIC_STOP);

	SendBuffer1[0] = 0x01;
	XIic_Send(iic.BaseAddress,OPT_ADDR,(u8 *)&SendBuffer1, 1, XIIC_REPEATED_START);
	XIic_Recv(iic.BaseAddress,OPT_ADDR,(u8 *)&RecvBuffer, 2, XIIC_STOP);
	//xil_printf("Lectura OPT RECVBUFFER CONFIG 0: %x\n\r", RecvBuffer[0]);
	//xil_printf("Lectura OPT RECVBUFFER  CONFIG 1: %x\n\r", RecvBuffer[1]);

	SendBuffer_low[0] = 0x02;
	XIic_Send(iic.BaseAddress,OPT_ADDR,(u8 *)&SendBuffer_low, 1, XIIC_REPEATED_START);
	XIic_Recv(iic.BaseAddress,OPT_ADDR,(u8 *)&RecvBuffer, 2, XIIC_STOP);
	//xil_printf("Lectura OPT RECVBUFFER LOW LIMIT 0: %x\n\r", RecvBuffer[0]);
	//xil_printf("Lectura OPT RECVBUFFER  LOW LIMIT 1: %x\n\r", RecvBuffer[1]);


	SendBuffer_high[0] = 0x03;
	XIic_Send(iic.BaseAddress,OPT_ADDR,(u8 *)&SendBuffer_high, 1, XIIC_REPEATED_START);
	XIic_Recv(iic.BaseAddress,OPT_ADDR,(u8 *)&RecvBuffer, 2, XIIC_STOP);
	//xil_printf("Lectura OPT RECVBUFFER HIGH LIMIT 0: %x\n\r", RecvBuffer[0]);
	//xil_printf("Lectura OPT RECVBUFFER  HIGH LIMIT 1: %x\n\r", RecvBuffer[1]);


	//RecvBuffer[0] = 0x01;
	//XIic_Recv(iic.BaseAddress,OPT_ADDR,(u8 *)&RecvBuffer, 2, XIIC_STOP);

	SendBuffer[0] = 0x00;
	XIic_Send(iic.BaseAddress,OPT_ADDR,(u8 *)&SendBuffer, 1, XIIC_REPEATED_START);
	XIic_Recv(iic.BaseAddress,OPT_ADDR,(u8 *)&RecvBuffer, 2, XIIC_STOP);
	Lux = (int)((RecvBuffer[0])*256 + (RecvBuffer[1]));
	//xil_printf("Lectura OPT LUX 0: %x\n\r", RecvBuffer[0]);
	//xil_printf("Lectura OPT LUX 1: %x\n\r", RecvBuffer[1]);

	return Lux;
}
