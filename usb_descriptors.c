/*
 * Licensed under GNU Public License v3
 * Copyright (c) 2022 Thomas Roth <code@stacksmashing.net>
 * Based on:
 * Copyright (c) 2019 Ha Thach (tinyusb.org)
 *
 */

#include "tusb.h"

/* A combination of interfaces must have a unique product id, since PC will save device driver after the first plug.
 * Same VID/PID with different interface e.g MSC (first), then CDC (later) will possibly cause system error on PC.
 *
 * Auto ProductID layout's Bitmap:
 *   [MSB]       MIDI | HID | MSC | CDC          [LSB]
 */
#define _PID_MAP(itf, n)  ( (CFG_TUD_##itf) << (n) )
#define USB_PID 0xfffd
// #define USB_PID           (0x4000 | _PID_MAP(CDC, 0) | _PID_MAP(MSC, 1) | _PID_MAP(HID, 2) | \
//                            _PID_MAP(MIDI, 3) | _PID_MAP(VENDOR, 4) )

#define USB_VID   0x37de
#define USB_BCD   0x0200



/** If not already defined in your project, define the length of the
 *  minimal Microsoft OS 2.0 Platform Descriptor in BOS (header only).
 */
#ifndef TUD_BOS_MS_OS_20_DESC_LEN
  #define TUD_BOS_MS_OS_20_DESC_LEN 28
#endif


//--------------------------------------------------------------------+
// Custom Vendor Request Codes
//--------------------------------------------------------------------+
// Pick any non-colliding values in the 0x01-0xFF range
#define VENDOR_REQUEST_WEBUSB      0x01
#define VENDOR_REQUEST_MICROSOFT   0x02

//--------------------------------------------------------------------+
// Device Descriptors
//--------------------------------------------------------------------+
tusb_desc_device_t const desc_device =
{
    .bLength            = sizeof(tusb_desc_device_t),
    .bDescriptorType    = TUSB_DESC_DEVICE,
    .bcdUSB             = 0x0210, // At least 2.1 or 3.x for BOS & WebUSB

    // Use Interface Association Descriptor (IAD) for CDC
    // As required by USB Specs IAD's subclass must be common class (2) and protocol must be IAD (1)
    .bDeviceClass       = 0xEF,
    .bDeviceSubClass    = MISC_SUBCLASS_COMMON,
    .bDeviceProtocol    = MISC_PROTOCOL_IAD,
    .bMaxPacketSize0    = CFG_TUD_ENDPOINT0_SIZE,

    .idVendor           = USB_VID,
    .idProduct          = USB_PID,
    .bcdDevice          = 0x0100,

    .iManufacturer      = 0x01,
    .iProduct           = 0x02,
    .iSerialNumber      = 0x03,

    .bNumConfigurations = 0x01
};

// Invoked when received GET DEVICE DESCRIPTOR
// Application return pointer to descriptor
uint8_t const * tud_descriptor_device_cb(void)
{
  return (uint8_t const *) &desc_device;
}

//--------------------------------------------------------------------+
// Configuration Descriptor
//--------------------------------------------------------------------+
/* 1. Interface numbers ------------------------------------------------ */
enum
{
  ITF_NUM_VENDOR = 0,        /* NEW – replaces former CDC_0                */
  ITF_NUM_PROBE,               /* existing vendor interface (Picoprobe)      */

  ITF_NUM_CDC_0,               /* single remaining CDC function              */
  ITF_NUM_CDC_0_DATA,

  ITF_NUM_TOTAL
};

/* 2. Endpoint numbers -------------------------------------------------- */
/* Vendor‑0 keeps the easy‑to‑remember 0x02 / 0x83 that the old CDC used */
#define EPNUM_VENDOR0_OUT   0x02
#define EPNUM_VENDOR0_IN    0x83

#define EPNUM_PROBE_OUT     0x04
#define EPNUM_PROBE_IN      0x85

#define EPNUM_CDC_0_NOTIF   0x86
#define EPNUM_CDC_0_OUT     0x07
#define EPNUM_CDC_0_IN      0x88

/* 3. Total length: 2×Vendor + 1×CDC (no IAD needed for vendor) --------- */
#define CONFIG_TOTAL_LEN  ( TUD_CONFIG_DESC_LEN       \
                          + 8 + 8 \
                          + 2 * TUD_VENDOR_DESC_LEN   \
                          +      TUD_CDC_DESC_LEN )

