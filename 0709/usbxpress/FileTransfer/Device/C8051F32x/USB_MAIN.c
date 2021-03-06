//	File best viewed using Tab Size of 4 Characters
//	Author DM 	DATE	4-4-03
//	Modified CS DATE	8-25-03
//	This example illustrates usage of the USB_API.lib
//	DO NOT locate code segments at 0x1400 to 0x4000
//	These are used for non-voltile storage for transmitted data.

//	Include files
#include <c8051f320.h>		//	Header file for SiLabs c8051f320  
#include <stddef.h>			//	Used for NULL pointer definition
#include "USB_API.h"		//	Header file for USB_API.lib

//	Bit Definitions
sbit	Led1	=	P2^2;	//	LED='1' means ON
sbit	Led2	=	P2^3;	//	These blink to indicate data transmission

//	Constants Definitions
#define	NUM_STG_PAGES	20	//	Total number of flash pages to be used for file storage
#define MAX_BLOCK_SIZE_READ	64	//	Use the maximum read block size of 64 bytes

#define MAX_BLOCK_SIZE_WRITE 4096	//	Use the maximum write block size of 4096 bytes
#define	FLASH_PAGE_SIZE	512	        //	Size of each flash page
#define	BLOCKS_PR_PAGE	FLASH_PAGE_SIZE/MAX_BLOCK_SIZE_READ  // 512/64 = 8
#define MAX_NUM_BYTES	FLASH_PAGE_SIZE*NUM_STG_PAGES
#define MAX_NUM_BLOCKS	BLOCKS_PR_PAGE*NUM_STG_PAGES



//	Message Types
#define	READ_MSG	0x00	//	Message types for communication with host
#define	WRITE_MSG	0x01
#define	SIZE_MSG	0x02
#define DELAYED_READ_MSG	0x05

//	Machine States
#define	ST_WAIT_DEV	0x01	//	Wait for application to open a device instance
#define	ST_IDLE_DEV	0x02	//	Device is open, wait for Setup Message from host
#define	ST_RX_SETUP	0x04	//	Received Setup Message, decode and wait for data
#define	ST_RX_FILE	0x08	//	Receive file data from host
#define	ST_TX_FILE	0x10	//	Transmit file data to host
#define	ST_TX_ACK	0x20	//	Transmit ACK 0xFF back to host after every 8 packets
#define	ST_ERROR	0x80	//	Error state

// No such thing as a block of data anymore since it is variable between 1 and 1024
// So comment this out
typedef struct {			//	Structure definition of a block of data
	BYTE	Piece[MAX_BLOCK_SIZE_READ];
}	BLOCK;

typedef struct {			//	Structure definition of a flash memory page
	BYTE	FlashPage[FLASH_PAGE_SIZE];
}	PAGE;

xdata	BLOCK	TempStorage[BLOCKS_PR_PAGE];	//	Temporary storage of between flash writes

//data	BYTE code *	PageIndices[20]	=	{0x1400,0x1600,0x1800,0x1A00,0x1C00,0x1E00,0x2000,0x2200,0x2400,0x2600
//									,0x2800,0x2A00,0x2C00,0x2E00,0x3000,0x3200,0x3400,0x3600,0x3800,0x3A00};
data	BYTE code *	PageIndices[20]	=	{0x2200,0x2400,0x2600,0x2800,0x2A00,0x2C00,0x2E00,0x3000,0x3200,0x3400
									,0x3600,0x3800,0x3A00,0x3C00,0x3E00,0x4000,0x4200,0x4400,0x4600};

data	UINT	BytesToRead;	//	Total number of bytes to read from host
data	UINT	WriteStageLength;	//	Current write transfer stage length
data	UINT	ReadStageLength; //  Current read transfer stage length
data	BYTE	Buffer[3];		//	Buffer for Setup messages
data	UINT	NumBytes;		//	Number of Blocks for this transfer
data	BYTE	NumBlocks;
data	UINT	BytesRead;		//	Number of Bytes Read
data	BYTE	M_State;		//	Current Machine State
data	UINT	BytesWrote;		//	Number of Bytes Written
data	BYTE	BlockIndex;		//	Index of Current Block in Page
data	BYTE	PageIndex;		//	Index of Current Page in File
data	BYTE	BlocksWrote;	//	Total Number of Blocks Written
data	BYTE*	ReadIndex;
data	UINT	BytesToWrite;

/*** [BEGIN] USB Descriptor Information [BEGIN] ***/
code const UINT USB_VID = 0x10C4;
code const UINT USB_PID = 0xEA61;
code const BYTE USB_MfrStr[] = {0x1A,0x03,'S',0,'i',0,'l',0,'i',0,'c',0,'o',0,'n',0,' ',0,'L',0,'a',0,'b',0,'s',0};	                     //	Manufacturer String
code const BYTE USB_ProductStr[] = {0x10,0x03,'U',0,'S',0,'B',0,' ',0,'A',0,'P',0,'I',0};	//	Product Desc. String
code const BYTE USB_SerialStr[] = {0x0A,0x03,'1',0,'2',0,'3',0,'4',0};
code const BYTE USB_MaxPower = 15;            // Max current = 30 mA (15 * 2)
code const BYTE USB_PwAttributes = 0x80;      // Bus-powered, remote wakeup not supported
code const UINT USB_bcdDevice = 0x0100;       // Device release number 1.00
/*** [ END ] USB Descriptor Information [ END ] ***/

