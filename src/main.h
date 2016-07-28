#define SCREEN_W 144
#define SCREEN_W2 72 // half width
#define SCREEN_H 168
#define SCREEN_H2 84 // half height

#define TOTAL_TIME_DIGITS 4

#define DIGIT_Y 5
#define DIGIT_X1 4
#define DIGIT_X2 40
#define DIGIT_X1_S 82
#define DIGIT_X2_S 114
#define BHOFFSET 4

GPoint mImageSizeBig = { .x = 33, .y = 158 };
GPoint mImageSizeSm = { .x = 23, .y = 74 };

GFont mFont;
char* mDayText = NULL;
int mDayTextSize = sizeof("WEDNESDAY ");
GPoint mDayPoint = { .x = 78, .y = 94 };
GPoint mDayPointAlt = { .x = 0, .y = 94 };

bool mBigHour = true;