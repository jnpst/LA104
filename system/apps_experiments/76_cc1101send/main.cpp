#include <library.h>
#include "../../os_host/source/framework/Console.h"
#include "assert.h"
#include "device/json.h"
#include "device/cc1101.h"

using namespace BIOS;

CDeviceCC1101 gModem;

#define CC1101_MARCSTATE         0x35        // Main Radio Control State Machine State
#define WRITE_BURST              0x40
#define READ_SINGLE              0x80
#define READ_BURST               0xC0

/**
 * Type of register
 */
#define CC1101_CONFIG_REGISTER   READ_SINGLE
#define CC1101_STATUS_REGISTER   READ_BURST
#define CC1101_TXFIFO            0x3F        // TX FIFO address
#define CC1101_RXFIFO            0x3F        // RX FIFO address
#define CC1101_TXBYTES           0x3A        // Underflow and Number of Bytes
#define CC1101_RXBYTES           0x3B        // Overflow and Number of Bytes

bool send()
{
  struct packet_t
  {
    int length{2};
    uint8_t data[32];
  } packet;

  packet.data[0] = 0xaa;
  packet.data[1] = 0xaa;

  uint8_t marcState;
  int res = 0;

//    gModem.SetIdleState();
//    gModem.Calibrate();
//BIOS::SYS::DelayMs(5);
//  gModem.delayMicroseconds(500);

/*
  gModem.SetRxState();

  while (((marcState = gModem.Read(CC1101_MARCSTATE | CC1101_STATUS_REGISTER)) & 0x1F) != 0x0D)
  {
    CONSOLE::Color(RGB565(808080));
    CONSOLE::Print("(marc1=0x%02x)", marcState);
    CONSOLE::Color(RGB565(ffffff));

    if (marcState == 0x11)        // RX_OVERFLOW
      gModem.FlushRxFifo();
  }

  gModem.delayMicroseconds(500);
*/
/*
  marcState = gModem.Read(CC1101_MARCSTATE | CC1101_STATUS_REGISTER) & 0x1F;
    CONSOLE::Print("(marc:%x) ", marcState); BIOS::SYS::DelayMs(5);
  marcState = gModem.Read(CC1101_MARCSTATE | CC1101_STATUS_REGISTER) & 0x1F;
    CONSOLE::Print("(marc:%x) ", marcState); BIOS::SYS::DelayMs(5);
  marcState = gModem.Read(CC1101_MARCSTATE | CC1101_STATUS_REGISTER) & 0x1F;
    CONSOLE::Print("(marc:%x) ", marcState); BIOS::SYS::DelayMs(5);
  marcState = gModem.Read(CC1101_MARCSTATE | CC1101_STATUS_REGISTER) & 0x1F;
    CONSOLE::Print("(marc:%x) ", marcState); BIOS::SYS::DelayMs(5);
  */
  if (packet.length > 0)
  {
    CONSOLE::Print("sending %d bytes ", packet.length);
    gModem.SetDataRate(1000);
    gModem.SetOutputPower(0x50);
    gModem.SetPacketLength(packet.length);
    gModem.Write(CC1101_TXFIFO, packet.data, packet.length);

    // CCA enabled: will enter TX state only if the channel is clear
    gModem.SetTxState();
  }

//BIOS::SYS::DelayMs(5);
//  gModem.delayMicroseconds(8500);

  // Check that TX state is being entered (state = RXTX_SETTLING)
  do {
    marcState = gModem.Read(CC1101_MARCSTATE | CC1101_STATUS_REGISTER) & 0x1F;
  } while ((marcState != 0x13) && (marcState != 0x14) && (marcState != 0x15));

//    CONSOLE::Color(RGB565(808080));
//    CONSOLE::Print("(marc2=0x%02x)", marcState);
//    CONSOLE::Color(RGB565(ffffff));

  if((marcState != 0x13) && (marcState != 0x14) && (marcState != 0x15))
  {
    CONSOLE::Color(RGB565(ff0000));
    CONSOLE::Print("(no tx mode, 0x%02x)", marcState);
    CONSOLE::Color(RGB565(ffffff));

    gModem.SetIdleState();
    gModem.FlushTxFifo();

    return false;
  }

  // Wait for the sync word to be transmitted
//  CONSOLE::Print(".");

//  while (!gModem.Read());
//  CONSOLE::Print(".");

  // Wait until the end of the packet transmission
 // while (gModem.Read());
//  CONSOLE::Print(".");

  // Check that the TX FIFO is empty
  int txbytes = 0;
  do {
    txbytes = gModem.Read(CC1101_TXBYTES | CC1101_STATUS_REGISTER) & 0x7F;
  } while (txbytes > 0);

  if(txbytes == 0)
    res = true;
  else {
    CONSOLE::Color(RGB565(ff0000));
    CONSOLE::Print("(not sent, tx bytes=%d)", txbytes);
    CONSOLE::Color(RGB565(ffffff));
  }

//    gModem.SetIdleState();
//    gModem.FlushTxFifo();

  return res;
}

void setup()
{
  while (1)
  {
    CONSOLE::Print("Init... ");
    if (gModem.Init())
    {
      CONSOLE::Print("Ok!\n");
      break;
    } else
    {
      CONSOLE::Print("Failed!\n");
    }
  }
}

void loop()
{
  static long lastRequest = 0;
  static int seq = 0;
  
  long now = SYS::GetTick();
  if (now - lastRequest > 2000)
  {
    lastRequest = now;
    CONSOLE::Print("Sending...");
    if (send())
      CONSOLE::Print("Ok. \n");
    else
      CONSOLE::Print("Failed. \n");
  }
}

#ifdef _ARM
__attribute__((__section__(".entry")))
#endif
int _main(void)
{
    CRect rcClient(0, 0, LCD::Width, LCD::Height);
    LCD::Bar(rcClient, RGB565(0000b0));
    
    CRect rc1(rcClient);
    rc1.bottom = 14;
    GUI::Background(rc1, RGB565(4040b0), RGB565(404040));    
    LCD::Print(8, rc1.top, RGB565(ffffff), RGBTRANS, "CC1101 test signal generator");

    setup();

    CRect rc2(rcClient);
    rc2.top = rc2.bottom-14;
    GUI::Background(rc2, RGB565(404040), RGB565(202020));
    LCD::Printf(8, rc2.top, RGB565(808080), RGBTRANS, "F: %.1f MHz, BW: %.1 kHz", gModem.GetFrequency() / 1e6f, gModem.GetBandwidth() / 1e3f);

    KEY::EKey key;
    while ((key = KEY::GetKey()) != KEY::EKey::Escape)
    {
      loop();
    }
    
    return 0;
}