/* 4. Full configuration descriptor array ------------------------------ */

/* The interface association below is required for pyusb/libusb1 to
   be able to talk to the Faultier on Windows. We assign WinUSB to the two
   vendor interfaces, but libusb1 can only use those if they use IAD. */
uint8_t const desc_fs_configuration[] =
{
  /* Config number, interface count, string index, total length,
     attributes, power (mA) */
  TUD_CONFIG_DESCRIPTOR(1, ITF_NUM_TOTAL, 0,
                        CONFIG_TOTAL_LEN,
                        TUSB_DESC_CONFIG_ATT_REMOTE_WAKEUP, 100),

  /* Interface Association for vendor interface 1 */
  8, TUSB_DESC_INTERFACE_ASSOCIATION, ITF_NUM_VENDOR, 1, TUSB_CLASS_VENDOR_SPECIFIC, 0x0, 0x0, 0,

  /* 1  First vendor interface (replaces the old CDC_0 pair) */
  TUD_VENDOR_DESCRIPTOR(ITF_NUM_VENDOR, 0,
                        EPNUM_VENDOR0_OUT, EPNUM_VENDOR0_IN, 64),
  /* Interface Association for vendor interface 1 */
  8, TUSB_DESC_INTERFACE_ASSOCIATION, ITF_NUM_PROBE, 1, TUSB_CLASS_VENDOR_SPECIFIC, 0x0, 0x0, 0,
  /* 2  Existing Picoprobe vendor interface — unchanged        */
  TUD_VENDOR_DESCRIPTOR(ITF_NUM_PROBE, 0,
                        EPNUM_PROBE_OUT, EPNUM_PROBE_IN, 64),

  /* 3  Single remaining CDC function                          */
  TUD_CDC_DESCRIPTOR(ITF_NUM_CDC_0, 4,          /* string #4 reused */
                     EPNUM_CDC_0_NOTIF, 8,
                     EPNUM_CDC_0_OUT, EPNUM_CDC_0_IN, 64),
};

// Invoked when received GET CONFIGURATION DESCRIPTOR
// Application return pointer to descriptor
// Descriptor contents must exist long enough for transfer to complete
uint8_t const * tud_descriptor_configuration_cb(uint8_t index)
{
  (void) index; // for multiple configurations

  return desc_fs_configuration;
}

//--------------------------------------------------------------------+
// BOS Descriptor
//--------------------------------------------------------------------+
/*
 * The BOS (Binary Object Store) descriptor can contain multiple
 * "Platform Capability" entries. Here we add:
 *
 * 1) WebUSB Platform Capability
 * 2) Microsoft OS 2.0 Platform Capability
 *
 * The TUD_BOS_WEBUSB_DESCRIPTOR() macro sets:
 *   bRequest = VENDOR_REQUEST_WEBUSB (0x01)
 *   iLandingPage = 1 (string index for the URL)
 *
 * The TUD_BOS_MS_OS_20_DESCRIPTOR() macro sets:
 *   bMS_VendorCode = VENDOR_REQUEST_MICROSOFT (0x02)
 *   wTotalLength = MS_OS_20_DESC_LEN
 *
 * If your TinyUSB is older or doesn't have these macros,
 * you can define the bytes by hand.
 */
#define BOS_TOTAL_LEN      (TUD_BOS_DESC_LEN + TUD_BOS_MS_OS_20_DESC_LEN)

// Our custom MS OS 2.0 descriptor is 0xB2 bytes
// #define MS_OS_20_DESC_LEN  0xB2

#define MS_OS_20_PROP_LEN     0x0084          // registry‑property descriptor
#define MS_OS_20_SUBSET_LEN   (0x0008 /*hdr*/ + 0x0014 /*WINUSB*/ + MS_OS_20_PROP_LEN)
#define MS_OS_20_DESC_LEN     (0x000A /*set hdr*/ + 0x0008 /*cfg hdr*/ + 2*MS_OS_20_SUBSET_LEN)

