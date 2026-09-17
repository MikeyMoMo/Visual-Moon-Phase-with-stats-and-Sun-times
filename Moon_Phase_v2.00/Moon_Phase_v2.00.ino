//*****************************************************************************************//
//   Created by Mike Morrow on May 10th, 2025. GitHub: MikeyMoMo                           //
//                                                                                         //
//   The latest update will be available on GitHub.                                        //
//                                                                                         //
//   You may use this for any non-commercial purpose as long as you leave in the           //
//    creator attribution.                                                                 //
//                                                                                         //
//   Parts of this code was either created or fixed by CoPilot and DeepSeek.               //
//   I promised that I would double CoPilot's current salary of $0.  It was so happy.      //
//                                                                                         //
/*
  Notes for future Mikey and others:
  - Partition table must have: spiffs,data,littlefs
  - Place partitions.csv in sketch folder (auto-picked up by build)
  - Use ESP32 LittleFS Data Upload tool, not SPIFFS
  - Moon face images (.jpg) go in LittleFS
  - Test sketch: Enumerate_LittleFS confirms files are loaded
  - This code now computes Libration, the apparent rotation of the moon face by
    latitude, north and south, but in a linear calculation for now.

  # Name,   Type, SubType, Offset,  Size, Flags
  nvs,data,nvs,0x9000,0x5000,
  otadata,data,ota,0xE000,0x2000,
  app0,app,ota_0, ,0x300000,
  spiffs,data,littlefs, ,0xCD0000,
  coredump,data,coredump, ,0x10000,
*/
//*****************************************************************************************//

// This define controls setting of WAP SSID & Password, location and timezone.
//#define CONFIG_FOR_JOE  // My friend.  We co-develop code so this makes it easy.

#define DO_DEBUG_PRINTS

