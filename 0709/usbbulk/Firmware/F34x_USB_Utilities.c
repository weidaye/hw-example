//-----------------------------------------------------------------------------
// F34x_USB_Utilities.c
//-----------------------------------------------------------------------------
// Copyright 2005 Silicon Laboratories, Inc.
// http://www.silabs.com
//
// Program Description:
//
// Source file for USB firmware. Includes the following support routines:
//
// - HaltEndpoint()
// - EnableEndpoint()
// - GetEpStatus()
// - SetConfiguration()
// - SetInterface()
// - FIFOWrite()
// - FIFORead()
//
//
// How To Test:    See Readme.txt
//
//
// FID:            34X000014
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
//    -Changed file name from usb_utils.c
//
//
// Release 1.2
//    -Initial Revision (JS)
//    -XX AUG 2003
//

//-----------------------------------------------------------------------------
// Includes
//-----------------------------------------------------------------------------

#include "c8051F340.h"
#include "F34x_USB_Registers.h"
#include "F34x_USB_Structs.h"
#include "F34x_USB_Main.h"
#include "F34x_USB_Descriptors.h"
#include "F34x_USB_Config.h"
#include "F34x_USB_Request.h"

//-----------------------------------------------------------------------------
// Extern Global Variables
//-----------------------------------------------------------------------------

extern DEVICE_STATUS    gDeviceStatus;
extern code DESCRIPTORS gDescriptorMap;
extern DEVICE_STATUS    gDeviceStatus;
extern EP_STATUS        gEp0Status;
extern EP_STATUS        gEp2OutStatus;
extern EP_STATUS        gEp1InStatus;

//-----------------------------------------------------------------------------
// HaltEndpoint
//-----------------------------------------------------------------------------
//
// Return Value : None
// Parameters   : 
// 1) UINT uEp
//
//-----------------------------------------------------------------------------
BYTE HaltEndpoint (UINT uEp)
{
   BYTE bReturnState, bIndex;

   // Save current INDEX value and target selected endpoint
   UREAD_BYTE (INDEX, bIndex);
   UWRITE_BYTE (INDEX, (BYTE)uEp & 0x00EF);

   // Halt selected endpoint and update its status flag
   switch (uEp)
   {
      case EP1_IN:
         UWRITE_BYTE (EINCSRL, rbInSDSTL);
         gEp1InStatus.bEpState = EP_HALTED;
         bReturnState = EP_IDLE;          // Return success flag
         break;
      case EP2_OUT:
         UWRITE_BYTE (EOUTCSRL, rbOutSDSTL);
         gEp2OutStatus.bEpState = EP_HALTED;
         bReturnState = EP_IDLE;          // Return success flag
         break;
      default:
         bReturnState = EP_ERROR;         // Return error flag
                                          // if endpoint not found
         break;
   }

   UWRITE_BYTE (INDEX, bIndex);           // Restore saved INDEX
   return bReturnState;
}

//-----------------------------------------------------------------------------
// EnableEndpoint
//-----------------------------------------------------------------------------
//
// Return Value : None
// Parameters   : 
// 1) UINT uEp
//
//-----------------------------------------------------------------------------
BYTE EnableEndpoint (UINT uEp)
{
   BYTE bReturnState, bIndex;

   // Save current INDEX value and target selected endpoint
   UREAD_BYTE (INDEX, bIndex);
   UWRITE_BYTE (INDEX, (BYTE)uEp & 0x00EF);

   // Flag selected endpoint has HALTED
   switch (uEp)
   {
      case EP1_IN:
         // Disable STALL condition and clear the data toggle
         UWRITE_BYTE (EINCSRL, rbInCLRDT);
         gEp1InStatus.bEpState = EP_IDLE; // Return success
         bReturnState = EP_IDLE;
         break;
      case EP2_OUT:
         // Disable STALL condition and clear the data toggle
         UWRITE_BYTE (EOUTCSRL, rbOutCLRDT);
         gEp2OutStatus.bEpState = EP_IDLE;// Return success
         bReturnState = EP_IDLE;
         break;
      default:
         bReturnState = EP_ERROR;         // Return error
                                          // if no endpoint found
         break;
   }

   UWRITE_BYTE (INDEX, bIndex);           // Restore INDEX

   return bReturnState;
}

//-----------------------------------------------------------------------------
// GetEpStatus
//-----------------------------------------------------------------------------
//
// Return Value : None
// Parameters   : 
// 1) UINT uEp
//
//-----------------------------------------------------------------------------
BYTE GetEpStatus (UINT uEp)
{
   BYTE bReturnState;

   // Get selected endpoint status
   switch (uEp)
   {
      case EP1_IN:
         bReturnState = gEp1InStatus.bEpState;
         break;
      case EP2_OUT:
         bReturnState = gEp2OutStatus.bEpState;
         break;
      default:
         bReturnState = EP_ERROR;
         break;
   }

   return bReturnState;
}