uint8_t const desc_bos[] =
{
  // BOS header: length, number of device capabilities
  TUD_BOS_DESCRIPTOR(BOS_TOTAL_LEN, 1),

  // 1) WebUSB Platform Capability
  //    - bRequest = VENDOR_REQUEST_WEBUSB
  //    - iLandingPage = 1
  // TUD_BOS_WEBUSB_DESCRIPTOR(VENDOR_REQUEST_WEBUSB, /*iLandingPage=*/1),

  // 2) Microsoft OS 2.0 Platform Capability
  //    - bMS_VendorCode = VENDOR_REQUEST_MICROSOFT
  //    - wTotalLength = MS_OS_20_DESC_LEN
  TUD_BOS_MS_OS_20_DESCRIPTOR(MS_OS_20_DESC_LEN, VENDOR_REQUEST_MICROSOFT)
};

uint8_t const * tud_descriptor_bos_cb(void)
{
  return desc_bos;
}

//--------------------------------------------------------------------+
// WebUSB Landing Page Descriptor
//--------------------------------------------------------------------+
/*
 * Because we're NOT using CFG_TUD_WEBUSB, we must manually respond
 * to the WebUSB GET_URL request in our vendor control callback.
 *
 * This descriptor is the "landing page" that Chrome might prompt the user
 * to open after the device is plugged in.
 *
 * Format:
 *   byte[0] = bLength = 3 + URL length
 *   byte[1] = bDescriptorType = 3 (WEBUSB URL)
 *   byte[2] = bScheme (0x00 = http, 0x01 = https)
 *   byte[3..] = URL in ASCII
 */

// "https://flash.hextree.io"
uint8_t const desc_webusb_url[] =
{
  // bLength = 3 + length_of("flash.hextree.io")
  3 + (sizeof("flash.hextree.io") - 1),
  // bDescriptorType = 3 (WEBUSB URL)
  3,
  // bScheme = 1 => "https://"
  0x01,
  // URL in ASCII
  'f','l','a','s','h','.','h','e','x','t','r','e','e','.','i','o'
};
//--------------------------------------------------------------------+
// Microsoft OS 2.0 Descriptor
//--------------------------------------------------------------------+
/*
 * This tells Windows to automatically load WinUSB for interface 0.
 * Compatible ID = "WINUSB"
 */
