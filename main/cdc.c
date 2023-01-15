

#include "tusb.h"
#include "stdbool.h"

extern void setMscEnabledOriginal(int ena);
extern int isMscEnabled();

int flagListDir = 0;


uint8_t tmpbuf[128];

static int  strBuffPtr = 0;
static char strBuff [ 128 ];

static char printfBuff [ 256 ];

void usbprintf(char * msg, ...)
{
   va_list list;
   va_list args;
   int len;

   va_start(list, msg);
   va_copy(args, list);
   va_end(list);

   if ((len = vsprintf(printfBuff, msg, args)) > 0)
   {
    tud_cdc_write(printfBuff, len);
    tud_cdc_write_flush();
   }
}

void usbprint(char * msg)
{
    tud_cdc_write(msg, strlen(msg));
    tud_cdc_write_flush();
}

static void execCommand(char * cmd, int len)
{
    if (!memcmp(cmd, "msc", 3) || !memcmp(cmd, "MSC", 3))
    {
      if (atoi (cmd + 4))
      {
        if (isMscEnabled())
        {
          usbprint("\r\nMSC already ON\r\n");
        }
        else
        {
          usbprint("\r\nturning MSC on\r\n");
          //esp_restart();
          setMscEnabledOriginal(1);
        }
      }
      else
      {
        if (!isMscEnabled())
        {
          usbprint("\r\nMSC already OFF\r\n");
        }
        else
        {
          usbprint("\r\nturning MSC off\r\n");
          flagListDir = 1;
          setMscEnabledOriginal(0);
        }
      }
    }
    else
        usbprintf("unknown commnad\n");
}


//--------------------------------------------------------------------+
// USB CDC
//--------------------------------------------------------------------+
void cdc_task(void* params)
{
  (void) params;

  // RTOS forever loop
  while ( 1 )
  {
    // connected() check for DTR bit
    // Most but not all terminal client set this when making connection
    if ( tud_cdc_connected() )
    {
//      if (flagListDir)
//      {
//        spisd_list_root();
//
//        flagListDir = 0;
//      }

      // connected and there are data available
      if ( tud_cdc_available() )
      {
        char * on = (char*)&tmpbuf[0];

        uint32_t count = tud_cdc_read(tmpbuf, sizeof(tmpbuf));

        tud_cdc_write(tmpbuf, count);
        tud_cdc_write_flush();

        while (count--)
        {
            // dumb overrun protection
            if (strBuffPtr >= sizeof(strBuff))
            {
              strBuffPtr = 0;
            }

            if ( ('\r' == *on) ||  ('\n' == *on))
            {
                strBuff[strBuffPtr] = 0;

                execCommand(strBuff, strBuffPtr);

                strBuffPtr = 0;
            }
            else
            {
                strBuff[strBuffPtr] = *on;

                strBuffPtr++;
            }

            on++;
        }
      }
    }

    vTaskDelay(1);
  }
}

// Invoked when cdc when line state changed e.g connected/disconnected
void tud_cdc_line_state_cb(uint8_t itf, bool dtr, bool rts)
{
  (void) itf;
  (void) rts;

  // TODO set some indicator
  if ( dtr )
  {
    // Terminal connected
  }else
  {
    // Terminal disconnected
  }
}

// Invoked when CDC interface received data from host
void tud_cdc_rx_cb(uint8_t itf)
{
  (void) itf;
}