//-----------------------------------------------------------------------------
// SetConfiguration
//-----------------------------------------------------------------------------
//
// Return Value : None
// Parameters   : 
// 1) BYTE SelectConfig
//
//-----------------------------------------------------------------------------
BYTE SetConfiguration(BYTE SelectConfig)
{
   BYTE bReturnState = EP_IDLE;           // Endpoint state return value

   PIF_STATUS pIfStatus;                  // Pointer to interface status
                                          // structure

   // Store address of selected config desc
   gDeviceStatus.pConfig = &gDescriptorMap.bCfg1;

   // Confirm that this configuration descriptor matches the requested
   // configuration value
   if (gDeviceStatus.pConfig[cfg_bConfigurationValue] != SelectConfig)
   {
      bReturnState = EP_ERROR;
   }

   else
   {
      // Store number of interfaces for this configuration
      gDeviceStatus.bNumInterf = gDeviceStatus.pConfig[cfg_bNumInterfaces];

      // Store total number of interface descriptors for this configuration
      gDeviceStatus.bTotalInterfDsc = MAX_IF;

      // Get pointer to the interface status structure
      pIfStatus = (PIF_STATUS)&gDeviceStatus.IfStatus[0];

      // Build Interface status structure for Interface0
      pIfStatus->bIfNumber = 0;           // Set interface number
      pIfStatus->bCurrentAlt = 0;         // Select alternate number zero
      pIfStatus->bNumAlts = 0;            // No other alternates

      SetInterface(pIfStatus);            // Configure Interface0, Alternate0

      gDeviceStatus.bDevState = DEV_CONFIG;// Set device state to configured
      gDeviceStatus.bCurrentConfig = SelectConfig;// Store current config
   }

   return bReturnState;
}

//-----------------------------------------------------------------------------
// SetInterface
//-----------------------------------------------------------------------------
//
// Return Value : None
// Parameters   : 
// 1) PIF_STATUS pIfStatus
//
//-----------------------------------------------------------------------------
BYTE SetInterface(PIF_STATUS pIfStatus)
{
   BYTE bReturnState = EP_IDLE;
   BYTE bIndex;

   // Save current INDEX value
   UREAD_BYTE (INDEX, bIndex);

   // Add actions for each possible interface alternate selections
   switch(pIfStatus->bIfNumber)
   {
      // Configure endpoints for interface0
      case 0:
         // Configure Endpoint1 IN
         UWRITE_BYTE(INDEX, 1);           // Index to Endpoint1 registers

         // direction = IN ; Double-buffering enabled
         UWRITE_BYTE(EINCSRH, (rbInDIRSEL | rbInDBIEN));      
         gEp1InStatus.uNumBytes = 0;      // Reset byte counter
         gEp1InStatus.uMaxP = EP1_IN_MAXP;// Set maximum packet size
         gEp1InStatus.bEp = EP1_IN;       // Set endpoint address
         gEp1InStatus.bEpState = EP_IDLE; // Set endpoint state

         // Endpoint2 OUT
         UWRITE_BYTE(INDEX, 2);           // Index to Endpoint2 registers
         // Double-buffering enabled ; direction = OUT
         UWRITE_BYTE(EOUTCSRH, rbOutDBOEN);      
         gEp2OutStatus.uNumBytes = 0;     // Reset byte counter
         gEp2OutStatus.uMaxP = EP2_OUT_MAXP;// Set maximum packet size
         gEp2OutStatus.bEp = EP2_OUT;     // Set endpoint number
         gEp2OutStatus.bEpState = EP_IDLE;// Set endpoint state

         UWRITE_BYTE(INDEX, 0);           // Return to index 0

         break;

      // Configure endpoints for interface1
      case 1:

      // Configure endpoints for interface2
      case 2:

      // Default (error)
      default:
         bReturnState = EP_ERROR;
   }
   UWRITE_BYTE (INDEX, bIndex);           // Restore INDEX

   return bReturnState;
}

//-----------------------------------------------------------------------------
// SetInterface
//-----------------------------------------------------------------------------
//
// Return Value : None
// Parameters   : 
// 1) BYTE bEp
// 2) UINT uNumBytes
// 3) BYTE * pData
//
// Read from the selected endpoint FIFO
//
//-----------------------------------------------------------------------------
void FIFORead (BYTE bEp, UINT uNumBytes, BYTE * pData)
{
   BYTE TargetReg;
   UINT i;

   // If >0 bytes requested,
   if (uNumBytes)
   {
      TargetReg = FIFO_EP0 + bEp;         // Find address for target
                                          // endpoint FIFO

      USB0ADR = (TargetReg & 0x3F);       // Set address (mask out bits7-6)
      USB0ADR |= 0xC0;                    // Set auto-read and initiate
                                          // first read

      // Unload <NumBytes> from the selected FIFO
      for(i=0;i<uNumBytes-1;i++)
      {
         while(USB0ADR & 0x80);           // Wait for BUSY->'0' (data ready)
         pData[i] = USB0DAT;              // Copy data byte
      }


      while(USB0ADR & 0x80);              // Wait for BUSY->'0' (data ready)
      pData[i] = USB0DAT;                 // Copy data byte
      USB0ADR = 0;                        // Clear auto-read
   }
}

//-----------------------------------------------------------------------------
// FIFOWrite
//-----------------------------------------------------------------------------
//
// Return Value : None
// Parameters   : 
// 1) BYTE bEp
// 2) UINT uNumBytes
// 3) BYTE * pData
//
// Write to the selected endpoint FIFO
//
//-----------------------------------------------------------------------------
void FIFOWrite (BYTE bEp, UINT uNumBytes, BYTE * pData)
{
   BYTE TargetReg;
   UINT i;

   // If >0 bytes requested,
   if (uNumBytes)
   {
      TargetReg = FIFO_EP0 + bEp;         // Find address for target
                                          // endpoint FIFO

      while(USB0ADR & 0x80);              // Wait for BUSY->'0'
                                          // (register available)
      USB0ADR = (TargetReg & 0x3F);       // Set address (mask out bits7-6)

      // Write <NumBytes> to the selected FIFO
      for(i=0;i<uNumBytes;i++)
      {
         USB0DAT = pData[i];
         while(USB0ADR & 0x80);           // Wait for BUSY->'0' (data ready)
      }
   }
}

//-----------------------------------------------------------------------------
// End Of File
//-----------------------------------------------------------------------------