uint8_t const desc_ms_os_20[] =
{
  // ------------------- Set Header -------------------
  // wLength (10), wDescriptorType=0, dwWindowsVersion=0x06030000, wTotalLength=0x00B2
  U16_TO_U8S_LE(0x000A), U16_TO_U8S_LE(MS_OS_20_SET_HEADER_DESCRIPTOR),
  U32_TO_U8S_LE(0x06030000),
  U16_TO_U8S_LE(MS_OS_20_DESC_LEN),

  // -------------- Configuration Subset Header --------------
  // wLength=8, wDescriptorType=1, bConfigurationValue=0, bReserved=0
  // wTotalLength= MS_OS_20_DESC_LEN - 10
  U16_TO_U8S_LE(0x0008), U16_TO_U8S_LE(MS_OS_20_SUBSET_HEADER_CONFIGURATION),
  0, 0,
  U16_TO_U8S_LE(MS_OS_20_DESC_LEN - 0x0A),

  // ---------------- Function Subset Header -----------------
  // wLength=8, wDescriptorType=2, bFirstInterface=ITF_NUM_VENDOR, bReserved=0
  // wTotalLength= MS_OS_20_DESC_LEN - 0x0A - 0x08
  U16_TO_U8S_LE(0x0008), U16_TO_U8S_LE(MS_OS_20_SUBSET_HEADER_FUNCTION),
  ITF_NUM_VENDOR, 0,
  U16_TO_U8S_LE(MS_OS_20_SUBSET_LEN),

  // ---------- Compatible ID Descriptor ----------
  // wLength=20, wDescriptorType=3
  // "WINUSB\0\0"
  U16_TO_U8S_LE(0x0014), U16_TO_U8S_LE(MS_OS_20_FEATURE_COMPATBLE_ID),
  'W','I','N','U','S','B', 0x00, 0x00,
  0x00,0x00,0x00,0x00, 0x00,0x00,0x00,0x00,

  // ------- Registry Property Descriptor (optional) --------
  // wLength = remainder, wDescriptorType=4
  U16_TO_U8S_LE(MS_OS_20_PROP_LEN),
  U16_TO_U8S_LE(MS_OS_20_FEATURE_REG_PROPERTY),

  // wPropertyDataType=0x0007 (REG_MULTI_SZ), wPropertyNameLength=0x002A
  // PropertyName="DeviceInterfaceGUIDs\0" (UTF-16)
  U16_TO_U8S_LE(0x0007), U16_TO_U8S_LE(0x002A),
  'D',0x00,'e',0x00,'v',0x00,'i',0x00,'c',0x00,'e',0x00,
  'I',0x00,'n',0x00,'t',0x00,'e',0x00,'r',0x00,'f',0x00,
  'a',0x00,'c',0x00,'e',0x00,'G',0x00,'U',0x00,'I',0x00,
  'D',0x00,'s',0x00, 0x00,0x00,

  // wPropertyDataLength=0x0050
  // bPropertyData="{975F44D9-0D08-43FD-8B3E-127CA8AFFF9D}\0" (UTF-16)
  U16_TO_U8S_LE(0x0050),
  '{',0x00,'9',0x00,'7',0x00,'5',0x00,'F',0x00,'4',0x00,
  '4',0x00,'D',0x00,'9',0x00,'-',0x00,'0',0x00,'D',0x00,
  '0',0x00,'8',0x00,'-',0x00,'4',0x00,'3',0x00,'F',0x00,
  'D',0x00,'-',0x00,'8',0x00,'B',0x00,'3',0x00,'E',0x00,
  '-',0x00,'1',0x00,'2',0x00,'7',0x00,'C',0x00,'A',0x00,
  '8',0x00,'A',0x00,'F',0x00,'F',0x00,'F',0x00,'9',0x00,
  'F',0x00,'}',0x00, 0x00,0x00, 0x00,0x00,


  // ---------------- Function Subset Header -----------------
  // wLength=8, wDescriptorType=2, bFirstInterface=ITF_NUM_VENDOR, bReserved=0
  // wTotalLength= MS_OS_20_DESC_LEN - 0x0A - 0x08
  U16_TO_U8S_LE(0x0008), U16_TO_U8S_LE(MS_OS_20_SUBSET_HEADER_FUNCTION),
  ITF_NUM_PROBE, 0,
  U16_TO_U8S_LE(MS_OS_20_SUBSET_LEN),

  // ---------- Compatible ID Descriptor ----------
  // wLength=20, wDescriptorType=3
  // "WINUSB\0\0"
  U16_TO_U8S_LE(0x0014), U16_TO_U8S_LE(MS_OS_20_FEATURE_COMPATBLE_ID),
  'W','I','N','U','S','B', 0x00, 0x00,
  0x00,0x00,0x00,0x00, 0x00,0x00,0x00,0x00,

  // ------- Registry Property Descriptor (optional) --------
  // wLength = remainder, wDescriptorType=4
  U16_TO_U8S_LE(MS_OS_20_PROP_LEN),
  U16_TO_U8S_LE(MS_OS_20_FEATURE_REG_PROPERTY),

  // wPropertyDataType=0x0007 (REG_MULTI_SZ), wPropertyNameLength=0x002A
  // PropertyName="DeviceInterfaceGUIDs\0" (UTF-16)
  U16_TO_U8S_LE(0x0007), U16_TO_U8S_LE(0x002A),
  'D',0x00,'e',0x00,'v',0x00,'i',0x00,'c',0x00,'e',0x00,
  'I',0x00,'n',0x00,'t',0x00,'e',0x00,'r',0x00,'f',0x00,
  'a',0x00,'c',0x00,'e',0x00,'G',0x00,'U',0x00,'I',0x00,
  'D',0x00,'s',0x00, 0x00,0x00,

  // wPropertyDataLength=0x0050
  // bPropertyData="{975F44D9-0D08-43FD-8B3E-127CA8AFFF9D}\0" (UTF-16)
  U16_TO_U8S_LE(0x0050),
  '{',0x00,'9',0x00,'7',0x00,'5',0x00,'F',0x00,'4',0x00,
  '4',0x00,'D',0x00,'9',0x00,'-',0x00,'0',0x00,'D',0x00,
  '0',0x00,'8',0x00,'-',0x00,'4',0x00,'3',0x00,'F',0x00,
  'D',0x00,'-',0x00,'8',0x00,'B',0x00,'3',0x00,'E',0x00,
  '-',0x00,'1',0x00,'2',0x00,'7',0x00,'C',0x00,'A',0x00,
  '8',0x00,'A',0x00,'F',0x00,'F',0x00,'F',0x00,'9',0x00,
  'E',0x00,'}',0x00, 0x00,0x00, 0x00,0x00
};

