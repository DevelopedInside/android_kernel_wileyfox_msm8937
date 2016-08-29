#define ABSSUB(a,b) ((a > b) ? (a - b) : (b - a))

char ilitek_GestureMatchProcess(unsigned char ucCurrentPointNum,unsigned char ucLastPointNum,unsigned short curx,unsigned short cury);
void ilitek_readgesturelist(void);
char ilitek_GetGesture(void);
short ilitek_gesture_model_value_x(int i);
short ilitek_gesture_model_value_y(int i);
