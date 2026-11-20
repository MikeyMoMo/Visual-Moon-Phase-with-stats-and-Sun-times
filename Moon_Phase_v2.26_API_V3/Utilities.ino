/*******************************************************************************************/
void Build_and_Show()
/*******************************************************************************************/
{
  static int iPrev_PA_min = -1;
  static bool lastFetchFailed = false;
  static int failCount = 0;
  const int  maxFails = 3;
  //  static int testRotation = 0;
  //  static int fetchInterval;
  //  static float prevPAAngle;
  static float fArrowRot;

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
    // Serial.printf("Attempting PA fetch at %02d:%02d:%02d...\r\n", iHour, iMin, iSec);
    if (GetMoonData()) {
      // Serial.println(". Fetch succeeded.");
      iGotAstronomyNumbers++;
      lastFetchFailed = false;
      failCount = 0;
      iPrev_PA_min = iMin;
      fDeclination = moonDeclination(latitude, moon_altitude, moon_azimuth);
      fMaxAltitude = maxMoonAltitude(latitude, fDeclination);
      fMinAltitude = minMoonAltitude(latitude, fDeclination);
      fArrowRot = ComputeArrow(moon_altitude);
      // debugTime();
      // Serial.printf("MS Returned: MS Moon Alt %.2f, MS Moon Angle %.2f, MS Moon Az %.2f\r\n",
      //               moon_altitude, moon_angle, moon_azimuth);
    } else {
      Serial.println("Fetch of PA failed. Will retry every minute.");
      lastFetchFailed = true;
      failCount = 1;
      iPrev_PA_min = iMin;
    }
  }

  // Case 2: Retry after a failure — on next minute change
  else if (lastFetchFailed && (iPrev_PA_min != iMin)) {
    Serial.printf("Retry attempt %d at %02d:%02d...", failCount, iHour, iMin);
    if (GetMoonData()) {
      Serial.printf("PA Fetch Retry %d succeeded.\r\n", failCount);
      iGotAstronomyNumbers++;
      lastFetchFailed = false;
      failCount = 0;
      iPrev_PA_min = iMin;
      fDeclination = moonDeclination(latitude, moon_altitude, moon_azimuth);
      fMaxAltitude = maxMoonAltitude(latitude, fDeclination);
      fMinAltitude = minMoonAltitude(latitude, fDeclination);
      fArrowRot = ComputeArrow(moon_altitude);
    } else {
      debugTime(); Serial.println("Retry failed.");
      failCount++;
      iPrev_PA_min = iMin;
      if (failCount >= maxFails) {
        Serial.println("\r\nMax retries reached. Waiting until next fetch time.");
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
  // };

  if (showMoon) {
    time_t epoch = get_epoch_time();
    moonData = moonPhase.getPhase();
    // Serial.printf("Moon angle before AdjustedMoonAngle %.2f\r\n", moonData.angleDeg);
    // if (iPrevMin != iMin)
    //   Serial.printf("moonData exact angle %.2f\r\n", moonData.angleExact);
    MoonOrientation orient = AdjustedMoonAngle(moonData.angleDeg, latitude);
    // if (iPrevMin != iMin)
    //   Serial.printf("AdjustedMoonAngle %d, MS angle %.2f\r\n",
    //                 orient.adjustedAngle, moon_angle);
    sprintf(moonPhasePic, "/m%03i.jpg", orient.adjustedAngle);  // Full brightness
    Phase = orient.adjustedAngle;  // Used elsewhere, don't delete
    targetSprite = &spriteMoonInvis;
    JPB_RC = TJpgDec.drawFsJpg(0, 0, moonPhasePic, LittleFS);
    if (JPB_RC != JDR_OK)
      Serial.printf("Picture %s load failed with error %i\r\n", moonPhasePic, JPB_RC);

    // Draw circle if invisible
    if (iIN_RANGE(orient.adjustedAngle, 328, 32))
      spriteMoonInvis.drawCircle(50, 50, 38, 0x7BCF);

    // Push rotated moon sprite
    //    spriteMoonInvis.pushRotated(&spriteSF, moonRotationAngle, TFT_BLACK);
    spriteMoonInvis.pushRotated(&spriteSF, moon_parallactic_angle, TFT_BLACK);
  }
  spriteSF.pushToSprite(&spriteBG, 0, 0);

  spriteBG.setTextColor(TFT_WHITE, SpriteDarkBlueBG);

  // Time
  strftime(chBuffer, sizeof(chBuffer), "%T", &timeinfo);
  spriteBG.drawString("Time:",  149, dispLine1, 4);
  spriteBG.drawString(chBuffer, 215, dispLine1, 4);

  // Date
  strftime(chBuffer, sizeof(chBuffer), "%D", &timeinfo);
  spriteBG.drawString("Date:",  149, dispLine2, 4);
  spriteBG.drawString(chBuffer, 215, dispLine2, 4);

  // Moon Rise / Set
  time(&now);
  mr.calculate(latitude, longitude, now);
  if (iMin % 30 == 0 && iSec == 0) {
    double haDeg = fmod(mr.hourAngle * RAD_TO_DEG, 360.0);
    if (haDeg < 0) haDeg += 360.0;
    // debugTime();
    // Serial.printf("Hour angle %.1f (%.1fº), Halfhour %.1f, nextHour %.1f\r\n",
    //               mr.hourAngle, haDeg, mr.halfHourAngle, mr.nextHourAngle);
  }
  if (mr.riseTime < now) {
    mr.calculate(latitude, longitude, now + 86400);  // Check tomorrow
    moonEventEpoch = localtime(&mr.riseTime);
    strftime(chBuffer, sizeof(chBuffer), "%T", moonEventEpoch);

    // If the rise time is more than a day away, I change the label and value to yellow.
    // I think it is obvious enough that it is tomorrow so I dropped off the red coloration.

    if ((mr.riseTime - now) > 86400)
      spriteBG.setTextColor(TFT_YELLOW, RGB565(0, 0, 166));
    mr.calculate(latitude, longitude, now);  // Reset times to today
  } else {
    moonEventEpoch = localtime(&mr.riseTime);
    strftime(chBuffer, sizeof(chBuffer), "%T", moonEventEpoch);
  }
  spriteBG.drawString("Rise:",  spriteBG.width() / 2 - 11, dispLine3, 4);
  if (moonEventEpoch == 0)
    spriteBG.drawString("--:--:--", spriteBG.width() / 2 + 55, dispLine3, 4);
  else
    spriteBG.drawString(chBuffer, spriteBG.width() / 2 + 55, dispLine3, 4);
  spriteBG.setTextColor(TFT_WHITE, RGB565(0, 0, 166));

  if (mr.setTime < now) {
    mr.calculate(latitude, longitude, now + 86400);  // Check tomorrow
    moonEventEpoch = localtime(&mr.setTime);
    strftime(chBuffer, sizeof(chBuffer), "%T", moonEventEpoch);

    // If the rise time is more than a day away, I change the label and value to yellow.
    // I think it is obvious enough that it is tomorrow so I dropped off the red coloration.

    if ((mr.setTime - now) > 86400)
      //      spriteBG.setTextColor(TFT_RED, RGB565(0, 0, 166));
      //    else
      spriteBG.setTextColor(TFT_YELLOW, RGB565(0, 0, 166));
    mr.calculate(latitude, longitude, now);  // Reset times to today
  } else {
    moonEventEpoch = localtime(&mr.setTime);
    strftime(chBuffer, sizeof(chBuffer), "%T", moonEventEpoch);
  }
  spriteBG.drawString("Set:", 149, dispLine4, 4);
  if (moonEventEpoch == 0)
    spriteBG.drawString("--:--:--", 215, dispLine4, 4);
  else
    spriteBG.drawString(chBuffer, 215, dispLine4, 4);
  spriteBG.setTextColor(TFT_WHITE, RGB565(0, 0, 166));

  // Visible
  snprintf(illumBuffer, sizeof(illumBuffer), "Vis:%s", mr.isVisible ? "Y" : "N");
  spriteBG.drawString(illumBuffer, 149, dispLine5, 4);

  // Phase text
  spriteBG.fillRect(0, dispLine6, tft.width(), tft.height(), SpriteDarkBlueBG);
  spriteBG.setTextColor(TFT_YELLOW, SpriteDarkBlueBG);
  if (millis() > BLChangeMillis + showPhaseDelay) {
    // Format into buffer for drawString
    /*
      if (sscanf(day_length, "%d:%d", &sscanfHours, &sscanfMins) == 2)
        snprintf(chBuffer, sizeof(chBuffer), "%dh %dm", sscanfHours, sscanfMins);
      else
        snprintf(chBuffer, sizeof(chBuffer), "Undefined");
      tft.drawString(chBuffer, 170, dispLine3, 4);

    */
    String phaseText = MoonPhaseLabel(moonData.angleDeg);
    if (iGotAstronomyNumbers > 0) {
      floatwork = strtof(moon_illum_percentage, NULL);
      snprintf(illumBuffer, sizeof(illumBuffer), " (%d%%)", int(floatwork));
      phaseText += illumBuffer;
    }
    spriteBG.drawString(phaseText, 6, dispLine6 + 5, 4);
  } else {
    tft.setTextDatum(TL_DATUM);
    spriteBG.setTextPadding(tft.width());
    spriteBG.drawString(chBLChange, 6, dispLine6 + 5, 4);
    spriteBG.setTextPadding(0);
  }

  targetSprite = &spriteAR;
  //  spriteAR.fillSprite(SpriteDarkBlueBG);  // Dark background blue
  JPB_RC = TJpgDec.drawFsJpg(0, 0, "/20x20 Arrow.jpg", LittleFS);
  if (JPB_RC != JDR_OK)
    Serial.printf("Arrow load failed with error %i\r\n", JPB_RC);
  spriteAR.setPivot(spriteAR.width() / 2, spriteAR.height() / 2);
  // This arrow is not tethered to the standard compass used on the earth's surface.
  // It is a pointer to where the moon around the earth relative to where you are on
  //  the surface of the earth.
  // If it is above the middle, the moon might be visible (sunlight, clouds, etc.).
  // If it is below the middle, the moon is behind the earth from where you are.

  // Don't show the arrow until there are 2 samples for trend and quadrant and angle.
  // Need two altitude samples before rise/fall direction is reliable.  Really, need
  //  a current and previous so the trending direction can be found.
  // Without this delay, the arrow can point into quadrant 0 for up to 10 minutes
  //  during startup because prevAlt is still uninitialized.

  if (iGotAstronomyNumbers > 0) { // If rotation info retrieved and calculated...
    snprintf(illumBuffer, sizeof(illumBuffer), "Dir:%s",
             DegreesToCompass16(moon_azimuth));
    spriteBG.setTextColor(TFT_WHITE, RGB565(0, 0, 166));
    spriteBG.drawString(illumBuffer, 215, dispLine5, 4);
  } else {
    if (iPrevMin != iMin) {
      debugTime();
      Serial.println("Waiting for 1 MS sample to get moon azimuth.");
    }
  }

  if (iGotAstronomyNumbers > 1) { // If rotation info retrieved and calculated...
    spriteARot.fillSprite(SpriteDarkBlueBG);  // Dark background blue
    spriteAR.pushRotated(&spriteARot, fArrowRot, SpriteDarkBlueBG);
    spriteARot.pushToSprite(&spriteBG, 270, dispLine6);
  } else {
    if (iPrevMin != iMin) {
      debugTime();
      Serial.println("Waiting for 2 MS samples to get arrow information & trend");
    }
  }
  spriteBG.pushSprite(0, 0);  // Show the finished form of the display sprite.
}
/*******************************************************************************************/
MoonOrientation AdjustedMoonAngle(int rawAngle, float latitude)
/*******************************************************************************************/
{
  MoonOrientation result;

  if (latitude < 0) {
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

    // Done here to avoid the 2º alignment in AdjustedMoonAngle and it is a float here.
    if (latitude < 0.) {  // Is this in the southern hemisphere?
      angleExact = 360. - angleExact;
      if (angleExact == 360.0) angleExact = 0.0;
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
    // Serial.println("Show Moon ; Dates");
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
    sprintf(chBuffer, "%s Moon (m%03i.jpg)", showMoon ? "Hide" : "Show",
            Phase);
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
            ShowsunEventEpoch();
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
      Serial.printf("%02i:%02i:%02i - Just saved BL level for hour %02i: %02i\r\n",
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
        //        spriteSF_Base.fillSmoothCircle(spriteSF_Base.width() - 4,
        spriteSF_Base.fillSmoothCircle(spriteSF_Base.width() - random(4, 8),
                                       random(spriteSF_Base.height()), random(3),
                                       RGB565(brightness, brightness, brightness));
      }
      if (colorChoice > 4 && colorChoice < 7) {  // A 3 to 6 will put in a blue
        spriteSF_Base.fillSmoothCircle(spriteSF_Base.width() - random(4, 8),
                                       random(spriteSF_Base.height()), random(3),
                                       RGB565(204, 211, 255));  // Little bit Blue.
      }
      else if (colorChoice == 7) {  // Even more rare, a 3 making a redish/pinkish dot
        spriteSF_Base.fillSmoothCircle(spriteSF_Base.width() - random(4, 8),
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
void ShowsunEventEpoch()
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
  sunEventEpoch = localtime(&riseEpoch);
  strftime(chBuffer, sizeof(chBuffer), "%T", sunEventEpoch);
  tft.drawString("Sunrise at:", 4, dispLine1, 4);
  tft.drawString(chBuffer, 170, dispLine1, 4);
  Serial.printf("Sunrise: %s - ", chBuffer);

  setEpoch = sr.setTime;
  sunEventEpoch = localtime(&setEpoch);
  strftime(chBuffer, sizeof(chBuffer), "%T", sunEventEpoch);
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
  //  day_length.replace(":", "h ");
  //  day_length += "m";

  //  int sscanfHours, sscanfMins;
  if (sscanf(day_length, "%d:%d", &sscanfHours, &sscanfMins) == 2)
    snprintf(chBuffer, sizeof(chBuffer), "%dh %dm", sscanfHours, sscanfMins);
  else
    snprintf(chBuffer, sizeof(chBuffer), "Undefined");
  tft.drawString(chBuffer, 170, dispLine3, 4);

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
bool GetMoonData()
/*******************************************************************************************/
{
  static bool rc;

  rc = parseMoonData(apiKey, latitude, longitude);
  if (rc != 0)
    return false;
  else
    return true;
}
/*******************************************************************************************/
// Given:   latitude (phi), altitude (h), azimuth (A)
// Returns: declination (delta)
float moonDeclination(float latitude, float altitude, float azimuth)
/*******************************************************************************************/
{
  static float phi;
  static float h;
  static float A;

  // Convert everything to radians
  phi = radians(latitude);
  h   = radians(altitude);
  A   = radians(azimuth);

  // Formula: sin δ = sin φ * sin h + cos φ * cos h * cos A
  return degrees(asin(sin(phi) * sin(h) + cos(phi) * cos(h) * cos(A)));
}
/*******************************************************************************************/
// From CoP Nov 15
float maxMoonAltitude(float latitude, float fDeclination)
/*******************************************************************************************/
{
  return 90.0 - fabs(latitude - fDeclination);
}
/*******************************************************************************************/
float minMoonAltitude(float latitude, float declination)
/*******************************************************************************************/
{
  return -(90.0f - fabs(latitude + declination));
}
/*******************************************************************************************/
float ComputeArrow(float altitude)
/*******************************************************************************************/
{
  static float prevAlt = 0.0f;
  static bool wasRising = true;
  static MoonQuadrant quad = RISE_TO_MAX;
  bool rising;
  float pct, rot;

  // Serial.printf(
  //   "ComputeArrow start Alt %.2f, maxAltitude %.2f, minAltitude %.2f, prevAlt %.2f\r\n",
  //   altitude, fMaxAltitude, fMinAltitude, prevAlt);

  rising = (altitude > prevAlt);

  // Detect direction changes
  if (wasRising && !rising) quad = MAX_TO_SET;
  if (!wasRising && rising) quad = MIN_TO_RISE;

  // Detect horizon crossings
  if (prevAlt >= 0.0f && altitude < 0.0f) quad = SET_TO_MIN;
  if (prevAlt <= 0.0f && altitude > 0.0f) quad = RISE_TO_MAX;
  //  pct = constrain(fabs(altitude) / fMaxAltitude, 0.0f, 1.0f);

  switch (quad)
  {
    case RISE_TO_MAX:  // 0
      pct = constrain(altitude / fMaxAltitude, 0.0f, 1.0f);
      rot = 90.0f * (1.0f - pct);
      break;

    case MAX_TO_SET:  // 1
      pct = constrain(altitude / fMaxAltitude, 0.0f, 1.0f);
      rot = 270.0f + 90.0f * pct;
      if (rot >= 360.0f)
        rot -= 360.0f;
      break;

    case SET_TO_MIN:  // 2
      pct = constrain(fabs(altitude) / fabs(fMinAltitude), 0.0f, 1.0f);
      rot = 270.0f - 90.0f * pct;
      break;

    case MIN_TO_RISE: // 3
      pct = constrain(
              fabs(altitude) / fabs(fMinAltitude), 0.0f, 1.0f);
      rot = 90.0f + 90.0f * pct;
      // Serial.printf("MIN_TO_RISE pct %.3f\r\n", pct);
      break;

    default: rot = 0.0f; break;
  }

  // Serial.printf("Results of ComputeArrow: Quadrant %d, Rotation %.2f, prevAlt %.2f\r\n",
  //               quad, rot, prevAlt);
  // Serial.printf("Alt % .2f Prev % .2f Rising % d WasRising % d Quad % d Rot % .2f\r\n",
  //               altitude, prevAlt, rising, wasRising, quad, rot);

  prevAlt = altitude;
  wasRising = rising;

  return rot;
}
/*******************************************************************************************/
bool MoonMaxHeightTime( const char *riseStr, const char *setStr,
                        char *outStr, size_t outSize)
/*******************************************************************************************/
{
  static int riseH, riseM, setH, setM, riseMin, setMin, culmination;

  if (sscanf(riseStr, "%d:%d", &riseH, &riseM) != 2) return false;
  if (sscanf(setStr, "%d:%d", &setH, &setM) != 2)    return false;

  riseMin = riseH * 60 + riseM;
  setMin  = setH * 60 + setM;

  if (strlen(riseStr) < 5 || strlen(setStr) < 5) return false;  // If no rise today.

  if (setMin < riseMin) setMin += 1440;  // If set occurs after midnight, fix it.

  culmination = (riseMin + setMin) / 2;
  culmination %= 1440;

  snprintf(outStr, outSize, "%02d:%02d", culmination / 60, culmination % 60);

  return true;
}
/*******************************************************************************************/
double rad_to_degrees(double radians)
/*******************************************************************************************/
{
  return radians * (180.0 / M_PI);
}
/*******************************************************************************************/
const char* DegreesToCompass16(float degrees)
/*******************************************************************************************/
{
  static const char* dir[16] =
  {
    "N", "NNE", "NE", "ENE",
    "E", "ESE", "SE", "SSE",
    "S", "SSW", "SW", "WSW",
    "W", "WNW", "NW", "NNW"
  };

  // Normalize to 0-359.999
  while (degrees < 0)    degrees += 360.0f;
  while (degrees >= 360) degrees -= 360.0f;

  // Add half a sector (11.25°) for rounding
  iCompassIndex = (int)((degrees + 11.25f) / 22.5f) % 16;

  return dir[iCompassIndex];
}
/*******************************************************************************************/
const char* DegreesToCompass8(float degrees)
/*******************************************************************************************/
{
  static const char* dir[8] =
  {
    "N", "NE", "E", "SE",
    "S", "SW", "W", "NW"
  };

  // Normalize to 0-359.999
  while (degrees < 0.0f)
    degrees += 360.0f;

  while (degrees >= 360.0f)
    degrees -= 360.0f;

  // Add half a sector (22.5°) for rounding
  iCompassIndex = (int)((degrees + 22.5f) / 45.0f) % 8;

  return dir[iCompassIndex];
}
/*******************************************************************************************/
int parseMoonData(char* _KEY, float _LAT, float _LON)
/*******************************************************************************************/
{
  int error_code;

  sprintf(moonURL, "https://api.ipgeolocation.io/v3/astronomy?apiKey=%s&lat=%f&long=%f",
          _KEY, _LAT, _LON);

  //  Serial.println("Calling URL: ");
  //  Serial.println(moonURL);
  HTTPClient http;
  http.begin(moonURL);

  int httpCode = http.GET();
  //  Serial.printf("httpCode return code: %d\r\n", httpCode);
  if (httpCode > 399)
  {
    if (httpCode == 404)
    {
      http.end();
      error_code += 4;
      return error_code;
    }
    else
    {
      http.end();
      error_code += 5;
      return error_code;
    }

  }
  String payload = http.getString();
  //  Serial.println("api.ipgeolocation.io/v3/astronomy Return Packet");
  //  Serial.println(payload);

  // String input;

  JsonDocument doc;

  DeserializationError error = deserializeJson(doc, payload);

  if (error) {
    Serial.print("deserializeJson() failed: ");
    Serial.println(error.c_str());
    return (-1);
  }

  // First, the character variables

  JsonObject location = doc["location"];

  const char* location_latitudeL  = location["latitude"]  | "";  // "18.19121"
  const char* location_longitudeL = location["longitude"] | "";  // "120.55044"

  JsonObject astronomy = doc["astronomy"];

  // Small change for astronomy v3 to look inside ["astronomy"] for date and time.
  // Change per "https://arduinojson.org/v7/assistant" is to change to using "const char*"
  const char * current_dateL = astronomy["date"] | "";          // "2025-06-30" (ISO8601 format that Excel loves)
  const char * current_timeL = astronomy["current_time"] | "";  // "16:05:59.374"
  const char * sunriseL      = astronomy["sunrise"] | "";       // "05:25"
  const char * sunsetL       = astronomy["sunset"] | "";        // "18:37"
  const char * sun_statusL   = astronomy["sun_status"] | "";    // "-"
  const char * solar_noonL   = astronomy["solar_noon"] | "";    // "12:01"
  const char * day_lengthL   = astronomy["day_length"] | "";    // "13:12"
  const char * moonriseL     = astronomy["moonrise"] | "";      // "09:50"
  const char * moonsetL      = astronomy["moonset"] | "";       // "22:33"
  const char * moon_statusL  = astronomy["moon_status"] | "";   // "-"
  const char * moon_phaseL   = astronomy["moon_phase"] | "";    // "WAXING_CRESCENT"
  const char * moon_illum_percentageL = astronomy["moon_illumination_percentage"] | ""; // "26.53"

  // Do the copy outs
  strlcpy(location_longitude, location_longitudeL, sizeof(location_longitude));
  strlcpy(location_latitude, location_latitudeL, sizeof(location_latitude));
  strlcpy(current_time, current_timeL, sizeof(current_time));
  strlcpy(current_date, current_dateL, sizeof(current_date));
  strlcpy(sunrise, sunriseL, sizeof(sunrise));
  strlcpy(sunset, sunsetL, sizeof(sunset));
  strlcpy(sun_status, sun_statusL, sizeof(sun_status));
  strlcpy(solar_noon, solar_noonL, sizeof(solar_noon));
  strlcpy(day_length, day_lengthL, sizeof(day_length));
  strlcpy(moonrise, moonriseL, sizeof(moonrise));
  strlcpy(moonset, moonsetL, sizeof(moonset));
  strlcpy(moon_status, moon_statusL, sizeof(moon_status));
  strlcpy(moon_phase, moon_phaseL, sizeof(moon_phase));
  strlcpy(moon_illum_percentage, moon_illum_percentageL, sizeof(moon_illum_percentage));

  // Then the numeric variables
  sun_altitude  = astronomy["sun_altitude"];   // 33.009196129845215
  sun_azimuth   = astronomy["sun_azimuth"];    // 286.2520179561225
  sun_distance  = astronomy["sun_distance"];   // 152082740.2434402
  moon_altitude = astronomy["moon_altitude"];  // 79.98075904963498
  moon_distance = astronomy["moon_distance"];  // 391324.0794163109
  moon_azimuth  = astronomy["moon_azimuth"];   // 168.85244425102178
  moon_angle    = astronomy["moon_angle"];     // 62.00725976252736
  moon_parallactic_angle = astronomy["moon_parallactic_angle"];  // -10.690759049458498

  http.end();
  return 0;
}
