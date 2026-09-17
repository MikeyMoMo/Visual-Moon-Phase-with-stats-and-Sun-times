#define RGB565(r,g,b) ((((r>>3)<<11) | ((g>>2)<<5) | (b>>3)))
#define RGB888(r,g,b) ((r << 16) | (g << 8) | b)

//SleepTime = xIN_RANGE(12, 23, 10);  // Should return false
#define iIN_RANGE(v, low, high) (((low) <= (high)) ? ((v) >= (low) && (v) <= (high)) : ((v) >= (low) || (v) <= (high)))
#define xIN_RANGE(v, low, high) (((low) <= (high)) ? ((v) > (low) && (v) < (high)) : ((v) > (low) || (v) < (high)))

#include "TFT_eSPI.h"
TFT_eSPI    tft = TFT_eSPI();
TFT_eSprite spriteBG = TFT_eSprite(&tft);    // Background sprite.
TFT_eSprite spriteSF = TFT_eSprite(&tft);    // Starfield  sprite.
// The Base version of the sprite is needed for scrolling the starfield without extra
//  clutter leaving a trail across the sprite from the scrolling.  After 120the .scroll,
//  Base is coped into the SF then the moon is added.  Rinse, repeat...
TFT_eSprite spriteSF_Base = TFT_eSprite(&tft);    // Starfield  sprite work area.
TFT_eSprite spriteMoonInvis = TFT_eSprite(&tft);  // Moon intermediary for invisiblility.
TFT_eSprite spriteMenu   = TFT_eSprite(&tft);     // Menu sprite.
#include <MoonStruck.h>
MoonStruck MS;
// This is a free API key from: ipgeolocation.io
// Get a kay here: https://app.ipgeolocation.io/
// The free key gives you up to 1000 pulls per day.  This code uses 144/day.
#define apiKey "8046078e6bc64b2784e7e6a51dc31343"  // From: ipgeolocation.io

int         dispLine1 = 8, dispLine2, dispLine3, dispLine4, dispLine5, dispLine6;

#define ORIENT_POWER_RIGHT 1
#define ORIENT_POWER_LEFT  3
#define BUTTON_PRESSED 0
#define BUTTON_NOT_PRESSED 1

// Include the jpeg decoder library
#include <TJpg_Decoder.h>
JRESULT JPB_RC;   // Return code from drawing BG pic.

#include "MoonRise.h"
MoonRise mr;
struct tm * moonTimes;
struct tm * sunTimes;
bool showMoon = true;

#include <MoonPhase.hpp>  // #include <MoonPhase.hpp>
MoonPhase moonPhase;      // include a MoonPhase instance
moonData_t moon;          // variable (struct?) to receive the data
struct tm *tmlocalTime;

struct MoonOrientation {
  int adjustedAngle;   // normalized angle for text labels and filenames
  int rotationAngle;   // rotation for pushRotated()
};

#include <SunRise.h>
SunRise sr;

#include <WiFi.h>
String   stringIP;  // IP address.
int      tftBL_Lvl = 75, prevBL_Lvl = -1;      // tft brightness, prev brightness.

#include <esp_sntp.h>      // Get UTC epoch here.
time_t   TS_Epoch = 0;  // Set by Time Sync, printed by loop if not 0.
time_t   UTC;
time_t   now;
time_t   startMillis;
time_t   showPhase = 0;     // If millis() is larger than this, show phase
time_t   showPhaseDelay = 2000;  // How long to delay Phase show
//float    MS_PA_Angle;   // MoonStruck Parallactic Angle
//float    librationLon;  // degrees
//float    librationLat;  // degrees
float    moonRotationAngle;  // degrees

struct tm timeinfo;
int       prevSec = -1;  // One second gate.
int       iDOM, iMonth, iYear, iHour, iPrevHour = -1, iMin, iSec;
int       brightness;
int       count;
int       topButton, bottomButton;  // Will be set depending on board orientation.
int       colorChoice;
int       nTemp;
int       pixInLittleFS;
bool      SleepTime;
bool      veriPix = false;
const char* fileName;
const char* extension;

#include <JPEGDecoder.h>  // JPEG decoder library

#include <LITTLEFS.h>
#define FORMAT_LittleFS_IF_FAILED true

char      chBuffer[100];     // Work area for sprintf, etc.
char      moonPhasePic[100]; // Loaded moon phase picture
char      chBLChange[30];    // Save last brightness setting string
char      chHour[3];         // Hour.
String    sVer;

#if defined CONFIG_FOR_JOE    // My friend's WAP credentials and location
const char* chSSID      = "N_Port";           // Your router SSID.
const char* chPassword  = "helita1943";       // Your router password.
String      Hemisphere  = "north";            // or "south"
const double lat        = 38.052147;          // Your location.
const double lon        = -122.153893;
const int WakeupHour    = 10;  // Default turn on display time
const int SleepHour     = 23;  // Default turn off display time  // was 23
const int defaultBright = 60;
const int myOrientation = ORIENT_POWER_LEFT;

#else                         // My WAP credentials and location.

