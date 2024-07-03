#ifndef ESL_DISPLAY_H
#define ESL_DISPLAY_H

#define ESL_STATE_OFF   0
#define ESL_STATE_ADV   1
#define ESL_STATE_SYNC  2

void ESL_DISPLAY_Init(uint8_t init_image);

void ESL_DISPLAY_Clear(void);

void ESL_DISPLAY_Show(void);

int ESL_DISPLAY_SetText(const char *txt);

int ESL_DISPLAY_SetPrice(uint16_t price_int, uint8_t price_fract);

int ESL_DISPLAY_SetIcon(uint8_t icon_index);

void ESL_DISPLAY_SetState(uint8_t state);

void ESL_DISPLAY_SetID(uint8_t group_id, uint8_t esl_id);

#endif /* ESL_DISPLAY_H */