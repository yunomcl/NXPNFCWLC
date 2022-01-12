#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>

#if 1
#define PRINT_BUF(x,y,z)  {int loop; printf(x); \
                           for(loop=0;loop<z;loop++) printf("%.2x ", y[loop]); \
                           printf("\n");}
#else
#define PRINT_BUF(x,y,z)	
#endif

	
static int openNfc(int * handle)
{
    *handle = open((char const *)"/dev/CTN730", O_RDWR);
    if (*handle < 0) return -1;
	return 0;
}

static void closeNfc(int handle)
{
    close(handle);
}

static void reset(int handle)
{
    ioctl(handle, _IOW(0xE9, 0x01, unsigned int), 0);
    usleep(10 * 1000);
    ioctl(handle, _IOW(0xE9, 0x01, unsigned int), 1);
}

static int send(int handle, char *pBuff, int buffLen)
{
	int ret = write(handle, pBuff, buffLen);
	if(ret <= 0) {
		/* retry to handle standby mode */
		ret = write(handle, pBuff, buffLen);
		if(ret <= 0) return 0;
	}
	PRINT_BUF(">> ", pBuff, ret);
	return ret;
}

static int receive(int handle, char *pBuff, int buffLen)
{
    int numRead;
    int ret;

    ret = read(handle, pBuff, 2);
    if (ret <= 0) return 0;
    numRead = 2;
	if(pBuff[1] + 2 > buffLen) return 0;

    ret = read(handle, &pBuff[2], pBuff[1]);
    if (ret <= 0) return 0;
    numRead += ret;

	PRINT_BUF("<< ", pBuff, numRead);

	return numRead;
}

static int transceive(int handle, char *pTx, int TxLen, char *pRx, int RxLen)
{
	if(send(handle, pTx, TxLen) == 0) return 0;
	return receive(handle, pRx, RxLen);
}

static void RfOn (int handle)
{
    char BISTRfOn[] = {0x06, 0x01, 0x01};
    char Answer[256];
    int NbBytes = 0;
	NbBytes = transceive(handle, BISTRfOn, sizeof(BISTRfOn), Answer, sizeof(Answer));
	
	printf("RF ON mode - Press enter to stop\n");
}

static void RfOff (int handle)
{
    char BISTRfOff[] = {0x06, 0x01, 0x02};
    char Answer[256];
    int NbBytes = 0;
	NbBytes = transceive(handle, BISTRfOff, sizeof(BISTRfOff), Answer, sizeof(Answer));
	
	printf("RF Off mode - Press enter to stop\n");
}

static void Activate (int handle)
{
    char BISTActivateTag[] = {0x06, 0x01, 0x04};
    char Answer[256];
    int NbBytes = 0;
	NbBytes = transceive(handle, BISTActivateTag, sizeof(BISTActivateTag), Answer, sizeof(Answer));
	if((NbBytes == 15) && (Answer[1] == 0x0D))
    {	
		if(Answer[2]== 0x00)
			printf("Tag activated and WLCCAP is present\n");
		
		else
			printf("Wrong WLC CAP\n");
    }
	else
		printf("Tag activation failed\n");
	
}

