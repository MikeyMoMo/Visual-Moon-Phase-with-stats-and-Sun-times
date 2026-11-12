/*******************************************************************************************/
void Build_and_Show()
/*******************************************************************************************/
{
  static int iPrev_PA_min = -1;
  static bool lastFetchFailed = false;
  static int failCount = 0;
  const int  maxFails = 3;
  static int testRotation = 0;
  static int fetchInterval;
  static float prevPAAngle;

  if (iSec % 10 == 2) AddStars();

  // Modulo lanes:
  //
  // (iMin % 5) == 0 -> 00,05,10,15,20,25,30,35,40,45,50,55
  // (iMin % 5) == 1 -> 01,06,11,16,21,26,31,36,41,46,51,56
  // (iMin % 5) == 2 -> 02,07,12,17,22,27,32,37,42,47,52,57
  // (iMin % 5) == 3 -> 03,08,13,18,23,28,33,38,43,48,53,58
  // (iMin % 5) == 4 -> 04,09,14,19,24,29,34,39,44,49,54,59
  //
  // Using lane 2 so the weather fetch never collides with the
  //  top-of-hour processing.

  // Case 1: Normal 5‑minute fetch
  if (!lastFetchFailed && (iMin % 5 == 2) && (iPrev_PA_min != iMin)) {
    Serial.printf("Attempting PA fetch at %02d:%02d:%02d...\r\n", iHour, iMin, iSec);
    if (GetMS_PA_Value()) {
      Serial.println(". Fetch succeeded.");
      lastFetchFailed = false;
      failCount = 0;
      iPrev_PA_min = iMin;
      Serial.printf("%02i:%02i - Moon Alt %.2f, Moon Az %.2f, Moon Angle %.2f°\r\n",
                    iHour, iMin, MS.moon_altitude, MS.moon_azimuth, MS.moon_angle);
    } else {
      Serial.println(". Fetch failed. Will retry each minute.");
      lastFetchFailed = true;
      failCount = 1;
      iPrev_PA_min = iMin;
    }
  }

  // Case 2: Retry after a failure — on next minute change
  else if (lastFetchFailed && (iPrev_PA_min != iMin)) {
    Serial.printf("Retry attempt %d at %02d:%02d...\r\n", failCount, iHour, iMin);
    if (GetMS_PA_Value()) {
      Serial.printf(" Retry %d succeeded.\r\n", failCount);
      lastFetchFailed = false;
      failCount = 0;
      iPrev_PA_min = iMin;
    } else {
      Serial.println("Retry failed.");
      failCount++;
      iPrev_PA_min = iMin;
      if (failCount >= maxFails) {
        Serial.println("Max retries reached. Waiting until next 10‑minute mark.");
        lastFetchFailed = false;
        failCount = 0;
      }
    }
  }

  spriteSF_Base.pushToSprite(&spriteSF, 22, 22);
  spriteMoonInvis.fillSprite(TFT_BLACK);

  // Just for reference use.
  // struct MoonOrientation {
  //   int adjustedAngle;   // normalized angle for text labels and filenames
  //   int rotationAngle;   // rotation for pushRotated()
  // };

  if (showMoon) {
    time_t epoch = get_epoch_time();
    moonData = moonPhase.getPhase();
    // Serial.printf("Moon angle before AdjustedMoonAngle %.2f\r\n", moonData.angleDeg);
    MoonOrientation orient = AdjustedMoonAngle(moonData.angleDeg, latitude, Hemisphere);
    // Serial.printf("Moon angle after  AdjustedMoonAngle %.2f\r\n", orient.adjustedAngle);

    sprintf(moonPhasePic, "/m%03i.jpg", orient.adjustedAngle);  // Full brightness

    Phase = orient.adjustedAngle;  // Used elsewhere
    targetSprite = &spriteMoonInvis;
    JPB_RC = TJpgDec.drawFsJpg(0, 0, moonPhasePic, LittleFS);
    if (JPB_RC != JDR_OK)
      Serial.printf("Picture %s load failed with error %i\r\n", moonPhasePic, JPB_RC);

    // Draw circle if invisible
    if (iIN_RANGE(orient.adjustedAngle, 328, 32))
      spriteMoonInvis.drawCircle(50, 50, 38, 0x7BCF);

    // Push rotated moon sprite
    // spriteMoonInvis.pushRotated(&spriteSF, orient.rotationAngle, TFT_BLACK);
    spriteMoonInvis.pushRotated(&spriteSF, moonRotationAngle, TFT_BLACK);
  }
  spriteSF.pushToSprite(&spriteBG, 0, 0);

  spriteBG.setTextColor(TFT_WHITE, SpriteDarkBlueBG);

  // Time
  strftime(chBuffer, sizeof(chBuffer), "%T", &timeinfo);
  spriteBG.drawString("Time:",  spriteBG.width() / 2 - 11, dispLine1, 4);
  spriteBG.drawString(chBuffer, spriteBG.width() / 2 + 55, dispLine1, 4);

  // Date
  strftime(chBuffer, sizeof(chBuffer), "%D", &timeinfo);
  spriteBG.drawString("Date:",  spriteBG.width() / 2 - 11, dispLine2, 4);
  spriteBG.drawString(chBuffer, spriteBG.width() / 2 + 55, dispLine2, 4);

  // Moon Rise / Set
  time(&now);
  mr.calculate(latitude, longitude, now);

  if (mr.riseTime < now) {
    mr.calculate(latitude, longitude, now + 86400);  // Check tomorrow
    moonTimes = localtime(&mr.riseTime);
    strftime(chBuffer, sizeof(chBuffer), "%T", moonTimes);

    // If the rise time is more than a day away, I change the label and value to yellow.
    // I think it is obvious enough that it is tomorrow so I dropped off the red coloration.

    if ((mr.riseTime - now) > 86400)
      //      spriteBG.setTextColor(TFT_RED, RGB565(0, 0, 166));
      //    else
      spriteBG.setTextColor(TFT_YELLOW, RGB565(0, 0, 166));
    mr.calculate(latitude, longitude, now);  // Reset times to today
  } else {
    moonTimes = localtime(&mr.riseTime);
    strftime(chBuffer, sizeof(chBuffer), "%T", moonTimes);
  }
  spriteBG.drawString("Rise:",  spriteBG.width() / 2 - 11, dispLine3, 4);
  spriteBG.drawString(chBuffer, spriteBG.width() / 2 + 55, dispLine3, 4);
  spriteBG.setTextColor(TFT_WHITE, RGB565(0, 0, 166));

  if (mr.setTime < now) {
    mr.calculate(latitude, longitude, now + 86400);  // Check tomorrow
    moonTimes = localtime(&mr.setTime);
    strftime(chBuffer, sizeof(chBuffer), "%T", moonTimes);

    // If the rise time is more than a day away, I change the label and value to yellow.
    // I think it is obvious enough that it is tomorrow so I dropped off the red coloration.

    if ((mr.setTime - now) > 86400)
      //      spriteBG.setTextColor(TFT_RED, RGB565(0, 0, 166));
      //    else
      spriteBG.setTextColor(TFT_YELLOW, RGB565(0, 0, 166));
    mr.calculate(latitude, longitude, now);  // Reset times to today
  } else {
    moonTimes = localtime(&mr.setTime);
    strftime(chBuffer, sizeof(chBuffer), "%T", moonTimes);
  }
  spriteBG.drawString("Set:",   spriteBG.width() / 2 - 11, dispLine4, 4);
  spriteBG.drawString(chBuffer, spriteBG.width() / 2 + 55, dispLine4, 4);
  spriteBG.setTextColor(TFT_WHITE, RGB565(0, 0, 166));

  // Visible
  spriteBG.drawString("Visible:", spriteBG.width() / 2 - 11, dispLine5, 4);
  spriteBG.drawString(mr.isVisible ? "Yes" : "No", spriteBG.width() / 2 + 78, dispLine5, 4);

  // Phase text
  spriteBG.fillRect(0, dispLine6, tft.width(), tft.height(), SpriteDarkBlueBG);
  spriteBG.setTextColor(TFT_YELLOW, SpriteDarkBlueBG);
  if (millis() > BLChangeMillis + showPhaseDelay) {
    // Format into buffer for drawString
    char illumBuffer[16];
    snprintf(illumBuffer, sizeof(illumBuffer), " (%.1f%%)", abs(illumPercent));
    String phaseText = MoonPhaseLabel(moonPhase.getPhase().angleDeg);
    //    phaseText += " ";
    phaseText += illumBuffer;
    spriteBG.drawString(phaseText, 6, dispLine6 + 5, 4);
  } else {
    tft.setTextDatum(TL_DATUM);
    spriteBG.setTextPadding(tft.width());
    spriteBG.drawString(chBLChange, 6, dispLine6 + 5, 4);
    spriteBG.setTextPadding(0);
  }

  targetSprite = &spriteAR;
  JPB_RC = TJpgDec.drawFsJpg(0, 0, "/20x20 Arrow.jpg", LittleFS);
  if (JPB_RC != JDR_OK)
    Serial.printf("Arrow load failed with error %i\r\n", JPB_RC);
  spriteAR.setPivot(spriteAR.width() / 2, spriteAR.height() / 2);
  spriteAR.pushRotated(&spriteARot, MS.moon_azimuth, SpriteDarkBlueBG);
  spriteARot.pushToSprite(&spriteBG, 280, 105);

  spriteBG.pushSprite(0, 0);  // Show the finished form of the display sprite.
}
/*******************************************************************************************/
MoonOrientation AdjustedMoonAngle(int rawAngle, float latitude, String hemisphere)
/*******************************************************************************************/
{
  MoonOrientation result;

  if (hemisphere == "south") {
    rawAngle = 360 - rawAngle;
    if (rawAngle == 360) rawAngle = 0;
  }

  rawAngle &= ~1;  // Align to 2° increments

  result.adjustedAngle = rawAngle;
  return result;
}
/*******************************************************************************************/
bool iInRange (int v, int low, int high)  // Inclusive
/*******************************************************************************************/
{
  return (low <= high) ? (v >= low && v <= high) : (v >= low || v <= high);
}
/***************************************************************************/
bool xInRange (int v, int low, int high)  // Exclusive
/***************************************************************************/
{
  return (low <= high) ? (v > low && v < high) : (v > low || v < high);
}
/*******************************************************************************************/
void ShowFuturePhases()
/*******************************************************************************************/
{
  int useLine = dispLine1;
  int hoursChecked = 0;

  bool NM_Done = false;
  bool FQ_Done = false;
  bool FM_Done = false;
  bool LQ_Done = false;

  tft.setTextDatum(TL_DATUM);
  tft.fillScreen(TFT_BLACK);
  tft.fillRect(0, 0, tft.width(), dispLine5, RGB565(150, 53, 73));
  time_t epoch = get_epoch_time();

  tft.setTextColor(TFT_WHITE, RGB565(150, 53, 73));
  // Serial.println("Get phase dates.");

  Serial.println("Times could be +/- 15 minutes.");

  while (!(NM_Done && FQ_Done && FM_Done && LQ_Done) &&
         hoursChecked < 3000)
  {
    //    Serial.println(epoch);
    moonData = moonPhase.getPhase(epoch);
    angleExact = moonData.angleExact;

    if (Hemisphere == "south") {  // Done here to avoid the 2º alignment in AdjustedMoonAngle
      angleExact = 360. - angleExact;
      if (angleExact == 360) angleExact = 0;
    }

    if (iIN_RANGE(angleExact, 0., 5.) && !NM_Done) {
      NM_Done = true;
      tft.drawString("New Moon", 4, useLine, 4);
      tmlocalTime = localtime(&epoch);
      strftime(chBuffer, sizeof(chBuffer), "%a %b %d", tmlocalTime);
      tft.drawString(chBuffer, 170, useLine, 4); useLine += font4Height;
      strftime(chBuffer, sizeof(chBuffer), "New Moon\t%c ", tmlocalTime);
      Serial.print(chBuffer);
      Serial.printf("at %.1fº, Lit %.2fº\r\n", angleExact, moonData.amountLit);
      epoch += 550000;  // 24 Hrs * 6 Days * 3600 Seconds/Hr + 1 hour
    }

    if (iIN_RANGE(angleExact, 90., 95.) && !FQ_Done) {
      FQ_Done = true;
      tmlocalTime = localtime(&epoch);
      strftime(chBuffer, sizeof(chBuffer), "%a %b %d", tmlocalTime);
      tft.drawString("First Quarter", 4, useLine, 4);
      tft.drawString(chBuffer, 170, useLine, 4); useLine += font4Height;
      strftime(chBuffer, sizeof(chBuffer), "First Quarter\t%c ", tmlocalTime);
      Serial.print(chBuffer);
      Serial.printf("at %.1fº, Lit %.2fº\r\n", angleExact, moonData.amountLit);
      epoch += 550000;  // 24 Hrs * 6 Days * 3600 Seconds/Hr + 1 hour
    }

    if (iIN_RANGE(angleExact, 180., 185.) && !FM_Done) {
      FM_Done = true;
      tmlocalTime = localtime(&epoch);
      strftime(chBuffer, sizeof(chBuffer), "%a %b %d", tmlocalTime);
      tft.drawString("Full Moon", 4, useLine, 4);
      tft.drawString(chBuffer, 170, useLine, 4); useLine += font4Height;
      strftime(chBuffer, sizeof(chBuffer), "Full Moon\t%c ", tmlocalTime);
      Serial.print(chBuffer);
      Serial.printf("at %.1fº, Lit %.2fº\r\n", angleExact, moonData.amountLit);
      epoch += 550000;  // 24 Hrs * 6 Days * 3600 Seconds/Hr + 1 hour
    }

    if (iIN_RANGE(angleExact, 270., 275.) && !LQ_Done) {
      LQ_Done = true;
      tmlocalTime = localtime(&epoch);
      strftime(chBuffer, sizeof(chBuffer), "%a %b %d", tmlocalTime);
      tft.drawString("Last Quarter", 4, useLine, 4);
      tft.drawString(chBuffer, 170, useLine, 4); useLine += font4Height;
      strftime(chBuffer, sizeof(chBuffer), "Last Quarter\t%c ", tmlocalTime);
      Serial.print(chBuffer);
      Serial.printf("at %.1fº, Lit %.2fº\r\n", angleExact, moonData.amountLit);
      epoch += 550000;  // 24 Hrs * 6 Days * 3600 Seconds/Hr + 1 hour
    }
    hoursChecked++;
    epoch += 900;  // advance one-quarter hour
  }
  // Footer
  tft.setTextColor(TFT_YELLOW, TFT_BLACK);
  tft.drawString("Press either button to return.", 1, dispLine6, 4);
}
/*******************************************************************************************/
void CheckButtons()
/*******************************************************************************************/
{
  int pressLength = 0;
  int prevBL = tftBL_Lvl;

  if ((digitalRead(topButton) == BUTTON_PRESSED) ||
      (digitalRead(bottomButton) == BUTTON_PRESSED)) {
    delay(250);
    if ((digitalRead(topButton) == BUTTON_PRESSED) &&
        (digitalRead(bottomButton) == BUTTON_PRESSED)) {
      Serial.printf("%lu - Both buttons pressed, do menuing.\r\n", millis());
      while ((digitalRead(topButton) == BUTTON_PRESSED) ||
             (digitalRead(bottomButton) == BUTTON_PRESSED));
      doMenu();
      return;
    }
  }

  tft.setTextDatum(TL_DATUM);
  // Allow the user to adjust backlight brightness.
  while ((digitalRead(topButton) == BUTTON_PRESSED) &&
         (tftBL_Lvl <= MAX_BRIGHTNESS))
  {
    delay(200);
    if (digitalRead(bottomButton) == BUTTON_PRESSED) {
      Serial.printf("%lu - Both buttons pressed, do menuing.\r\n", millis());
      doMenu();
      return;
    }

    // Increase.
    pressLength++;
    if (pressLength > 10) BLchange = 8;
    else if (pressLength > 5) BLchange = 4;
    else BLchange = 2;
    tftBL_Lvl += BLchange;
    if (prevBL == 0) {
      tft.writecommand(ST7789_DISPON);  // Turn on display hardware.
      Serial.println("Turning on display");
      delay(100);
      prevBL = tftBL_Lvl;
    }
    if (tftBL_Lvl > MAX_BRIGHTNESS)
      tftBL_Lvl = MAX_BRIGHTNESS;
    ledcWrite(TFT_BL, tftBL_Lvl);
    BLChangeMillis = millis();
    sprintf(chBLChange, "Brightness: %i", tftBL_Lvl);
    // Serial.println(chBLChange);
    Build_and_Show();
  }

  while ((digitalRead(bottomButton)) == BUTTON_PRESSED &&
         (tftBL_Lvl >= MIN_BRIGHTNESS))
  {
    delay(200);
    if (digitalRead(topButton) == BUTTON_PRESSED) {
      Serial.printf("%lu - Both buttons pressed, do menuing.\r\n", millis());
      doMenu();
      return;
    }

    // Decrease.
    pressLength++;
    if (pressLength > 10) BLchange = 8;
    else if (pressLength > 5) BLchange = 4;
    else BLchange = 2;
    tftBL_Lvl -= BLchange;

    if (tftBL_Lvl < MIN_BRIGHTNESS) tftBL_Lvl = MIN_BRIGHTNESS;

    ledcWrite(TFT_BL, tftBL_Lvl);
    BLChangeMillis = millis();
    if (tftBL_Lvl == MIN_BRIGHTNESS && prevBL > MIN_BRIGHTNESS) {
      tft.fillScreen(TFT_BLACK);
      tft.setTextColor(TFT_YELLOW, TFT_BLACK);
      tft.setTextDatum(TC_DATUM);
      ledcWrite(TFT_BL, 200);
      tft.drawString("User commanded to", tft.width() / 2, dispLine2, 4);
      tft.drawString("turn off screen.", tft.width() / 2, dispLine4, 4);
      Serial.println("User commanded screen off.");
      delay(3000);
      ledcWrite(TFT_BL, tftBL_Lvl);
      tft.writecommand(ST7789_DISPOFF);  // Turn off display hardware.
      delay(100);
    } else {
      sprintf(chBLChange, "Brightness: %i", tftBL_Lvl);
      // Serial.println(chBLChan
      Build_and_Show();
    }
    prevBL = tftBL_Lvl;
  }
}
/*******************************************************************************************/
void doMenu()
/*******************************************************************************************/
{
  int highlight = 1;
  tft.fillScreen(TFT_BLACK);

  // Wait for unpress of both buttons
  while ((digitalRead(topButton) == BUTTON_PRESSED) ||
         (digitalRead(bottomButton) == BUTTON_PRESSED));

  menuHide = millis() + MENU_HIDE_TIME;

  while (millis() < menuHide) {
    // Menu item 1: Show Moon Phase Dates
    spriteMenu.fillSprite(TFT_BLACK);
    spriteMenu.setTextColor(highlight == 1 ? TFT_BLACK : TFT_YELLOW,
                            highlight == 1 ? TFT_YELLOW : TFT_BLACK);
    spriteMenu.drawString("Show Moon Phase Dates", 0, 10, 4);
    // Serial.println("Show Moon Phase Dates");
    spriteMenu.drawFastHLine(0, SPR_MENU_HEIGHT - 1, tft.width(), TFT_YELLOW);
    spriteMenu.pushSprite(0, 10);

    // Menu item 2: Show Sun Rise/Set Times
    spriteMenu.fillSprite(TFT_BLACK);
    spriteMenu.setTextColor(highlight == 2 ? TFT_BLACK : TFT_YELLOW,
                            highlight == 2 ? TFT_YELLOW : TFT_BLACK);
    // Serial.println("Show Sun Rise/Set Times");
    spriteMenu.drawString("Show Sun Rise/Set Times", 0, 10, 4);
    spriteMenu.drawFastHLine(0, SPR_MENU_HEIGHT - 1, tft.width(), TFT_YELLOW);
    spriteMenu.pushSprite(0, 50);

    // Menu item 3: Show/Hide Moon
    spriteMenu.fillSprite(TFT_BLACK);
    spriteMenu.setTextColor(highlight == 3 ? TFT_BLACK : TFT_YELLOW,
                            highlight == 3 ? TFT_YELLOW : TFT_BLACK);
    sprintf(chBuffer, "%s Moon (m%03i.jpg)", showMoon ? "Hide" : "Show", Phase);
    // Serial.println(chBuffer);
    spriteMenu.drawString(chBuffer, 0, 10, 4);
    spriteMenu.drawFastHLine(0, SPR_MENU_HEIGHT - 1, tft.width(), TFT_YELLOW);
    spriteMenu.pushSprite(0, 90);

    // Menu item 4: Return
    spriteMenu.fillSprite(TFT_BLACK);
    spriteMenu.setTextColor(highlight == 4 ? TFT_BLACK : TFT_YELLOW,
                            highlight == 4 ? TFT_YELLOW : TFT_BLACK);
    // Serial.println("Return");
    spriteMenu.drawString("Return", 0, 10, 4);
    spriteMenu.drawFastHLine(0, SPR_MENU_HEIGHT - 1, tft.width(), TFT_YELLOW);
    spriteMenu.pushSprite(0, 130);

    // Handle bottom button (move highlight)
    if (digitalRead(bottomButton) == BUTTON_PRESSED) {
      delay(50);
      if (digitalRead(bottomButton) == BUTTON_PRESSED) {
        highlight++;
        if (highlight > 4) highlight = 1;
        menuHide = millis() + MENU_HIDE_TIME;
      }
      while (digitalRead(bottomButton) == BUTTON_PRESSED);
    }
    // Handle top button (execute selection)
    if (digitalRead(topButton) == BUTTON_PRESSED) {
      delay(50);
      if (digitalRead(topButton) == BUTTON_PRESSED) {
        while (digitalRead(topButton) == BUTTON_PRESSED);

        switch (highlight) {
          case 1:  // Show Moon Phase Dates
            ShowFuturePhases();  // now uses AdjustedMoonAngle + MoonPhaseLabel
            while ((digitalRead(topButton) == BUTTON_NOT_PRESSED) &&
                   (digitalRead(bottomButton) == BUTTON_NOT_PRESSED));
            while ((digitalRead(topButton) == BUTTON_PRESSED) ||
                   (digitalRead(bottomButton) == BUTTON_PRESSED));
            return;

          case 2:  // Show Sun Rise/Set Times
            ShowSunTimes();
            while ((digitalRead(topButton) == BUTTON_NOT_PRESSED) &&
                   (digitalRead(bottomButton) == BUTTON_NOT_PRESSED));
            while ((digitalRead(topButton) == BUTTON_PRESSED) ||
                   (digitalRead(bottomButton) == BUTTON_PRESSED));
            return;

          case 3:  // Toggle moon visibility
            showMoon = !showMoon;
            Serial.printf("Moon will%s be shown.\r\n", showMoon ? "" : " not");
            while ((digitalRead(topButton) == BUTTON_PRESSED) ||
                   (digitalRead(bottomButton) == BUTTON_PRESSED));
            return;

          case 4:  // Return
          default:
            return;
        }
      }
      menuHide = millis() + MENU_HIDE_TIME;
    }
  }
}
/*******************************************************************************************/
void setHourBrightness()
/*******************************************************************************************/
{
  preferences.begin("Hourlys", RO_MODE);

  // If this hour has not been set, 1000 will be returned.  If it is
  //  and it is late, turn off the display.  It can be turned back on
  //  if anyone is awake and want to set it to be on for the hour.
  sprintf(chHour, "%d", iHour);
  tftBL_Lvl = preferences.getInt(chHour, 1000);  // Never been set?
  Serial.printf("%02i:%02i:%02i - Read brightness level for hour %i of %i\r\n",
                iHour, iMin, iSec, iHour, tftBL_Lvl);
  preferences.end();

  //  Serial.printf("Calling xIN_RANGE with %i, %i, %i\r\n", iHour, SleepHour, WakeupHour);
  SleepTime = xIN_RANGE(iHour, SleepHour, WakeupHour);
  //  if (WakeupHour > SleepHour)
  //    SleepTime = (iHour >= WakeupHour || iHour <= SleepHour);
  //  else
  //    SleepTime = (iHour >= WakeupHour && iHour <= SleepHour);
#if defined DO_DEBUG_PRINTS
  Serial.print("In sleep time? ");
  Serial.println(SleepTime ? "Yes" : "No");
#endif
  // If unchanged value still in there, pick default value.
  // If less than 500 then user set a value, let it be.
  if (tftBL_Lvl > 500) {  // Is it unchanged?
    if (SleepTime)
      tftBL_Lvl = 0;
    else
      tftBL_Lvl = defaultBright;
    // Serial.printf("tftBL_Lvl now set to %i\r\n", tftBL_Lvl);
  }

  ledcWrite(TFT_BL, tftBL_Lvl);  // Activate whatever was decided on.
  Serial.printf("%02i:%02i:%02i - Brightness level for hour %i set to %i\r\n",
                iHour, iMin, iSec, iHour, tftBL_Lvl);
}
/*******************************************************************************************/
void SaveOptions()
/*******************************************************************************************/
{
  // On the modulo 10 minute, see if saving the brightness value is needed.
  // Also on the next to last second of the hour.
  if ((iMin > 0 && iMin % 10 == 0 && iSec == 0) ||
      (iMin == 59 && iSec == 58)) {
    preferences.begin("Hourlys", RW_MODE);
    sprintf(chHour, "%d", iHour);
    nTemp = preferences.getInt(chHour, defaultBright);
    if (nTemp != tftBL_Lvl) {
      preferences.putInt(chHour, tftBL_Lvl);
      Serial.printf("%02i:%02i:%02i - Saved BL level for hour %02i: %02i\r\n",
                    iHour, iMin, iSec, iHour, tftBL_Lvl);
    }
    preferences.end();
  }
}
/*******************************************************************************************/
void printVers()
/*******************************************************************************************/
{
  int      lastDot, lastV;
  String   sTemp;

  // Serial.println(__FILENAME__);  // Same as __FILE__
  sTemp = String(__FILE__);
  // Get rid of the trailing .ino tab name. In this case, "\Utilities.ino"
  sTemp = sTemp.substring(0, sTemp.lastIndexOf("\\"));
  Serial.print("Running from: "); Serial.println(sTemp);

  sTemp = String(__FILE__);  // Start again for the version number.
  lastDot = sTemp.lastIndexOf(".");
  if (lastDot > -1) {  // Found a dot.  Thank goodness!
    lastV = sTemp.lastIndexOf("v");  // Find start of version number
    if (lastV > -1) {  // Oh, good, found version number, too
      sVer = sTemp.substring(lastV + 1, lastDot); // Pick up version number
      lastV = sVer.lastIndexOf("\\");
      if (lastV > -1) sVer = sVer.substring(0, lastV);
    } else {
      sVer = "0.00";  // Unknown version.
    }
  } else {
    sVer = "n/a";  // Something badly wrong here!
  }
  Serial.print("Version " + sVer + ", ");
  Serial.printf("Compiled on %s at %s.\r\n", __DATE__, __TIME__);
}
/*******************************************************************************************/
void AddStars()
/*******************************************************************************************/
{
  static int timeForStars = 0;

  // y value is optional (default is 0, so no up/down scrolling here.
  // We stand on our rights!).
  spriteSF_Base.scroll(-1);  // Scroll the entire starfield left by one pixel.

  // Was 3 then 1, now 2.  Trying to get the right starfield look.
  if (timeForStars++ == 2) {
    timeForStars = 0;
    // Redraw new stars appearing on the right

    // Notes on the following for loop:
    // I took out the final color so it will use whatever is around the dot to be put in.
    // That means that the alpha blend will use whatever is there instead of forcing black.
    // If a new star goes in right next to another one, it will see some white as well as
    //  some black.  So alpha blend can do its job properly. As it should be!
    //
    // Also note:
    // I made the spriteSF_Base wider than the spriteSF (SF stands for star field). Then
    //  I make new stars on the right side of spriteSF_Base but they can't be seen yet.
    // Later, they scroll into view as if appearing in a window, as it should be.  The
    //  same effect happens when a star scrolls out from behind the disk of the moon.  It
    //  is occluded behind the moon then scrolls back into view on the left side of the
    //  disk as the star field scrolls left.  As it should be!
    count = random(3);  // 0, 1 or 2 stars will be created this pass.
    for (int i = 0; i < count; i++) {  // Few new stars each frame (maybe)
      brightness = random(155) + 100;   // Brightness range from 100 to 255.
      // Serial.printf("Brightness %i ", brightness);

      // colorChoice = 0, nothing
      // colorChoice > 0 && colorChoice < 5, white - 4 chances
      // colorChoice > 4 && colorChoice < 7, blue - 2 chances
      // colorChoice = 7, red = 1 chance
      // This gives a ratio of 4 white to 2 blue to 1 red.

      colorChoice = random(8);
      //      Serial.printf("colorChoice %i\r\n", colorChoice);

      // In the following, the horizontal placement is width - 4 so that stars won't be
      //  placed hanging off of the edge of the sprite and, therefore, coming out with
      //  their right edge being a straight line.  That's not nice!
      //  I prefer round circles.  They're the best kind!

      if (colorChoice > 0 && colorChoice < 5) {
        spriteSF_Base.fillSmoothCircle(spriteSF_Base.width() - 4,  // random(3) is size...
                                       random(spriteSF_Base.height()), random(3),
                                       RGB565(brightness, brightness, brightness));
      }
      if (colorChoice > 4 && colorChoice < 7) {  // A 2 or 3 will put in a blue
        spriteSF_Base.fillSmoothCircle(spriteSF_Base.width() - 4,  // could be 0...
                                       random(spriteSF_Base.height()), random(3),
                                       RGB565(204, 211, 255));  // Little bit Blue.
      }
      else if (colorChoice == 7) {  // Even more rare, a 3 making a redish/pinkish dot
        spriteSF_Base.fillSmoothCircle(spriteSF_Base.width() - 4,  // then no star!
                                       random(spriteSF_Base.height()), random(3),
                                       RGB565(255, 211, 204));  // Little bit Red.
      }
    }
  }
}
/*******************************************************************************************/
void HandleSerialInput()
/*******************************************************************************************/
{
  if (!Serial.available()) return;

  int i, iHTemp;

  char input = Serial.read();
  input = toupper(input);

  // Ignore carriage return and newline entirely
  if (input == '\r' || input == '\n') return;

  Serial.printf("User input %c\r\n", input);

  switch (input) {
    case 'D':
      Serial.printf("Initial tftBL_Lvl is %d\r\n", tftBL_Lvl);
      if (tftBL_Lvl == 0) {
        Serial.println("Turning screen on.");
        tft.writecommand(ST7789_DISPON);  // Turn on display hardware.
        sprintf(chHour, "%d", iHour);
        preferences.begin("Hourlys", RO_MODE);
        // If this hour has not been set, 1000 will be returned.  If it is
        //  and it is late, turn off the display.  It can be turned back on
        //  if anyone is awake and want to set it to be on for the hour.
        tftBL_Lvl = preferences.getInt(chHour, 1000);  // Never been set?
        preferences.end();
        Serial.printf("Brightness value for hour %d is %d\r\n",
                      iHour, tftBL_Lvl);
        if (tftBL_Lvl > 500) {  // Is it unchanged?
          tftBL_Lvl = defaultBright;
        }
        prevBL_Lvl = tftBL_Lvl;
        ledcWrite(TFT_BL, tftBL_Lvl);  // Turn on the backlight
      } else {
        tft.fillScreen(TFT_BLACK);
        ledcWrite(TFT_BL, defaultBright);  // Activate whatever was decided on.
        delay(10);
        tft.setTextColor(TFT_YELLOW, TFT_BLACK);
        tft.setTextDatum(TC_DATUM);
        tft.drawString("By User request,", tft.width() / 2, dispLine2, 4);
        tft.drawString("turning off", tft.width() / 2, dispLine3, 4);
        tft.drawString("screen.", tft.width() / 2, dispLine4, 4);
        Serial.println("Turning screen off.");
        delay(3000);
        tft.fillScreen(TFT_BLACK);
        ledcWrite(TFT_BL, 0);  // Turn off the backlight
        tftBL_Lvl = 0;
        prevBL_Lvl = tftBL_Lvl;
        tft.writecommand(ST7789_DISPOFF);  // Turn off display hardware.
      }
      showInputOptions();
      break;

    case 'B':
      preferences.begin("Hourlys", RO_MODE);
      for (i = 0; i < 24; i++) {
        sprintf(chHour, "%d", i);
        iHTemp = preferences.getInt(chHour, 1000);  // Never been set?
        Serial.printf("Brightness level for hour %2i is %4i\r\n", i, iHTemp);
      }
      preferences.end();
      showInputOptions();
      break;

    case 'P':
      Serial.printf("Picture loaded m%i.jpg\r\n", Phase);
      break;

    case 'M':
      showMoon = !showMoon;
      break;

    case '?':
      showInputOptionsFull();
      break;

    default:
      Serial.printf("*>> Unknown input '%c'!\r\n", input);
      showInputOptionsFull();
      break;
  }
}
/*******************************************************************************************/
void showInputOptions()
/*******************************************************************************************/
{
  Serial.println("Enter ? to see the full list of options.");
}
/*******************************************************************************************/
void showInputOptionsFull()
/*******************************************************************************************/
{
  Serial.println("Moon Phase & Sun Times, v" + sVer);
  Serial.println("Running from:"); Serial.println(__FILE__);
  Serial.print("Compiled on "); Serial.print(__DATE__);
  Serial.print(" at "); Serial.println(__TIME__);
  Serial.println("Enter D to turn on the display at default brightness or off.");
  Serial.println("Enter B (testing) to see all Hourly brightness values on the Monitor.");
  Serial.println("Enter P (testing) to see loaded picture name (Info).");
  Serial.println("Enter M (testing) to hide/show the moon image.");
  Serial.println("Enter ? for this list.  Upper or Lower case OK.");
  Serial.println("If you make an unknown entry, this list is printed on the monitor.\r\n");
}
/*******************************************************************************************/
void ShowSunTimes()
/*******************************************************************************************/
{
  static time_t riseEpoch, setEpoch, upTime;
  static time_t dayLength;
  static int    iHour, iMin, iSec;
  static int    hours;
  static int    minutes;
  static int    seconds;
  static char   buf[100];

  tft.setTextDatum(TL_DATUM);
  tft.fillScreen(TFT_BLACK);
  tft.fillRect(0, 0, tft.width(), dispLine5, RGB565(150, 53, 73));
  time_t epoch = get_epoch_time();
  tft.setTextColor(TFT_WHITE, RGB565(150, 53, 73));

  time(&now);

  sr.calculate(latitude, longitude, now);  // Get all of the answers

  // Rise time
  riseEpoch = sr.riseTime;
  sunTimes = localtime(&riseEpoch);
  strftime(chBuffer, sizeof(chBuffer), "%T", sunTimes);
  tft.drawString("Sunrise at:", 4, dispLine1, 4);
  tft.drawString(chBuffer, 170, dispLine1, 4);
  Serial.printf("Sunrise: %s - ", chBuffer);

  setEpoch = sr.setTime;
  sunTimes = localtime(&setEpoch);
  strftime(chBuffer, sizeof(chBuffer), "%T", sunTimes);
  tft.drawString("Sunset  at:", 4, dispLine2, 4);
  tft.drawString(chBuffer, 170, dispLine2, 4);
  Serial.printf("Sun set: %s\r\n", chBuffer);

  // I know that it is a bit silly to put up an indication of whether the sun is
  //  visible or not, but it was available from the library and I had room left over
  //  so I put it in.  Or, you could just look out the window.  ;-))

  tft.drawString("Visible:", 4, dispLine4, 4);
  if (sr.isVisible)
    tft.drawString("Yes", 170, dispLine4, 4);
  else
    tft.drawString("No",  170, dispLine4, 4);

  // I am being cheated by the call to get these two epochs.  It returns the
  //  closest one and that may be today and it may be tomorrow.  So I just
  //  abs it and call it done.  It might mean that there is a minute difference
  //  from the real day length but with sunset and civil sunset, it is all a bit
  //  of a random time, anyway.  So.... get over it!

  tft.drawString("Day Length:", 4, dispLine3, 4);
  // Replace ':' with 'h ' and append 'm'
  MS.day_length.replace(":", "h ");
  MS.day_length += "m";
  tft.drawString(MS.day_length, 170, dispLine3, 4);

  tft.setTextColor(TFT_YELLOW, TFT_BLACK);
  tft.drawString("Press either button to return.", 1, dispLine6, 4);
}
/*******************************************************************************************/
void initTime()
/*******************************************************************************************/
{
#define NTP_SERVERS "time.google.com", \
  "pool.ntp.org", "216.239.35.0"

  sntp_set_sync_interval(sNTP_UpdTime);  // 1 minute.
  Serial.printf("sNTP_UpdTime now set to %d ms.\r\n", sNTP_UpdTime);
  sntp_set_sync_mode(SNTP_SYNC_MODE_IMMED);
  sntp_set_time_sync_notification_cb(timeSyncCallback);

#if defined(CONFIG_FOR_JOE)
  configTzTime("PST8PDT,M3.2.0,M11.1.0", NTP_SERVERS);
#else
  configTzTime("PHT-8", NTP_SERVERS);
#endif

  Serial.print("Waiting for correct time...\r\n"); delay(5000);
  while (!TS_Epoch); // Wait for time hack.

  time(&UTC);
  strftime(chBuffer, sizeof(chBuffer), "%Y", localtime(&UTC));
  iYear = atoi(chBuffer);
  int iLooper = 0;
  while (iYear < 2025) {
    Serial.printf("iYear %d\r\n", iYear);
    Serial.print(".");
    time(&UTC);
    strftime (chBuffer, 100, "%Y", localtime(&UTC));
    iYear = atoi(chBuffer);
    if (iLooper++ > 30) {
      Serial.println("\r\nCannot get time set. Rebooting.");
      ESP.restart();
      if (iLooper % 5 == 0) {
        Serial.println("Restarting sNTP to issue new request.");
        sntp_restart();
      }
    }
    delay(5000);
  }
  Serial.println();
  spriteBG.setTextDatum(TL_DATUM);
}
/*******************************************************************************************/
void timeSyncCallback(struct timeval * tv)
/*******************************************************************************************/
{
  //  struct timeval {  // Instantiated as "*tv"
  //    Number of whole seconds of elapsed time.
  //   time_t      tv_sec;
  //    Number of microseconds of rest of elapsed time minus tv_sec.
  //     Always less than one million.
  //   long int    tv_usec;
  Serial.print("** > Time Sync Received at ");
  Serial.print(ctime(&tv->tv_sec));  // cTime should not but adds a \n\r
  TS_Epoch = tv->tv_sec;
  Serial.println();  // To make PuTTY happy.

  if (sNTP_UpdTime == sNTP_UpdTime2) {
    sNTP_UpdTime = sNTP_UpdTime3;
    sntp_set_sync_interval(sNTP_UpdTime);  // 1 minute, then 2, then 12 hours.
    // sntp_set_sync_mode(SNTP_SYNC_MODE_SMOOTH);  // Not worth the overhead!
    Serial.printf("sNTP_UpdTime now set to %d ms.\r\n", sNTP_UpdTime);
    esp_sntp_stop(); esp_sntp_init();
  }
  if (sNTP_UpdTime == sNTP_UpdTime1) {
    sNTP_UpdTime = sNTP_UpdTime2;
    sntp_set_sync_interval(sNTP_UpdTime);  // 1 minute, then 2, then 2 hours.
    Serial.printf("sNTP_UpdTime now set to %d ms.\r\n", sNTP_UpdTime);
    esp_sntp_stop(); esp_sntp_init();
  }
}
/*******************************************************************************************/
String MoonPhaseLabel(int angle)
/*******************************************************************************************/
{
  if (iIN_RANGE(angle, 355,   5)) return TXT_MOON_NEW;
  else if (iIN_RANGE(angle,   6,  84)) return TXT_MOON_WAXING_CRESCENT;
  else if (iIN_RANGE(angle,  85,  95)) return TXT_MOON_FIRST_QUARTER;
  else if (iIN_RANGE(angle,  96, 174)) return TXT_MOON_WAXING_GIBBOUS;
  else if (iIN_RANGE(angle, 175, 185)) return TXT_MOON_FULL;
  else if (iIN_RANGE(angle, 186, 264)) return TXT_MOON_WANING_GIBBOUS;
  else if (iIN_RANGE(angle, 265, 275)) return TXT_MOON_THIRD_QUARTER;
  else if (iIN_RANGE(angle, 276, 354)) return TXT_MOON_WANING_CRESCENT;
  return "Unknown Phase (angle=" + String(angle) + ")";
}
/*******************************************************************************************/
// Print image information to the serial port (optional)
/*******************************************************************************************/
void jpegInfo() {
  Serial.println(F("==============="));
  Serial.println(F("JPEG image info"));
  Serial.println(F("==============="));
  Serial.print(F(  "Width      :")); Serial.println(JpegDec.width);
  Serial.print(F(  "Height     :")); Serial.println(JpegDec.height);
  Serial.print(F(  "Components :")); Serial.println(JpegDec.comps);
  Serial.print(F(  "MCU / row  :")); Serial.println(JpegDec.MCUSPerRow);
  Serial.print(F(  "MCU / col  :")); Serial.println(JpegDec.MCUSPerCol);
  Serial.print(F(  "Scan type  :")); Serial.println(JpegDec.scanType);
  Serial.print(F(  "MCU width  :")); Serial.println(JpegDec.MCUWidth);
  Serial.print(F(  "MCU height :")); Serial.println(JpegDec.MCUHeight);
  Serial.println(F("==============="));
}
/*******************************************************************************************/
time_t get_epoch_time()
/*******************************************************************************************/
{
  struct timeval tv;
  gettimeofday(&tv, NULL);
  return tv.tv_sec;
}
/*******************************************************************************************/
void HourDance()
/*******************************************************************************************/
{
  //  Serial.printf("It is now %02i:00\r\n", iHour);

  // Do HourDance after updating the display to xx:00:00
  doThis(4) {
    tft.invertDisplay(false); delay(200);
    tft.invertDisplay(true); delay(200);
  }
}
/*******************************************************************************************/
bool jpg_output_Sprite(int16_t x, int16_t y, uint16_t w, uint16_t h, uint16_t* bitmap)
/*******************************************************************************************/
{
  if (!targetSprite) return 0; // safety check

  // Stop further decoding if image runs off bottom of sprite
  if (y >= targetSprite->height()) return 0;

  // Push block into whichever sprite is currently active
  targetSprite->pushImage(x, y, w, h, bitmap);

  return 1; // continue decoding
}
/*******************************************************************************************/
void debugTime()
/*******************************************************************************************/
{
  Serial.printf("%02i:%02i:%02i - ", iHour, iMin, iSec);
}
/*******************************************************************************************/
bool GetMS_PA_Value()
/*******************************************************************************************/
{
  static bool rc;

  rc = MS.parseMoonStruck(apiKey, latitude, longitude);
  if (rc != 0) return false;

  Serial.printf("Moon P.Angle:  %6.2fº", MS.moon_parallactic_angle);

  moonRotationAngle = MS.moon_parallactic_angle;
  illumPercent = MS.moon_illumination_percentage;
  return true;
}
/*******************************************************************************************/
// Given: latitude (phi), altitude (h), azimuth (A)
// Returns: declination (delta)
float moonDeclination(float latitude, float altitude, float azimuth)
/*******************************************************************************************/
{
  // Convert everything to radians
  static float phi = radians(latitude);
  static float h   = radians(altitude);
  static float A   = radians(azimuth);
  // Formula: sin δ = sin φ * sin h + cos φ * cos h * cos A
  return degrees(asin(sin(phi) * sin(h) + cos(phi) * cos(h) * cos(A)));
}
/*******************************************************************************************/
float maxMoonAltitude(float latitude, float declination, float altitude)
/*******************************************************************************************/
{
  float maxAlt = 90.0 - fabs(latitude - declination);
  // Apply the sign of the current altitude
  return (altitude < 0.0) ? -maxAlt : maxAlt;
}