code    BYTE  	LengthFile[3]	_at_	0x2000;
	//	{Length(Low Byte), Length(High Byte), Number of Blocks}

void	Port_Init(void);			//	Initialize Ports Pins and Enable Crossbar
void	State_Machine(void);		//	Determine new state and act on current state
void	Receive_Setup(void);		//	Receive and decode setup packet from host
void	Receive_File(void);			//	Receive file data from host
//void	Suspend_Device(void);		//  Place the device in suspend mode
void	Page_Erase(BYTE*)	small;	//	Erase a flash page
void	Page_Write(BYTE*)	small;	//	Write a flash page



//-----------------------------------------------------------------------------
// Main Routine
//-----------------------------------------------------------------------------
void main(void) 
{
	PCA0MD &= ~0x40;					//	Disable Watchdog timer

   USB_Clock_Start();                     // Init USB clock *before* calling USB_Init
   USB_Init(USB_VID,USB_PID,USB_MfrStr,USB_ProductStr,USB_SerialStr,USB_MaxPower,USB_PwAttributes,USB_bcdDevice);

	CLKSEL |= 0x02;

	
	RSTSRC	|=	0x02;

	Port_Init();						//	Initialize crossbar and GPIO

	USB_Int_Enable();					//	Enable USB_API Interrupts
		while (1);
}

void	Port_Init(void)
{  
   P2MDOUT	|=	0x0C;					//	Port 2 pins 0,1 set high impedence
   XBR0	=	0x00;						    
   XBR1	=	0x40;						//	Enable Crossbar
}

void	Page_Erase(BYTE*	Page_Address)	small
{
	BYTE	EA_Save;					//	Used to save state of global interrupt enable
	BYTE	xdata	*pwrite;			//	xdata pointer used to generate movx intruction

	EA_Save	=	EA;						//	Save current EA
	EA	=	0;							//	Turn off interrupts
	pwrite	=	(BYTE xdata *)(Page_Address);	//	Set write pointer to Page_Address
	PSCTL	=	0x03;					//	Enable flash erase and writes

	FLKEY	=	0xA5;					//	Write flash key sequence to FLKEY
	FLKEY	= 	0xF1;
	*pwrite	=	0x00;					//	Erase flash page using a write command

	PSCTL	=	0x00;					//	Disable flash erase and writes
	EA	=	EA_Save;					//	Restore state of EA
}

void	Page_Write(BYTE*	PageAddress)	small
{
	BYTE	EA_Save;					//	Used to save state of global interrupt enable
	BYTE	xdata	*pwrite;			//	Write Pointer
	BYTE	xdata	*pread;				//	Read Pointer
	UINT	x;							//	Counter for 0-512 bytes
	
	pread	=	(BYTE xdata *)(TempStorage);
	EA_Save	=	EA;						//	Save EA
	EA	=	0;							//	Turn off interrupts
	pwrite	=	(BYTE xdata *)(PageAddress);
	PSCTL	=	0x01;					//	Enable flash writes
	for(x = 0;	x<FLASH_PAGE_SIZE;	x++)//	Write 512 bytes
	{
		FLKEY	=	0xA5;				//	Write flash key sequence
		FLKEY	=	0xF1;
		*pwrite	=	*pread;				//	Write data byte to flash

		pread++;						//	Increment pointers
		pwrite++;
	}
	PSCTL	=	0x00;					//	Disable flash writes
	EA	=	EA_Save;					//	Restore EA
}

void	State_Machine(void)			
{
	switch	(M_State)
	{
		case	ST_RX_SETUP:			
			Receive_Setup();			//	Receive and decode host Setup Message
			break;
		case	ST_RX_FILE:
			Receive_File();				//	Receive File data from host
			break;
		case	ST_TX_ACK:
			M_State =	ST_RX_FILE;		//	Ack Transmit complete, continue RX data
			break;
		case	ST_TX_FILE:				//	Send file data to host
			WriteStageLength = ((BytesToWrite - BytesWrote) > MAX_BLOCK_SIZE_WRITE)? MAX_BLOCK_SIZE_WRITE:(BytesToWrite - BytesWrote);
			BytesWrote	+=	Block_Write((BYTE*)(ReadIndex), WriteStageLength);
			ReadIndex += WriteStageLength;

			if	((BlocksWrote%8) == 0)	Led2 = ~Led2;
			if	(BytesWrote == NumBytes)	Led2 = 0;
			break;
		default:
			break;
	}
}


	//	ISR for USB_API, run when API interrupts are enabled, and an interrupt is received