const char*  chSSID        = "Converge2G";        // Your router SSID.
const char*  chPassword    = "Lallave@Family7";   // Your router password.
String       Hemisphere    = "north";  // or "south"
const double lat           = 18.5376;            // Your location.
const double lon           = 120.7671;
int32_t      Phase         = 0.;
const int    WakeupHour    = 10;  // Default turn on display time
const int    SleepHour     = 23;  // Default turn off display time
const int    defaultBright = 60;
const int    myOrientation = ORIENT_POWER_LEFT;
#endif

#include <Preferences.h>
Preferences preferences;
#define RO_MODE true   // Preferences calls Read-only mode
#define RW_MODE false  // Preferences calls Read-write mode

// The maximum brightness that the button will allow.
#define MAX_BRIGHTNESS  254  // T-Display-S3 display brightness maximum.
// The minimum brightness that the button will allow.
#define MIN_BRIGHTNESS    0  // T-Display-S3 display brightness minimum.

#define MENU_HIDE_TIME 15000  // With no button press, menu will disappear in this many ms.

//Moon
const String TXT_MOON_NEW             = "New Moon";
const String TXT_MOON_WAXING_CRESCENT = "Waxing Crescent";
const String TXT_MOON_FIRST_QUARTER   = "First Quarter";
const String TXT_MOON_WAXING_GIBBOUS  = "Waxing Gibbous";
const String TXT_MOON_FULL            = "Full Moon";
const String TXT_MOON_WANING_GIBBOUS  = "Waning Gibbous";
const String TXT_MOON_THIRD_QUARTER   = "Third Quarter";
const String TXT_MOON_WANING_CRESCENT = "Waning Crescent";

#define      SPR_MENU_FONT_SIZE       4  // Menu sprite uses builtin font 4
#define      SPR_MENU_HEIGHT         40  // Menu sprite height is 42 pixels
#define      SPR_MENU_WIDTH         320  // Menu sprite width is the display width

int     BLchange;                        // Backlight change amount
int     prevHour = -1;                   // We have the BL for this hour
int     dRead;                           // Reading from digitalRead.
int     font4Height;                     // Height of the built-in font 4 for line spacing.

time_t  BLChangeMillis = 0;
time_t  menuHide;

// Used to be routines but I figured out how to make it a macro.  Not so hard!
///***************************************************************************/
//bool iInRange (int v, int low, int high)  // Inclusive
///***************************************************************************/
//{
//  return (low <= high) ? (v >= low && v <= high) : (v >= low || v <= high);
//}
///***************************************************************************/
//bool xInRange (int v, int low, int high)  // Exclusive
///***************************************************************************/
//{
////SleepTime = xIN_RANGE(21, 23, 10);  // Should return false
//  return (low  <=  high)  ? ( v  >  low  &&  v  < high)   : ( v  >  low  ||  v  <  high);
//}

/*  Notes for Build_and_Show routine:

    Well, the following needs a little explanation.  It took a while to figure out the order
    of stuff to do and I want to remember and you to know what is happening here.  It is
    fairly simple to understand now that it is complete.

    First, a note: spriteSF_Base is some 20 pixels wider than spriteSF.

    Step 1: I have to put the starfield that is kept in spriteSF_Base onto the
    display starfield.  spriteSF_Base is a bit wider then the display starfield sprite
    (spriteSF).  That's so I can create the "stars" and have them appear to scroll
    onto the screen rather than suddenly appear, fully formed on the right edge of the moon
    window. So extra stars are waiting, hidden, to come onto the screen in a minute or two.
    That's done by the next code statement of pushToSprite.  The right end of the Base
    starfield gets truncated and that's exactly what I wanted to happen. So, now, we have
    a  black background and a starfield in spriteBG sprite.

    Note again: I have to determine the moon phase picture to use, then...
    Step 2: I have to push the moon jpg into a sprite.  But, since all jpgs
    are square, it has four sorta-triangles on it in four places -- the "corners".
    I had to edit these and make them totally black for the next step.

    Step 3: Now that I have the moon jpg with its totally black corners in a sprite, I need
    to get rid of the black corners.  To do this, I have to push the moon sprite onto
    another sprite with the invisible color (black) noted so those pixels will not be
    copied to the second sprite.  That is done with the pushToSprite, 4 code lines below.
    The target sprite already has a black background and you would not see the four corners
    but there is still a problem. Those four corners would obscure the stars that happened
    along where they are.  So, they have to be lopped off with the pushToSprite setting
    black as the invisible color to leave them behind.  So, now, we have a black sprite
    that has had a slightly wide starfield pushed onto it and then a moon image that is
    now round, without any corners pressed on top of the start field.  Yes, that obscures
    some stars but that's exactly what should be happening.

    And, suddenly, we are done.

    Step 4 and onward: That just involves putting text on the screen. Nothing special here.
    All of that stuff goes right onto the spriteBG and when I am done with the text, I
    throw spriteBG on the screen for the happy users to see and watch the stars scroll by.

    This all happens every second and, last time I checked, takes about 60 ms.
    That's very little of the one second I have to wait to do the next update so
    there is lots of just waiting around...

    Is it soup yet?  No.
    Is it soup yet?  No.
    Is it soup yet?  No.
    Is it soup yet?  No.
    Is it soup yet?  No.

    Cards, anyone?
*/
