#include "EPD_1in54_V2.h"
#include "DEV_Config.h"
#include "GUI_Paint.h"
#include "esl_display.h"

#define MAX_TXT_LENGHT              10  // It depends on used font

#define IMAGE_SIZE ((EPD_1IN54_V2_WIDTH % 8 == 0)? (EPD_1IN54_V2_WIDTH / 8 ): (EPD_1IN54_V2_WIDTH / 8 + 1)) * EPD_1IN54_V2_HEIGHT

NO_INIT(static UBYTE image[IMAGE_SIZE]);

extern const unsigned char gImage_st_logo_80x47[470];
extern const unsigned char gImage_euro_symbol_30[120];
extern const unsigned char gImage_sale_tag_70x69[790];
extern const unsigned char gImage_star_70x64[576];

struct
{
  const unsigned char * img;
  uint8_t width;
  uint8_t height; 
} icons[] = 
{
  {
    .img = gImage_sale_tag_70x69,
    .width = 70,
    .height = 69,
  },
  {
    .img = gImage_star_70x64,
    .width = 70,
    .height = 64,
  },  
};

extern sFONT Font_britannic_bold70;
extern sFONT Font_courbd30;

void ESL_DISPLAY_Init(uint8_t init_image)
{
  APP_DBG_MSG("Initializing Display...");
  
  DEV_Module_Init();
  EPD_1IN54_V2_Init();  
  
  Paint_NewImage(image, EPD_1IN54_V2_WIDTH, EPD_1IN54_V2_HEIGHT, 270, WHITE);
  
  if(init_image)
  {
    Paint_Clear(WHITE);
    Paint_DrawBitMap_Paste(gImage_st_logo_80x47, 0, 200-47, 80, 47, 0);
    Paint_DrawBitMap_Paste(gImage_euro_symbol_30, 0, 92, 29, 30, 0); 
    Paint_DrawLine(5, 50, 195, 50, BLACK, DOT_PIXEL_3X3, LINE_STYLE_SOLID);
  }
  
  EPD_1IN54_V2_Sleep();
  
  APP_DBG_MSG("Done\n");
}

void ESL_DISPLAY_Clear(void)
{ 
  DEV_Module_Init();
  EPD_1IN54_V2_Init();  
  EPD_1IN54_V2_Clear();  
  EPD_1IN54_V2_Sleep();
}

void ESL_DISPLAY_Show(void)
{
  APP_DBG_MSG("Updating display...\n");
  DEV_Module_Init();
  EPD_1IN54_V2_Init();
    /* TODO: use an interrupt-based function and/or a background task whenever the display
     needs to be updated.  */
  EPD_1IN54_V2_Display(image);
  EPD_1IN54_V2_Sleep();
  APP_DBG_MSG("Done\n");
}

int ESL_DISPLAY_SetPrice(uint16_t price_int, uint8_t price_fract)
{
  char price_int_str[10];
  char price_fract_str[10];
  
  if(price_int > 999 || price_fract > 99)
    return 1;
  
  Paint_ClearWindows(28, 100-Font_britannic_bold70.Height/2,
                     200, 100+Font_britannic_bold70.Height/2, WHITE);
  
  sprintf(price_int_str, "%3d.", price_int);
  sprintf(price_fract_str, "%02d", price_fract);
  
  Paint_DrawString_EN(28,
                      100-Font_britannic_bold70.Height/2,
                      price_int_str, &Font_britannic_bold70, WHITE, BLACK);
  
  Paint_DrawString_EN(165,
                      80,
                      price_fract_str, &Font24, WHITE, BLACK);
  
  return 0;  
}

int ESL_DISPLAY_SetText(const char *txt)
{
  char crop_txt[MAX_TXT_LENGHT + 1] = {0};
  
  strncpy(crop_txt, txt, MIN(MAX_TXT_LENGHT, strlen(txt)));
  
  Paint_ClearWindows(0, 0, 200-15, Font_courbd30.Height, WHITE);
  
  Paint_DrawString_EN(5, 5, crop_txt, &Font_courbd30, WHITE, BLACK);
  
  return 0;  
}

int ESL_DISPLAY_SetIcon(uint8_t icon_index)
{
  if(icon_index > 2)
  {
    return 1;
  }
  
  Paint_ClearWindows(100, 200-69, 170, 200, WHITE);
  
  if(icon_index > 0)
  {
    Paint_DrawBitMap_Paste(icons[icon_index - 1].img,
                           100, 200 - icons[icon_index - 1].height,
                           icons[icon_index - 1].width, icons[icon_index - 1].height, 0); 
  }
  
  return 0;
}

void ESL_DISPLAY_SetState(uint8_t state)
{
  Paint_ClearWindows(200-7, 0, 200, 7, WHITE);
  
  switch(state)
  {
  case ESL_STATE_ADV:
    Paint_DrawCircle(200-3, 4, 3, BLACK, DOT_PIXEL_1X1, DRAW_FILL_EMPTY);
    break;
  case ESL_STATE_SYNC:
    Paint_DrawCircle(200-3, 4, 3, BLACK, DOT_PIXEL_1X1, DRAW_FILL_FULL);
    break;
  default:
    break;    
  }
}
                           
void ESL_DISPLAY_SetID(uint8_t group_id, uint8_t esl_id)
{  
  Paint_ClearWindows(200-16, 200-Font12.Height, 200, 200, WHITE);  
  Paint_DrawNum(200-16, 200-Font12.Height, group_id, &Font12, BLACK, WHITE);
  Paint_DrawNum(200-7, 200-Font12.Height, esl_id, &Font12, BLACK, WHITE);
}