#include "defines.h"
/*******************************************************************************************/
void setup()
/*******************************************************************************************/
{
  Serial.begin(115200); delay(2000);

  tft.init();  // Init the tft.  What else?
  tft.setRotation(3);
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.setTextPadding(tft.width());
  tft.setTextDatum(TC_DATUM);

  printVers();
  tft.fillScreen(TFT_BLACK);
  tft.drawString("Moon Phase v" + sVer, tft.width() / 2, dispLine2, 4);

  spriteBG.createSprite(tft.width(), tft.height());  // The big kahuna. The big cheese!
  spriteBG.setSwapBytes(false);

  font4Height = spriteBG.fontHeight(4);
  dispLine2 = dispLine1 + font4Height;
  dispLine3 = dispLine2 + font4Height;
  dispLine4 = dispLine3 + font4Height;
  dispLine5 = dispLine4 + font4Height;
  dispLine6 = dispLine5 + font4Height;
  Serial.printf("Display line   %3i, %3i, %3i, %3i, %3i, %3i\r\n", 1, 2, 3, 4, 5, 6);
  Serial.printf("on display row %3i, %3i, %3i, %3i, %3i, %3i\r\n",
                dispLine1, dispLine2, dispLine3, dispLine4, dispLine5, dispLine6);

  // Asjust pin function based on display orientation.
  if (myOrientation == ORIENT_POWER_LEFT) {
    // Increase brightness is always on top, and...
    topButton = 0; bottomButton = 14;
  } else {
    // Decrease brightness is always on the bottom button.
    topButton = 14; bottomButton = 0;
  }

  tft.fillScreen(TFT_BLACK);
  tft.drawString("Moon Phase v" + sVer, tft.width() / 2, dispLine2, 4);

  Serial.println("This is Moon Phase and Time on T-Display S3.");
  //  Serial.println("Running from:");
  //  Serial.println(__FILE__);
  Serial.println("For the T-Display S3 target board, use tft_espi config# 206");

  tft.fillScreen(TFT_BLACK);
  tft.drawString("Moon Phase v" + sVer, tft.width() / 2, dispLine2, 4);
  tft.drawString("Initializing LittleFS", tft.width() / 2, dispLine4, 4);
  // Initialise LittleFS
  Serial.println("Initializing LittleFS.");
  if (!LittleFS.begin(FORMAT_LittleFS_IF_FAILED)) {
    Serial.println("LittleFS initialisation failed!");
    while (1) yield(); // Stay here twiddling thumbs waiting
  }
  tft.fillScreen(TFT_BLACK);
  tft.drawString("Moon Phase v" + sVer, tft.width() / 2, dispLine2, 4);
  tft.drawString("Counting .jpg files", tft.width() / 2, dispLine4, 4);
  Serial.println("Counting .jpg files in LittleFS."); delay(1000);
  pixInLittleFS = 0;
  File root = LittleFS.open("/");
  File file = root.openNextFile();
  while (file) {
    // fileLen = strlen(file.name());
    // The default max filename length in LittleFS is 255 bytes for ESP32.
    // This can be changed in lfs.h in the library. Look for #define "LFS_NAME_MAX 255" in
    //  that file.  255 is a lot and if you increase it, it will add to the memory used to
    //  store the files.  It is a trivial amount, however, comparted to file data size.
    //  Serial.print("The filename "); Serial.print(file.name());
    //  Serial.printf(" is %i bytes long.\r\n", fileLen);  // Verify filename byte length.
    fileName = file.name();  // Get it somewhere usable, locally.
    extension = &fileName[strlen(fileName) - 4];  // Get the last 4 bytes of the filename.
    if (veriPix) {
      Serial.print("File in LittleFS: "); Serial.println(fileName); delay(50);
    }
    if (strcmp(extension, ".jpg") == 0)  // Is this a jpeg file (.jpg)?
      pixInLittleFS++;  // If so, count it.
    file = root.openNextFile();  // Next file, please.  There are no folder used here.
  }
  Serial.printf("LittleFS Initialization done. There are %i pics loaded.\r\n",
                pixInLittleFS);  // Whew!

  // Initialize TJpgDec
  TJpgDec.setSwapBytes(true); // We need to swap the colour bytes (endianess)
  // The jpeg image can be scaled by a factor of 1, 2, 4, or 8
  TJpgDec.setJpgScale(1);
  // The decoder must be given the exact name of the rendering function.
  TJpgDec.setCallback(jpg_output_Sprite);

  // I build the star field here then shove it into spriteSF.  spriteSF_Base is wider so
  //  it cuts off 20 columns on the right side. This lets me create stars (circles) on
  //  the right and have them scroll onto screen more naturally vs. having them pop up
  //  fully formed.
  spriteSF.createSprite(spriteBG.width() / 2 - 15, dispLine6);
  spriteSF_Base.createSprite(spriteSF.width() + 20, spriteSF.height());

  spriteSF_Base.fillSprite(TFT_BLACK);
  count = 56; Serial.printf("Creating up to %i stars.\r\n", count);
  for (int i = 0; i < count; i++) {  // Initial star field.  Up to "count" stars.
    brightness = random(155) + 100;  // Brightness range from 100 to 255.
    // Note:
    // I took out the final color so it will use whatever is around the dot to be put in.
    // That means that the alpha blend will use whatever is there instead of forcing black.
    // If a new start goes in right next to another one, it will see some white as well as
    //  some black.  As it should be!
    //
    // Also note:
    // I made the spriteSF_Base wider than the spriteSF (SF stands for star field). Then
    //  I make stars on the right end of spriteSF_Base but they can't be seen right away.
    // Later, they scroll into view as if appearing in a window, as it should be.  The
    //  same effect happens when a star scrolls out from behind the disk of the moon.  It
    //  is occluded behind the moon then scrolls back into view on the left side of the
    //  disk as the star field scrolls left.  As it should be!
    //    spriteSF_Base.fillSmoothCircle(random(spriteSF_Base.width()),
    //                                   random(spriteSF.height()), random(3),
    //                                   RGB565(brightness, brightness, brightness));

    // colorChoice = 0, nothing
    // colorChoice > 0 && colorChoice < 5, white - 4 chances
    // colorChoice > 4 && colorChoice < 7, blue - 2 chances
    // colorChoice = 7, red = 1 chance
    // This gives a ratio of 4 white to 2 blue to 1 red.

    colorChoice = random(8);
//    Serial.printf("colorChoice %i\r\n", colorChoice);

    if (colorChoice > 0 && colorChoice < 5) {
      spriteSF_Base.fillSmoothCircle(random(spriteSF_Base.width()),
                                     random(spriteSF_Base.height()), random(3),
                                     RGB565(brightness, brightness, brightness));
//      Serial.println("Made a white/grey circle.");
    }
    else if (colorChoice > 4 && colorChoice < 7) {
      spriteSF_Base.fillSmoothCircle(random(spriteSF_Base.width()),
                                     random(spriteSF_Base.height()), random(3),
                                     RGB565(204, 211, 255));  // Little bit Blue.
      //                             RGB565(177, 185, 255));
//      Serial.println("Made a blue circle.");
    }
    else { // if (colorChoice == 7) {
      spriteSF_Base.fillSmoothCircle(random(spriteSF_Base.width()),
                                     random(spriteSF_Base.height()), random(3),
                                     RGB565(255, 211, 204));  // Little bit Red.
      //                             RGB565(225, 157, 157));
//      Serial.println("Made a red circle.");
    }
  }

  spriteMoonInvis.createSprite(100, 100);

  spriteMenu.createSprite(tft.width(), SPR_MENU_HEIGHT);
  spriteMenu.setSwapBytes(true);
  //  spriteMenu.setSwapBytes(false);
  spriteMenu.setTextDatum(TL_DATUM);

  ledcAttach(TFT_BL, 5000, 8);  // PWM timer automatically assigned.
  ledcWrite(TFT_BL, 200);       // Turn the display on bigly for init messages.

  tft.setTextDatum(TC_DATUM);
  tft.fillScreen(TFT_BLACK);
  tft.drawString("Moon Phase v" + sVer, tft.width() / 2, dispLine2, 4);
  tft.drawString("Awaiting WiFi connection", tft.width() / 2, dispLine4, 4);
  tft.drawString("to " + String(chSSID), tft.width() / 2, dispLine5, 4);
  size_t bytesFree = heap_caps_get_free_size(MALLOC_CAP_8BIT | MALLOC_CAP_SPIRAM);
  Serial.printf("memReport(): there are %zu MALLOC_CAP_8BIT | "
                "MALLOC_CAP_SPIRAM bytes free.\r\n", bytesFree);
  Serial.printf("Connecting to WiFi %s ...", chSSID);
  WiFi.begin(chSSID, chPassword); delay(1500);
  while (WiFi.status() != WL_CONNECTED)  // Wait for a connection.
  {
    delay(500);
    Serial.print(".");
  }
  tft.fillScreen(TFT_BLACK);
  tft.drawString("Moon Phase v" + sVer, tft.width() / 2, dispLine2, 4);
  stringIP = WiFi.localIP().toString();
  tft.drawString("WiFi connected to:", tft.width() / 2, dispLine4, 4);
  tft.drawString(stringIP, tft.width() / 2, dispLine5, 4);
  Serial.printf("\r\nWiFi connected to %s at %s\r\n", chSSID, stringIP);
  tft.drawString("Awaiting correct time...", tft.width() / 2, dispLine6, 4);
  initTime();

  //  setHourBrightness();
  //  Serial.printf("Setup setting brightness level for hour %i of %i\r\n",
  //                iHour, tftBL_Lvl);

  tft.setTextPadding(0);
  tft.setTextDatum(TL_DATUM);
  //  ledcWrite(TFT_BL, tftBL_Lvl);  // Set the display at default level for operation.

  /* Testing */
  // To clear out all of the hourly brightness values, use the following four.
  //      preferences.begin("Hourlys", RW_MODE);
  //      preferences.clear();
  //      Serial.println("Hourlys entries cleared.");
  //      preferences.end();
  /* End Testing */

  Hemisphere.toLowerCase();
  moonPhasePic[0] = 0;  // Effectively set it to nothing for now.
  GetMS_PA_Value();     // Initial rotation angle from PA and longitude

  Serial.println("Setup is finished.");
}
/*******************************************************************************************/
void loop()
/*******************************************************************************************/
{
  static bool firstPass = true;

  while (Serial.available() > 0)  // Check if data is available to read
    HandleSerialInput();

  CheckButtons();  // See if brightness change or menu is requested.

  SaveOptions();  // Save any changed options every 10 minutes.

  spriteBG.fillRect(0, 0, tft.width(), dispLine6, RGB565(0, 0, 166));
  spriteBG.setTextColor(TFT_WHITE, RGB565(0, 0, 166));

  startMillis = millis();
  //  struct tm * now_local = gmtime(&now);  // Get UTC/GMT. (Thought it was needed. Nope!)
  getLocalTime(&timeinfo);  // This is what I needed.  Local time!

  iSec = timeinfo.tm_sec;
  if (prevSec == iSec) return;  // Wait for the next second to roll around...

  prevSec = iSec;

  iMonth  = timeinfo.tm_mon + 1;
  iDOM    = timeinfo.tm_mday;
  iYear   = timeinfo.tm_year + 1900;
  iMin    = timeinfo.tm_min;
  iHour   = timeinfo.tm_hour;

  if (prevHour != iHour) {
    setHourBrightness();
#if defined DO_DEBUG_PRINTS
    Serial.printf("Current date: %i/%i/%i\r\n", iMonth, iDOM, iYear);
#endif
  }
  if (tftBL_Lvl == 0) {
    prevHour = iHour;
    delay(100);
    if (prevBL_Lvl != 0) {
      prevBL_Lvl = 0;
      tft.fillScreen(TFT_BLACK);
      ledcWrite(TFT_BL, 200);  // Turn on the backlight
      tft.setTextColor(TFT_YELLOW, TFT_BLACK);
      tft.setTextDatum(TC_DATUM);
      if (xIN_RANGE(iHour, SleepHour, WakeupHour)) {
        tft.drawString("Turning off screen", tft.width() / 2, dispLine2, 4);
        tft.drawString("per Preference value.", tft.width() / 2, dispLine4, 4);
      } else {
        tft.drawString("Dark time now...", tft.width() / 2, dispLine2, 4);
        tft.drawString("Turning off", tft.width() / 2, dispLine3, 4);
        tft.drawString("screen.", tft.width() / 2, dispLine4, 4);
      }
      Serial.println("Turning screen off.");
      delay(5000);
      tft.fillScreen(TFT_BLACK);
      ledcWrite(TFT_BL, 0);  // Turn off the backlight
      tft.writecommand(ST7789_DISPOFF);  // Turn off display hardware.
    }
    return;
  } else {  // if tftBL_Lvl > 0
    if (prevBL_Lvl == 0) {  // If we were previously in hardware sleep
      prevBL_Lvl = tftBL_Lvl;           // Remember the change and
      tft.writecommand(ST7789_DISPON);  // turn on display hardware.
    }
  }

  Build_and_Show();

  if (prevHour != iHour) {
    //    setHourBrightness();  // Already done, above.
    if (!firstPass) HourDance();
    firstPass = false;
    prevHour = iHour;
  }
  yield();  // I am getting a phantom Guru Meditation error from buttons. Hoping this helps.
  //  Serial.printf("Loop took %lu ms.\r\n", millis() - startMillis);
}
