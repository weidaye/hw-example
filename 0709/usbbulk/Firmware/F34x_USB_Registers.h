//-----------------------------------------------------------------------------
// F34x_USB_Registers.h
//-----------------------------------------------------------------------------
// Copyright 2005 Silicon Laboratories, Inc.
// http://www.silabs.com
//
// Program Description:
//
// Header file for USB firmware. Includes all USB core register
// addresses, register access macros, and register bit masks.
//
//
// How To Test:    See Readme.txt
//
//
// FID:            34X000010
// Target:         C8051F34x
// Tool chain:     Keil C51 7.50 / Keil EVAL C51
//                 Silicon Laboratories IDE version 2.6
// Command Line:   See Readme.txt
// Project Name:   F34x_USB_Bulk
//
//
// Release 1.3
//    -All changes by GP
//    -21 NOV 2005
//    -Changed revision number to match project revision
//     No content changes to this file
//    -Modified file to fit new formatting guidelines
//    -Changed file name from usb_regs.h
//
//
// Release 1.2
//    -Initial Revision (JS)
//    -XX AUG 2003
//

#ifndef  F34x_USB_REGISTERS_H
#define  F34x_USB_REGISTERS_H

//-----------------------------------------------------------------------------
// Global Constants
//-----------------------------------------------------------------------------

// USB Core Registers
#define  BASE     0x00
#define  FADDR    BASE
#define  POWER    BASE + 0x01
#define  IN1INT   BASE + 0x02	//USB0����˵��жϼĴ���
#define  OUT1INT  BASE + 0x04 	//USB0����˵��жϼĴ���
#define  CMINT    BASE + 0x06 	//USB0�����жϼĴ���
#define  IN1IE    BASE + 0x07
#define  OUT1IE   BASE + 0x09
#define  CMIE     BASE + 0x0B
#define  FRAMEL   BASE + 0x0C
#define  FRAMEH   BASE + 0x0D
#define  INDEX    BASE + 0x0E 	//USB0 �˵������Ĵ��� 
#define  CLKREC   BASE + 0x0F
#define  E0CSR    BASE + 0x11	//USB0�˵� 0���ƼĴ��� 
#define  EINCSRL  BASE + 0x11
#define  EINCSRH  BASE + 0x12
#define  EOUTCSRL BASE + 0x14
#define  EOUTCSRH BASE + 0x15
#define  E0CNT    BASE + 0x16
#define  EOUTCNTL BASE + 0x16
#define  EOUTCNTH BASE + 0x17
#define  FIFO_EP0 BASE + 0x20
#define  FIF0_EP1 BASE + 0x21
#define  FIF0_EP2 BASE + 0x22
#define  FIFO_EP3 BASE + 0x23

// USB Core Register Bits

// POWER
#define  rbISOUD        0x80
#define  rbSPEED        0x40
#define  rbUSBRST       0x08
#define  rbRESUME       0x04
#define  rbSUSMD        0x02
#define  rbSUSEN        0x01

// IN1INT
#define  rbIN3          0x08
#define  rbIN2          0x04
#define  rbIN1          0x02
#define  rbEP0          0x01

// OUT1INT
#define  rbOUT3         0x08
#define  rbOUT2         0x04
#define  rbOUT1         0x02

// CMINT
#define  rbSOF          0x08
#define  rbRSTINT       0x04
#define  rbRSUINT       0x02
#define  rbSUSINT       0x01

// IN1IE
#define  rbIN3E         0x08
#define  rbIN2E         0x04
#define  rbIN1E         0x02
#define  rbEP0E         0x01

// OUT1IE
#define  rbOUT3E        0x08
#define  rbOUT2E        0x04
#define  rbOUT1E        0x02

// CMIE
#define  rbSOFE         0x08
#define  rbRSTINTE      0x04
#define  rbRSUINTE      0x02
#define  rbSUSINTE      0x01

// E0CSR
#define  rbSSUEND       0x80
#define  rbSOPRDY       0x40	//SOPRDY��OPRDY ����λ �����Ӧ�ڴ�������յ��Ķ˵� 0���ݰ��󽫸�λ�á�1��
#define  rbSDSTL        0x20	//SDSTL������ STALL  �������ͨ�����λд��1����������ǰ�����ݴ��䣨��Ϊ������������ϣ���Ĵ�������ȣ�
#define  rbSUEND        0x10	//SUENDλ:setup������־λ
#define  rbDATAEND      0x08	//DATAEND�����ݽ���
#define  rbSTSTL        0x04	//STSTLλ: �ѷ��� STALL
#define  rbINPRDY       0x02	//INPRDY�����루IN�����ݰ�׼���� ���Ӧ�ڽ�һ��Ҫ���͵����ݰ�װ��˵� 0 FIFO �����λд��1��
#define  rbOPRDY        0x01	//OPRDY�������OUT�����ݰ�׼���� 

// EINCSRL
#define  rbInCLRDT      0x40
#define  rbInSTSTL      0x20
#define  rbInSDSTL      0x10
#define  rbInFLUSH      0x08
#define  rbInUNDRUN     0x04
#define  rbInFIFONE     0x02
#define  rbInINPRDY     0x01

// EINCSRH
#define  rbInDBIEN      0x80
#define  rbInISO        0x40
#define  rbInDIRSEL     0x20
#define  rbInFCDT       0x08
#define  rbInSPLIT      0x04

// EOUTCSRL
#define  rbOutCLRDT     0x80
#define  rbOutSTSTL     0x40
#define  rbOutSDSTL     0x20
#define  rbOutFLUSH     0x10
#define  rbOutDATERR    0x08 
#define  rbOutOVRUN     0x04
#define  rbOutFIFOFUL   0x02
#define  rbOutOPRDY     0x01

// EOUTCSRH
#define  rbOutDBOEN     0x80
#define  rbOutISO       0x40

//-----------------------------------------------------------------------------
// Macros
//-----------------------------------------------------------------------------

// Register read/write macros
#define UREAD_BYTE(addr,target) {  USB0ADR = (0x80 | addr); \
                                   while(USB0ADR & 0x80);   \
								   target = USB0DAT; }


#define UWRITE_BYTE(addr,data)  {  USB0ADR = addr; \
                                   USB0DAT = data; \
								   while(USB0ADR & 0x80); }


#endif                                 // F34x_USB_REGISTERS_H

//-----------------------------------------------------------------------------
// End Of File
//-----------------------------------------------------------------------------