static void EnableCharging (int handle)
{
//	int DetectionInterval = 
//	printf("Detection Interval: ");
//	scanf("%x", &DetectionInterval);
	
    char EnableChargingCMD[] = {0x10, 0x02, 0x00, 0x02};
    char Answer[256];
    int NbBytes = 0;
	int PayloadLen =0;
	int PayloadCounter = 0;
	NbBytes = transceive(handle, EnableChargingCMD, sizeof(EnableChargingCMD), Answer, sizeof(Answer));
	if((NbBytes == 3) && (Answer[2] == 0x00))
    {	
		printf(" Response Status OK\n");
		while(1)
		{
		 NbBytes = receive(handle,  Answer, sizeof(Answer));
		 
		 if((NbBytes == 10) && (Answer[0] == 0x92) && (Answer[1] == 0x8) && (Answer[2] == 0x00))
			 printf("Device detected\n");
		 else if (Answer[0] == 0x92 && (Answer[2] == 0x1))
			 printf("Device deactivated\n");
		 else if (Answer[0] == 0x92 && (Answer[2] == 0x2))
			 printf("Device lost\n");
		 else if (Answer[0] == 0x94 && (Answer[2] == 0x0))
			 printf("Charging Started\n");
		 else if (Answer[0] == 0x94 && (Answer[2] == 0x1))
			 printf("Charging Ended\n");		 
		 else if (Answer[0] == 0x94 && (Answer[2] == 0x2))
			 printf("Charging Stopped\n");		 		 
		 else if (Answer[0] == 0x95)
			 printf("Battery percentage: %d\n",Answer[2]);
		 else if (Answer[0] == 0x9F){
				PayloadLen = Answer[1]; 
				PayloadCounter = sizeof(Answer)- PayloadLen;
				do{
				printf("Optional NDEF: %d\n",Answer[PayloadCounter]);		 
				PayloadCounter ++;
				}while(PayloadCounter<sizeof(Answer))
		}
    }
	else
		printf("Tag activation failed\n");
	
}

static void WriteParameter (int handle)
{
	unsigned int parameter = 0;
	int length=0;
	unsigned int value = 0;
	int exit=1;
	do {
	printf("\nEnter Parameter ID:");
	scanf("%x", &parameter);
	printf("\nEnter length of value:");
	scanf("%d", &length);
	printf("\nEnter value:");
	scanf("%x", &value);
	
	char WriteParameterCMD[] = {0x01, length+1 , parameter, value};
    char Answer[256];
    int NbBytes = 0;
	NbBytes = transceive(handle, WriteParameterCMD, sizeof(WriteParameterCMD), Answer, sizeof(Answer));
	
	printf("\n Enter 0 to exit:");
	scanf("%d", &exit);
	}while(exit);
}

static void ReadParameter (int handle)
{
	unsigned int parameter = 0;
	int exit=1;
	do {
	printf("\nEnter Parameter ID:");
	scanf("%x", &parameter);
	
	char ReadParameterCMD[] = {0x02, 0x01 , parameter};
    char Answer[256];
    int NbBytes = 0;
	NbBytes = transceive(handle, ReadParameterCMD, sizeof(ReadParameterCMD), Answer, sizeof(Answer));
	
	printf("\n Enter 0 to exit:");
	scanf("%d", &exit);
	}while(exit);

}

static void ReadCRN_EEPROM (int handle)
{
	unsigned int block = 0;
	unsigned int NumOfBlock = 0;
	int exit=1;
	do {
	printf("\nEnter starting block (min. 12):");
	scanf("%x", &block);
	printf("\nEnter Number of blocks to be read:");
	scanf("%x", &NumOfBlock);
	
	char ReadParameterCMD[] = {0x06,0x03,0x09,block,NumOfBlock};
    char Answer[256];
    int NbBytes = 0;
	NbBytes = transceive(handle, ReadParameterCMD, sizeof(ReadParameterCMD), Answer, sizeof(Answer));
	printf("\n Enter 0 to exit:");
	scanf("%d", &exit);
	}while(exit);
}

int main()
{
	int nHandle;
    char ResetNormalCMD[] = {0x00, 0x01, 0x00};
    char Answer[256];
    int NbBytes = 0;
	
	printf("\n----------------------------\n");
	printf("NFC WLC test application\n");
	printf("----------------------------\n");

	if(openNfc(&nHandle) != 0) {
		printf("Cannot connect to CTN730\n");
		return -1;
	}

	reset(nHandle);
	usleep(100*1000);
    NbBytes = receive(nHandle,  Answer, sizeof(Answer));
	if((NbBytes == 4) && (Answer[0] == 0x80) && (Answer[1] == 0x02) && (Answer[2] == 0x01))
    {
		printf("Reset Download\n");
    }
	else if((NbBytes == 5) && (Answer[0] == 0x80) && (Answer[1] == 0x03) && (Answer[2] == 0x00))
	{
		printf("Normal Reset\n");
	}
	
	NbBytes = transceive(nHandle, ResetNormalCMD, sizeof(ResetNormalCMD), Answer, sizeof(Answer));
    /* Catch potential notification */
    usleep(100*1000);
    NbBytes = receive(nHandle,  Answer, sizeof(Answer));
    if((NbBytes == 5) && (Answer[0] == 0x80) && (Answer[1] == 0x03) && (Answer[2] == 0x00))
    {
		 printf("Reset Normal\n");
        printf("CTN730 detected\n");
    }
    else if((NbBytes == 4) && (Answer[0] == 0x80) && (Answer[1] == 0x02) && (Answer[2] == 0x01))
    {
		 printf("Failed Normal Event\n");
    }
	do{
	printf("Select the test to run:\n");
	printf("\t 1. Continuous RF ON\n");
	printf("\t 2. Continuous RF Off\n");
	printf("\t 3. Activate Tag and verify WLCCAP\n");
	printf("\t 4. Enable Charging\n");
	printf("\t 5. Write Parameter\n");
	printf("\t 6. Read Parameter\n");
	printf("\t 7. Read CRN120 EEPROM\n");
	printf("\t 0. Quit application \n");
	printf("Your choice: ");
	scanf("%d", &NbBytes);
		
	switch(NbBytes)	{
		case 1:	RfOn(nHandle);	break;
		case 2:	RfOff(nHandle);	break;
		case 3:	Activate(nHandle);	break;
		case 4:	EnableCharging(nHandle);break;
		case 5:	WriteParameter(nHandle);break;
		case 6:	ReadParameter(nHandle);break;
		case 7:	ReadCRN_EEPROM(nHandle);break;
		case 0: break;
		default: printf("Wrong choice\n");	break;
	}
	}while(NbBytes != 0);
	
	fgets(Answer, sizeof(Answer), stdin);
	
	reset(nHandle);
	closeNfc(nHandle);
	
	return 0;
}