// Ensure descriptor size matches
TU_VERIFY_STATIC(sizeof(desc_ms_os_20) == MS_OS_20_DESC_LEN, "MS OS 2.0 descriptor length mismatch!");
// TU_VERIFY_STATIC(sizeof(desc_ms_os_20) == 0xb2, "MS OS 2.0 descriptor length mismatch!");

//--------------------------------------------------------------------+
// Vendor Control Request Callback
//--------------------------------------------------------------------+
/*
 * We handle two custom vendor requests:
 *
 * 1) VENDOR_REQUEST_WEBUSB (0x01): 
 *    If Chrome wants the "landing page" descriptor, it will issue:
 *      - bmRequestType = 0xC0 (vendor, device-to-host)
 *      - bRequest = VENDOR_REQUEST_WEBUSB (0x01)
 *      - wIndex = 1 (the iLandingPage we set in BOS)
 *    We must return desc_webusb_url[].
 *
 * 2) VENDOR_REQUEST_MICROSOFT (0x02):
 *    If Windows wants the MS OS 2.0 descriptor, it will issue:
 *      - bmRequestType = 0xC0
 *      - bRequest = VENDOR_REQUEST_MICROSOFT (0x02)
 *      - wIndex = 7
 *    We must return desc_ms_os_20[].
 */
bool tud_vendor_control_xfer_cb(uint8_t rhport, uint8_t stage, tusb_control_request_t const * request)
{
  if (stage != CONTROL_STAGE_SETUP) return true; // Only handle SETUP stage

  // Handle each vendor request as needed
  if (request->bRequest == VENDOR_REQUEST_WEBUSB)
  {
    // Chrome requesting our landing page descriptor
    // Typically: bmRequestType=0xC0, wIndex=1
    if ( (request->bmRequestType == 0xC0) && (request->wIndex == 1) )
    {
      // The first byte of desc_webusb_url[] is its length
      uint8_t desc_size = desc_webusb_url[0];
      return tud_control_xfer(rhport, request, (void*)desc_webusb_url, desc_size);
    }
  }
  else if (request->bRequest == VENDOR_REQUEST_MICROSOFT)
  {
    // Windows requesting the MS OS 2.0 descriptor
    // Typically: bmRequestType=0xC0, wIndex=7
    if ( (request->bmRequestType == 0xC0) && (request->wIndex == 7) )
    {
      return tud_control_xfer(rhport, request, (void*)desc_ms_os_20, MS_OS_20_DESC_LEN);
    }
  }

  // Stall any unknown vendor request
  return false;
}
//--------------------------------------------------------------------+
// String Descriptors
//--------------------------------------------------------------------+

// array of pointer to string descriptors
char const* string_desc_arr [] =
{
  (const char[]) { 0x09, 0x04 }, // 0: is supported language is English (0x0409)
  "stacksmashing",                     // 1: Manufacturer
  "Faultier",              // 2: Product
  "faultier",                      // 3: Serials, should use chip ID
  "Faultier Control",                 // 4: Control Vendor Interface
  "Faultier Probe",                 // 5: Probe Vendor Interface
  "Faultier Serial",                 // 6: CDC Interface
};

static uint16_t _desc_str[32];

// Invoked when received GET STRING DESCRIPTOR request
// Application return pointer to descriptor, whose contents must exist long enough for transfer to complete
uint16_t const* tud_descriptor_string_cb(uint8_t index, uint16_t langid)
{
  (void) langid;

  uint8_t chr_count;

  if ( index == 0)
  {
    memcpy(&_desc_str[1], string_desc_arr[0], 2);
    chr_count = 1;
  }else
  {
    // Note: the 0xEE index string is a Microsoft OS 1.0 Descriptors.
    // https://docs.microsoft.com/en-us/windows-hardware/drivers/usbcon/microsoft-defined-usb-descriptors

    if ( !(index < sizeof(string_desc_arr)/sizeof(string_desc_arr[0])) ) return NULL;

    const char* str = string_desc_arr[index];

    // Cap at max char
    chr_count = strlen(str);
    if ( chr_count > 31 ) chr_count = 31;

    // Convert ASCII string into UTF-16
    for(uint8_t i=0; i<chr_count; i++)
    {
      _desc_str[1+i] = str[i];
    }
  }

  // first byte is length (including header), second byte is string type
  _desc_str[0] = (TUSB_DESC_STRING << 8 ) | (2*chr_count + 2);

  return _desc_str;
}