void 	USB_API_TEST_ISR(void)	interrupt	16
{
	BYTE	INTVAL	=	Get_Interrupt_Source();	//	Determine type of API interrupts
	if	(INTVAL	&	USB_RESET)					//	Bus Reset Event, go to Wait State
	{
		M_State	=	ST_WAIT_DEV;
	}

	if	(INTVAL	&	DEVICE_OPEN)				//	Device opened on host, go to Idle
	{
		M_State	=	ST_IDLE_DEV;
	}

	if	(INTVAL	&	TX_COMPLETE)
	{
		if	(M_State == ST_RX_FILE)				//	Ack Transmit complete, go to RX state
		{
			M_State	=	(ST_TX_ACK);
		}
		if	(M_State == ST_TX_FILE)				//	File block transmit complete, go to TX state
		{
			M_State	=	(BytesWrote == BytesToWrite) ? ST_IDLE_DEV :ST_TX_FILE;	//	Go to Idle when done
		}
	}
	if (INTVAL	&	RX_COMPLETE)				//	RX Complete, go to RX Setup or RX file state
	{
		M_State	=	(M_State == ST_IDLE_DEV) ? ST_RX_SETUP : ST_RX_FILE;
	}
	if	(INTVAL	&	DEVICE_CLOSE)				//	Device closed, wait for re-open
	{
		M_State	=	ST_WAIT_DEV;
	}
	if	(INTVAL	&	FIFO_PURGE)					//	Fifo purged, go to Idle State
	{
		M_State	=	ST_IDLE_DEV;
	}

	State_Machine();							//	Call state machine routine
}

void	Receive_Setup(void)
{
	BytesRead	=	Block_Read(&Buffer,	3);		//	Read Setup Message
	
	if (Buffer[0]	==	READ_MSG)				//	Check See if Read File Setup
	{			
		PageIndex	=	0;						//	Reset Index
		NumBlocks	=	LengthFile[2];			//	Read NumBlocks from flash stg
		NumBlocks	= (NumBlocks > MAX_NUM_BLOCKS)?	MAX_NUM_BLOCKS:	NumBlocks; // only write as many bytes 
												// as we have space available
		Buffer[0]	=	SIZE_MSG;				//	Send host size of transfer message
		Buffer[1]	=	LengthFile[1];
		Buffer[2]	=	LengthFile[0];
		BytesToWrite	=	Buffer[1]	+	256*Buffer[2];	
		BytesWrote	=	Block_Write((BYTE*)&Buffer,	3);
		M_State	=	ST_TX_FILE;					//	Go to TX data state
		BytesWrote	=	0;
		ReadIndex	=	PageIndices[0];
		Led2	=	1;		
	}
	else	//	Otherwise assume Write Setup Packet
	{
		BytesToRead	=	Buffer[1]	+	256*Buffer[2];	
		NumBlocks	=	(BYTE)(BytesToRead/MAX_BLOCK_SIZE_READ);	//	Find NumBlocks

		if (BytesToRead > MAX_NUM_BYTES)						//	State Error if transfer too big
		{
			M_State = ST_ERROR;
		}
		else
		{

			if	(BytesToRead%MAX_BLOCK_SIZE_READ)	NumBlocks++;	//	Increment NumBlocks for last partial block

			TempStorage->Piece[0]	=	Buffer[2];
			TempStorage->Piece[1]	=	Buffer[1];
			TempStorage->Piece[2]	=	NumBlocks;

			//	Write Values to Flash
			Page_Erase(0x2000);								//	Store file data to flash
			Page_Write(0x2000);

			PageIndex	=	0;									//	Reset Index 
			BlockIndex	=	0;
			BytesRead	=	0;
			Led1	=	1;
			M_State = ST_RX_FILE;								//	Go to RX data state
		}
	}
}

void	Receive_File(void)
{
	ReadStageLength = ((BytesToRead - BytesRead) > MAX_BLOCK_SIZE_READ)? MAX_BLOCK_SIZE_READ:(BytesToRead - BytesRead);
	
	BytesRead	+=	Block_Read((BYTE*)(&TempStorage[BlockIndex]), ReadStageLength);	//	Read Block

	BlockIndex++;
	//	If device has received as many bytes as fit on one FLASH page, disable interrupts,
	//	write page to flash, reset packet index, enable interrupts
	//	Send handshake packet 0xFF to host after FLASH write
	if	((BlockIndex	==	(BLOCKS_PR_PAGE))	||	(BytesRead	==	BytesToRead))
	{
		Page_Erase((BYTE*)(PageIndices[PageIndex]));
		Page_Write((BYTE*)(PageIndices[PageIndex]));
		PageIndex++;
		Led1 = ~Led1;
		BlockIndex	=	0;
		Buffer[0]	=	0xFF;
		Block_Write(Buffer,	1);			//	Send handshake Acknowledge to host
	}

	//	Go to Idle state if last packet has been received
	if	(BytesRead	==	BytesToRead)	{M_State	=	ST_IDLE_DEV;	Led1	=	0;}
}


