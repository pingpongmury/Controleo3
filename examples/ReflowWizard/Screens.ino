// Written by Peter Easton
// Released under the MIT license
// Build a reflow oven: https://whizoo.com


extern void setTouchCallback(void (*f) (), uint16_t interval);
extern boolean drawTemperatureOnScreenNow;

void setTouchTemperatureUnitChangeCallback(void (*f) (boolean));

  #define NUM_POWER_SETTINGS             6
  #define NUM_INERTIA_SETTINGS           9
  #define NUM_INSULATION_SETTINGS        4

// This is the main loop, displaying one screen after the other as the user
// navigates between them.
void showScreen(uint8_t screen) 
{ 
  profiles *p;
  uint8_t output = 0;
  boolean onOff = 0;
  uint8_t bypassPower, bypassInertia, bypassInsulation;

  uint16_t powerMap[NUM_POWER_SETTINGS+1] = {5, 10, 15, 20, 25, 30, 0};
  uint16_t inertiaMap[NUM_INERTIA_SETTINGS+1] = {30, 35, 40, 45, 50, 60, 80, 120, 180, 0};
  uint16_t insulationMap[NUM_INSULATION_SETTINGS+1] = {100, 130, 170, 220, 0};
  const char* powerDescription[] = {"120~C needs 5% duty", "120~C needs 10% duty", "120~C needs 15% duty", "120~C needs 20% duty", "120~C needs 25% duty", "120~C needs 30% duty"};
  const char* inertiaDescription[] = {"30~C rise in 30 sec", "30~C rise in 35 sec", "30~C rise in 40 sec", "30~C rise in 45 sec", "30~C rise in 50 sec", \
                                "30~C rise in 60 sec","30~C rise in 80 sec", "30~C rise in 2 minutes", "30~C rise in 3 minutes"};
  const char* insulationDescription[] = {"Insulation is poor", "Insulation is pretty good", "Insulation is excellent", "Like me, it is amazing!"};
  const uint16_t powerX[NUM_POWER_SETTINGS] = {115, 108, 109, 107, 107, 107};
  const uint16_t inertiaX[NUM_INERTIA_SETTINGS] = {128, 128, 127, 128, 128, 127, 127, 109, 110};
  const uint16_t insulationX[NUM_INSULATION_SETTINGS] = {142, 101, 117, 111};
  

  // This is the main loop, changing between the various screens.  Stay here forever ...
  while (1) {
    // Clear the screen
    tft.fillScreen(WHITE);

    // Check the amount of free memory each time the screen is drawn.  Make sure there
    // aren't any memory leaks
    checkFreeMemory();
    
    // Ug, hate goto's!  But this saves a lot of extraneous code.
    // This is mostly used after drawing a help screen, where erasing the screen
    // and redrawing it will causes flicker as the screen refreshes.
redraw:

    // Erase all touch targets
    clearTouchTargets();

    // Set the flag to display the temperature on the screen as soon as possible
    drawTemperatureOnScreenNow = true;

    switch (screen) {
      case SCREEN_HOME: 
        // Draw the screen
        renderBitmap(BITMAP_CONTROLEO3_SMALL, 106, 5);
        drawTouchButton(110, 80, 260, 125, BUTTON_LARGE_FONT, (char *) "Profiles");
        drawTouchButton(110, 160, 260, 77, BUTTON_LARGE_FONT, (char *) "Bake");
        drawTouchButton(110, 240, 260, 182, BUTTON_LARGE_FONT, (char *) "Settings");
        renderBitmap(BITMAP_SETTINGS, 285, 252);

        // Act on the tap
        switch(getTap(DONT_SHOW_TEMPERATURE)) {
          case 0: screen = SCREEN_REFLOW; break;
          case 1: screen = SCREEN_BAKE; break;
          case 2: screen = SCREEN_SETTINGS; break;
          case 99: enterSleep(); goto redraw;
        }
        break;

      case SCREEN_BAKE:
        // Draw the screen
        displayHeader((char *) "Bake", false);
        setTouchTemperatureUnitChangeCallback(displayBakeTemperatureAndDuration);
        drawTouchButton(140, 100, 200, 56, BUTTON_SMALL_FONT, (char *) "Start");
        drawTouchButton(140, 180, 200, 96, BUTTON_SMALL_FONT, (char *) "Edit");
        renderBitmap(BITMAP_SETTINGS, 247, 192);
        drawNavigationButtons(true, true);

        // Act on the tap
        switch(getTap(SHOW_TEMPERATURE_IN_HEADER)) {
          case 0: bake(); break;
          case 1: screen = SCREEN_EDIT_BAKE1; break;
          case 2: screen = SCREEN_HOME; break;
          case 3: screen = SCREEN_HOME; break;
          case 4: showHelp(SCREEN_BAKE); goto redraw;
          case 5: screen = SCREEN_EDIT_BAKE1; break;
          case 99: enterSleep(); goto redraw;
        }
        break;
                
      case SCREEN_EDIT_BAKE1:
        // Draw the screen
        displayHeader((char *) "Bake", true);
        displayString(20, LINE(0), FONT_9PT_BLACK_ON_WHITE, (char *) "Temperature:");
        displayString(20, LINE(3), FONT_9PT_BLACK_ON_WHITE, (char *) "Duration:");
        drawIncreaseDecreaseTapTargets(TWO_SETTINGS);
        drawNavigationButtons(true, true);

        while (1) {
          // Show the temperature and duration
          sprintf(buffer100Bytes, "%d~C", prefs.bakeTemperature);
          displayFixedWidthString(180, LINE(0), buffer100Bytes, 5, FONT_9PT_BLACK_ON_WHITE_FIXED);
          secondsToEnglishString(buffer100Bytes, getBakeSeconds(prefs.bakeDuration));
          tft.fillRect(130, LINE(3), 267, 24, WHITE);
          displayString(130, LINE(3), FONT_9PT_BLACK_ON_WHITE_FIXED, buffer100Bytes);

          // Act on the tap
          switch(getTap(SHOW_TEMPERATURE_IN_HEADER)) {
            case 0: 
              if (prefs.bakeTemperature > BAKE_MIN_TEMPERATURE) {
                prefs.bakeTemperature -= BAKE_TEMPERATURE_STEP;
                savePrefs();
              }
              else
                playTones(TUNE_INVALID_PRESS);
              break;
            case 1: 
              if (prefs.bakeTemperature < BAKE_MAX_TEMPERATURE) {
                prefs.bakeTemperature += BAKE_TEMPERATURE_STEP;
                savePrefs();
              }
              else
                playTones(TUNE_INVALID_PRESS);
              break;
            case 2: 
              if (prefs.bakeDuration > 0) {
                prefs.bakeDuration--;
                savePrefs();
              }
              else
                playTones(TUNE_INVALID_PRESS);
              break;
            case 3: 
              if (prefs.bakeDuration < BAKE_MAX_DURATION) {
                prefs.bakeDuration++;
                savePrefs();
              }
              else
                playTones(TUNE_INVALID_PRESS);
              break;
              
            case 4: screen = SCREEN_BAKE; break;
            case 5: screen = SCREEN_HOME; break;
            case 6: showHelp(SCREEN_EDIT_BAKE1); goto redraw;
            case 7: screen = SCREEN_EDIT_BAKE2;
            case 99: enterSleep(); goto redraw;
          }
          if (screen != SCREEN_EDIT_BAKE1)
            break;
        }
        break;
                
      case SCREEN_EDIT_BAKE2:
        // Draw the screen
        displayHeader((char *) "Bake", true);
        drawIncreaseDecreaseTapTargets(TWO_SETTINGS);
        drawNavigationButtons(true, true);

        while (1) {
          // Show "open door after bake"
          tft.fillRect(20, LINE(0), 434, 24, WHITE);
          displayString(20, LINE(0), FONT_9PT_BLACK_ON_WHITE, (char *) bakeDoorDescription[prefs.openDoorAfterBake]);
          // Show "use cooling fan"
          tft.fillRect(20, LINE(3), 434, 24, WHITE);
          displayString(20, LINE(3), FONT_9PT_BLACK_ON_WHITE, (char *) bakeUseCoolingFan[prefs.bakeUseCoolingFan]);

          // Act on the tap
          switch(getTap(SHOW_TEMPERATURE_IN_HEADER)) {
            case 0: 
              if (prefs.openDoorAfterBake > 0)
                prefs.openDoorAfterBake--;
              else
                prefs.openDoorAfterBake = BAKE_DOOR_LAST_OPTION;
              savePrefs();
              break;
            case 1: 
              if (prefs.openDoorAfterBake < BAKE_DOOR_LAST_OPTION)
                prefs.openDoorAfterBake++;
              else
                prefs.openDoorAfterBake = 0;
              savePrefs();
              break;
            case 2: 
            case 3:
              prefs.bakeUseCoolingFan = 1 - prefs.bakeUseCoolingFan;
              savePrefs();
              break;
            case 4: screen = SCREEN_EDIT_BAKE1; break;
            case 5: screen = SCREEN_HOME; break;
            case 6: showHelp(SCREEN_EDIT_BAKE2); goto redraw;
            case 7: screen = SCREEN_BAKE;
            case 99: enterSleep(); goto redraw;
          }
          if (screen != SCREEN_EDIT_BAKE2)
            break;
        }
        break;
                
      case SCREEN_REFLOW:
        // If there aren't any profiles then go straight to the reading the SD card
        if (!prefs.numProfiles) {
          screen = SCREEN_CHOOSE_PROFILE;
          goto redraw;
        }
        
        // Draw the screen
        displayHeader((char *) "Profiles", false);
        p = &prefs.profile[prefs.selectedProfile];
        tft.fillRect(20, LINE(0), 434, 24, WHITE);
        sprintf(buffer100Bytes, "#%d: %s", prefs.selectedProfile+1, p->name);
        displayString(20, LINE(0), FONT_9PT_BLACK_ON_WHITE, buffer100Bytes);
        drawTouchButton(120, 100, 240, 130, BUTTON_SMALL_FONT, (char *) "Run Profile");
        drawTouchButton(120, 180, 240, 170, BUTTON_SMALL_FONT, (char *) "Choose Profile");
        drawNavigationButtons(false, false);

        // Act on the tap
        switch(getTap(SHOW_TEMPERATURE_IN_HEADER)) {
          case 0: reflow(prefs.selectedProfile); break;
          case 1: screen = SCREEN_CHOOSE_PROFILE; break;
          case 2: screen = SCREEN_HOME; break;
          case 3: screen = SCREEN_HOME; break;
          case 4: showHelp(SCREEN_REFLOW); goto redraw;
          case 99: enterSleep(); goto redraw;
        }
        break;
                
       case SCREEN_CHOOSE_PROFILE:
        // Draw the screen
        displayHeader((char *) "Choose Profile", false);
        if (prefs.numProfiles) {
          drawIncreaseDecreaseTapTargets(ONE_SETTING_TEXT_BUTTON);
          drawTouchButton(10, 185, 210, 130, BUTTON_SMALL_FONT, (char *) "Delete");
          renderBitmap(BITMAP_TRASH, 155, 196);
          drawTouchButton(260, 185, 210, 168, BUTTON_SMALL_FONT, (char *) "Read SD Card");
        }
        else {
          // Dummy areas for the arrows and delete button
          defineTouchArea(0, 0, 0, 0);
          defineTouchArea(0, 0, 0, 0);
          defineTouchArea(0, 0, 0, 0);
          drawTouchButton(40, 105, 400, 330, BUTTON_SMALL_FONT, (char *) "Read Profiles from SD Card");
        }
        drawNavigationButtons(false, false);

        while (1) {
          p = &prefs.profile[prefs.selectedProfile];
          tft.fillRect(20, LINE(0), 400, 24, WHITE);
          sprintf(buffer100Bytes, "#%d: %s", prefs.selectedProfile+1, p->name);
          if (prefs.numProfiles)
            displayString(20, LINE(0), FONT_9PT_BLACK_ON_WHITE, buffer100Bytes);
          tft.fillRect(20, LINE(1), 400, 24, WHITE);
          sprintf(buffer100Bytes, "Peaks at %d~C (%d instructions)", p->peakTemperature, p->noOfTokens);
          if (prefs.numProfiles)
            displayString(20, LINE(1), FONT_9PT_BLACK_ON_WHITE, buffer100Bytes);
          
          // Act on the tap
          switch(getTap(SHOW_TEMPERATURE_IN_HEADER)) {
            case 0: prefs.selectedProfile = (prefs.selectedProfile + prefs.numProfiles -1) % prefs.numProfiles; savePrefs(); break;
            case 1: prefs.selectedProfile = (prefs.selectedProfile + 1) % prefs.numProfiles; savePrefs(); break;
            case 2: 
              drawThickRectangle(0, 90, 480, 230, 15, RED);
              tft.fillRect(15, 105, 450, 200, WHITE);
              displayString(126, 117, FONT_12PT_BLACK_ON_WHITE, (char *) "Delete Profile?");
              displayString(40, 150, FONT_9PT_BLACK_ON_WHITE, (char *) "Are you sure you want to delete");
              displayString(40, 180, FONT_9PT_BLACK_ON_WHITE, (char *) "this profile?");
              clearTouchTargets();
              drawTouchButton(60, 230, 160, 99, BUTTON_LARGE_FONT, (char *) "Delete");
              drawTouchButton(260, 230, 160, 105, BUTTON_LARGE_FONT, (char *) "Cancel");
              if (getTap(SHOW_TEMPERATURE_IN_HEADER) == 0) {
                deleteProfile(prefs.selectedProfile);
                prefs.selectedProfile = 0;
              }
              tft.fillRect(0, 90, 480, 230, WHITE);
              goto redraw;
            
            case 3: 
              tft.fillRect(40, 105, 400, 61, WHITE);
              displayString(128, 120, FONT_9PT_BLACK_ON_WHITE, (char *) "Reading SD card ...");
              ReadProfilesFromSDCard(); 
              tft.fillRect(40, 105, 400, 61, WHITE);
              prefs.selectedProfile = 0;
              goto redraw;
            case 4: screen = SCREEN_REFLOW; break;
            case 5: screen = SCREEN_HOME; break;
            case 6: showHelp(SCREEN_CHOOSE_PROFILE); goto redraw;
            case 99: enterSleep(); goto redraw;
          }
          if (screen != SCREEN_CHOOSE_PROFILE || !prefs.numProfiles)
            break;
        }
        break;
                
      case SCREEN_SETTINGS:
        // Draw the screen
        displayHeader((char *) "Settings", true);
        drawTouchButton(10, 45, 210, 50, BUTTON_SMALL_FONT, (char *) "Test");
        drawTouchButton(10, 120, 210, 97, BUTTON_SMALL_FONT, (char *) "Learning");
        drawTouchButton(10, 195, 210, 135, BUTTON_SMALL_FONT, (char *) "Log / Reset");
        drawTouchButton(260, 45, 210, 67, BUTTON_SMALL_FONT, (char *) "Setup");
        drawTouchButton(260, 120, 210, 127, BUTTON_SMALL_FONT, (char *) "PID Tuning");
        drawTouchButton(260, 195, 210, 69, BUTTON_SMALL_FONT, (char *) "About");
        drawNavigationButtons(false, false);

        // Act on the tap
        switch(getTap(SHOW_TEMPERATURE_IN_HEADER)) {
          case 0: output = 0; screen = SCREEN_TEST; break;
          case 1: screen = SCREEN_LEARNING; break;
          case 2: screen = SCREEN_RESET; break;
          case 3: output = 0; screen = SCREEN_SETUP_OUTPUTS; break;
          case 4: screen = SCREEN_PID_TUNING; break;
          case 5: screen = SCREEN_ABOUT; break;
          case 6:
          case 7: screen = SCREEN_HOME; break;
          case 8: showHelp(SCREEN_SETTINGS); goto redraw;
          case 99: enterSleep(); goto redraw;
        }
        break;
        
      case SCREEN_TEST:
        // Draw the screen
        displayHeader((char *) "Test Outputs", false);
        defineTouchArea(40, 60, 400, 170); // Supersize the button
        drawNavigationButtons(true, true);
        displayString(20, LINE(0), FONT_9PT_BLACK_ON_WHITE, (char *) "Output");

        // Allow the user to move between all 6 outputs
        while (1) {
          // Display the current output state
          sprintf(buffer100Bytes, "%d is %s.", output+1, onOff ? "On": "Off");
          tft.fillRect(107, LINE(0), 95, 19, WHITE);
          displayString(107, LINE(0), FONT_9PT_BLACK_ON_WHITE, buffer100Bytes);
          // Set the output to the current state
          setOutput(output, onOff);
          // Update the button text
          tft.fillRect(185, 170, 110, 19, WHITE);
          if (onOff)
            drawButton(140, 150, 200, 101, BUTTON_SMALL_FONT, (char *) "Turn Off");
          else
            drawButton(140, 150, 200, 96, BUTTON_SMALL_FONT, (char *) "Turn On");
          if (areOutputsConfigured()) {
            tft.fillRect(20, LINE(1), 190, 24, WHITE);
            displayString(20, LINE(1), FONT_9PT_BLACK_ON_WHITE, (char *) outputDescription[prefs.outputType[output]]);
            // Display an icon corresponding to the output
            drawTestOutputIcon(prefs.outputType[output], onOff);
          }

          // Act on the tap
          switch(getTap(NO_SLEEP)) {
            case 0: onOff = 1 - onOff;
                    break;
            case 1: setOutput(output, 0);
                    onOff = 0;
                    if (output > 0)
                      output--;
                    else
                      screen = SCREEN_SETTINGS;
                    break;
            case 2: setOutput(output, 0);
                    screen = SCREEN_HOME;
                    break;
            case 3: showHelp(SCREEN_TEST);
                    goto redraw;
            case 4: if (output + 1 < NUMBER_OF_OUTPUTS) {
                      setOutput(output, 0);
                      onOff = 0;
                      output++;
                    }
                    else
                      screen = SCREEN_SETTINGS;
                    break;
          }
          // If no longer on this screen, go to the new one
          if (screen != SCREEN_TEST) {
            onOff = 0;
            output = 0;
            break;
          }
        }
        break;

      case SCREEN_SETUP_OUTPUTS:
        // Draw the screen
        displayHeader((char *) "Outputs", true);
        drawIncreaseDecreaseTapTargets(ONE_SETTING_WITH_TEXT);
        drawNavigationButtons(true, true);
        displayString(20, LINE(0), FONT_9PT_BLACK_ON_WHITE, (char *) "Output");

        // Allow the user to move between all 6 outputs
        while (true) {
          // Display the current output state
          sprintf(buffer100Bytes, "%d is %s.", output+1, outputDescription[prefs.outputType[output]]);
          tft.fillRect(107, LINE(0), 248, 24, WHITE);
          displayString(107, LINE(0), FONT_9PT_BLACK_ON_WHITE, buffer100Bytes);
          tft.fillRect(20, LINE(1), 440, 24, WHITE);
          displayString(20, LINE(1), FONT_9PT_BLACK_ON_WHITE, (char *) longOutputDescription[prefs.outputType[output]]);

          // Act on the tap
          switch(getTap(NO_SLEEP)) {
            case 0: prefs.outputType[output] = (prefs.outputType[output] + NO_OF_TYPES - 1) % NO_OF_TYPES;
                    savePrefs();
                    break;
            case 1: prefs.outputType[output] = (prefs.outputType[output] + 1) % NO_OF_TYPES;
                    savePrefs();
                    break;
            case 2: if (output > 0)
                      output--;
                    else
                      screen = SCREEN_SETTINGS;
                    break;
            case 3: screen = SCREEN_HOME;
                    break;
            case 4: showHelp(SCREEN_SETUP_OUTPUTS);
                    goto redraw;
            case 5: if (output + 1 < NUMBER_OF_OUTPUTS)
                      output++;
                    else
                      screen = SCREEN_SERVO_OPEN;
                    break;
          }
          // If no longer on this screen, go to the new one
          if (screen != SCREEN_SETUP_OUTPUTS)
            break;
        }
        break;

      case SCREEN_SERVO_OPEN:
        // Draw the screen
        displayHeader((char *) "Door Servo", true);
        displayString(20, LINE(0), FONT_9PT_BLACK_ON_WHITE, (char *) "Door open position:");
        drawIncreaseDecreaseTapTargets(ONE_SETTING);
        drawNavigationButtons(true, true);

        while (1) {
          // Show the degrees
          sprintf(buffer100Bytes, "%d~", prefs.servoOpenDegrees);
          displayFixedWidthString(255, LINE(0), buffer100Bytes, 4, FONT_9PT_BLACK_ON_WHITE_FIXED);
          setServoPosition(prefs.servoOpenDegrees, 1000);

          // Act on the tap
          switch(getTap(NO_SLEEP)) {
            case 0: 
              if (prefs.servoOpenDegrees > 0) {
                prefs.servoOpenDegrees -= 5;
                savePrefs();
              }
              else
                playTones(TUNE_INVALID_PRESS);
              break;
            case 1: 
              if (prefs.servoOpenDegrees < 180) {
                prefs.servoOpenDegrees += 5;
                savePrefs();
              }
              else
                playTones(TUNE_INVALID_PRESS);
              break;
              
            case 2: screen = SCREEN_SETUP_OUTPUTS; break;
            case 3: screen = SCREEN_HOME; break;
            case 4: showHelp(SCREEN_SERVO_OPEN); goto redraw;
            case 5: screen = SCREEN_SERVO_CLOSE;
                    tft.fillRect(80, LINE(0), 250, 24, WHITE);
                    goto redraw;
          }
          if (screen != SCREEN_SERVO_OPEN)
            break;
        }
        break;
        
       case SCREEN_SERVO_CLOSE:
        // Draw the screen
        displayHeader((char *) "Door Servo", true);
        displayString(20, LINE(0), FONT_9PT_BLACK_ON_WHITE, (char *) "Door closed position:");
        drawIncreaseDecreaseTapTargets(ONE_SETTING);
        drawNavigationButtons(true, true);

        while (1) {
          // Show the degrees
          sprintf(buffer100Bytes, "%d~", prefs.servoClosedDegrees);
          displayFixedWidthString(265, LINE(0), buffer100Bytes, 4, FONT_9PT_BLACK_ON_WHITE_FIXED);
          setServoPosition(prefs.servoClosedDegrees, 1000);

          // Act on the tap
          switch(getTap(NO_SLEEP)) {
            case 0: 
              if (prefs.servoClosedDegrees > 0) {
                prefs.servoClosedDegrees -= 5;
                savePrefs();
              }
              else
                playTones(TUNE_INVALID_PRESS);
              break;
            case 1: 
              if (prefs.servoClosedDegrees < 180) {
                prefs.servoClosedDegrees += 5;
                savePrefs();
              }
              else
                playTones(TUNE_INVALID_PRESS);
              break;
              
            case 2: screen = SCREEN_SERVO_OPEN; 
                    tft.fillRect(80, LINE(0), 250, 24, WHITE);
                    goto redraw;
            case 3: screen = SCREEN_HOME; break;
            case 4: showHelp(SCREEN_SERVO_OPEN); goto redraw;
            case 5: screen = SCREEN_LINE_FREQUENCY; break;
          }
          if (screen != SCREEN_SERVO_CLOSE)
            break;
        }
        break;
        
       case SCREEN_LINE_FREQUENCY:
        // Draw the screen
        displayHeader((char *) "Line Frequency", true);
        displayString(20, LINE(0), FONT_9PT_BLACK_ON_WHITE, (char *) "Power line frequency: ");
        drawIncreaseDecreaseTapTargets(ONE_SETTING_WITH_TEXT);
        drawNavigationButtons(true, true);

        while (1) {
          tft.fillRect(20, LINE(1), 440, 24, WHITE);
          if (prefs.lineVoltageFrequency == CR0_NOISE_FILTER_60HZ) {
            displayFixedWidthString(280, LINE(0), (char *) "60Hz", 4,  FONT_9PT_BLACK_ON_WHITE_FIXED);
            displayString(20, LINE(1), FONT_9PT_BLACK_ON_WHITE, (char *) "North America, Canada");
          }
          else {
            displayFixedWidthString(280, LINE(0), (char *) "50Hz", 4,  FONT_9PT_BLACK_ON_WHITE_FIXED);
            displayString(20, LINE(1), FONT_9PT_BLACK_ON_WHITE, (char *) "Europe, Africa, Asia, Australia");
          }

          // Act on the tap
          switch(getTap(NO_SLEEP)) {
            case 0:
            case 1:
              prefs.lineVoltageFrequency = 1 - prefs.lineVoltageFrequency;
              initTemperature();
              savePrefs();
              break;
            case 2: screen = SCREEN_SERVO_CLOSE; break;
            case 3: screen = SCREEN_HOME; break;
            case 4: showHelp(SCREEN_LINE_FREQUENCY); goto redraw;
            case 5: screen = SCREEN_SETTINGS;
          }
          if (screen != SCREEN_LINE_FREQUENCY)
            break;
        }
        break;
                
       case SCREEN_RESET:
        // Draw the screen
        displayHeader((char *) "Log & Reset", false);

        // Logging
        displayString(20, LINE(0), FONT_9PT_BLACK_ON_WHITE, (char *) "Log time & temperature:");
        renderBitmap(BITMAP_DECREASE_ARROW, 40, LINE(1));
        renderBitmap(BITMAP_INCREASE_ARROW, 354, LINE(1));
        defineTouchArea(0, LINE(1)-10, 150, 60);
        defineTouchArea(330, LINE(1)-10, 150, 60);

        // Reset and touch calibration
        displayString(20, 150, FONT_9PT_BLACK_ON_WHITE, (char *) "Reset:");
        drawTouchButton(20, 180, 180, 134, BUTTON_SMALL_FONT, (char *) "All Settings");
        drawTouchButton(215, 180, 247, 201, BUTTON_SMALL_FONT, (char *) "Touch Calibration");
        drawNavigationButtons(false, false);

        // Act on the tap
        while (1) {
          displayString(prefs.logToSDCard? 140:179, LINE(1)+10, FONT_9PT_BLACK_ON_WHITE, prefs.logToSDCard? (char *) "Write to SD card" : (char *) "No logging");

          switch(getTap(NO_SLEEP)) {
            case 0:
            case 1:
              prefs.logToSDCard = 1 - prefs.logToSDCard;
              tft.fillRect(140, LINE(1)+10, 200, 24, WHITE);
              savePrefs();
              continue;
            case 2:
              drawThickRectangle(0, 90, 480, 230, 15, RED);
              tft.fillRect(15, 105, 450, 200, WHITE);
              displayString(126, 110, FONT_12PT_BLACK_ON_WHITE, (char *) "Factory Reset");
              displayString(54, 150, FONT_9PT_BLACK_ON_WHITE, (char *) "Are you sure you want to erase");
              displayString(54, 180, FONT_9PT_BLACK_ON_WHITE, (char *) "all settings?");
              clearTouchTargets();
              drawTouchButton(60, 230, 160, 88, BUTTON_LARGE_FONT, (char *) "Erase");
              drawTouchButton(260, 230, 160, 105, BUTTON_LARGE_FONT, (char *) "Cancel");
              if (getTap(SHOW_TEMPERATURE_IN_HEADER) == 0) {
                factoryReset(true);
                NVIC_SystemReset();
              }
              tft.fillRect(0, 90, 480, 230, WHITE);
              goto redraw;
            case 3: CalibrateTouchscreen(); break;
            case 4: screen = SCREEN_SETTINGS; break;
            case 5: screen = SCREEN_HOME; break;
            case 6: showHelp(SCREEN_RESET); goto redraw;
          }
          // Clear this screen and go to the new screen (redraw this one if touch calibration was tapped)
          break;
        }
        break;
                
       case SCREEN_ABOUT:
        // Draw the screen
        renderBitmap(BITMAP_CONTROLEO3_SMALL, 2, 5);
        renderBitmap(BITMAP_WHIZOO_SMALL, 316, 21);
        displayString(279, 30, FONT_9PT_BLACK_ON_WHITE, (char *) "by");
        tft.fillRect(5, 57, 470, 3, LIGHT_GREY);

        // Display information
        displayString(20, 70, FONT_9PT_BLACK_ON_WHITE, (char *) "Software:");
        displayString(139, 70, FONT_9PT_BLACK_ON_WHITE, (char *) CONTROLEO3_VERSION);
        displayString(20, 100, FONT_9PT_BLACK_ON_WHITE, (char *) "Serial Number:");
        sprintf(buffer100Bytes, "%lX", *((uint32_t *) 0x0080A00C));
        displayString(202, 100, FONT_9PT_BLACK_ON_WHITE, buffer100Bytes);
        displayString(20, 130, FONT_9PT_BLACK_ON_WHITE, (char *) "Flash ID:");
        sprintf(buffer100Bytes, "%lX", flash.readUniqueID());
        displayString(129, 130, FONT_9PT_BLACK_ON_WHITE, buffer100Bytes);
        displayString(20, 160, FONT_9PT_BLACK_ON_WHITE, (char *) "LCD Version:");
        sprintf(buffer100Bytes, "%lX", tft.getLCDVersion());
        displayString(180, 160, FONT_9PT_BLACK_ON_WHITE, buffer100Bytes);
        displayString(20, 190, FONT_9PT_BLACK_ON_WHITE, (char *) "Free RAM:");
        sprintf(buffer100Bytes, "%ld bytes (%ld%% free)", getFreeRAM(), getFreeRAM() / 320); // Has 32KB RAM
        displayString(156, 190, FONT_9PT_BLACK_ON_WHITE, buffer100Bytes);
        displayString(20, 220, FONT_9PT_BLACK_ON_WHITE, (char *) "Profiles run:");
        sprintf(buffer100Bytes, "%d", prefs.numReflows);
        displayString(169, 220, FONT_9PT_BLACK_ON_WHITE, buffer100Bytes);
        displayString(280, 220, FONT_9PT_BLACK_ON_WHITE, (char *) "Bakes:");
        sprintf(buffer100Bytes, "%d", prefs.numBakes);
        displayString(363, 220, FONT_9PT_BLACK_ON_WHITE, buffer100Bytes);

        drawNavigationButtons(false, true);

        switch(getTap(DONT_SHOW_TEMPERATURE)) {
          case 0: screen = SCREEN_SETTINGS; break;
          case 1: screen = SCREEN_HOME; break;
          case 2: showHelp(SCREEN_ABOUT); goto redraw;
        }
        break;
 
       case SCREEN_PID_TUNING:
        // Draw the screen
        displayHeader((char *) "Manual PID Tuning", false);

        // Draw all the buttons
        #define BL_LINE(x)    ((x * 60) + 60)
        #define BL_TEXT(x)    ((x * 60) + 69)
        // Power
        renderBitmap(BITMAP_DECREASE_ARROW, 10, BL_LINE(0));
        renderBitmap(BITMAP_INCREASE_ARROW, 384, BL_LINE(0));
        defineTouchArea(0, BL_LINE(0)-10, 120, 60);
        defineTouchArea(360, BL_LINE(0)-10, 120, 60);
        // Inertia
        renderBitmap(BITMAP_DECREASE_ARROW, 10, BL_LINE(1));
        renderBitmap(BITMAP_INCREASE_ARROW, 384, BL_LINE(1));
        defineTouchArea(0, BL_LINE(1)-10, 120, 60);
        defineTouchArea(360, BL_LINE(1)-10, 120, 60);
        // Insulation
        renderBitmap(BITMAP_DECREASE_ARROW, 10, BL_LINE(2));
        renderBitmap(BITMAP_INCREASE_ARROW, 384, BL_LINE(2));
        defineTouchArea(0, BL_LINE(2)-10, 120, 60);
        defineTouchArea(360, BL_LINE(2)-10, 120, 60);
        drawNavigationButtons(false, false);

        // Map existing values to the bypass values
        bypassPower = mapValue(prefs.learnedPower, powerMap);
        bypassInertia = mapValue(prefs.learnedInertia, inertiaMap);
        bypassInsulation = mapValue(prefs.learnedInsulation, insulationMap);

        // Display the current values
        displayString(powerX[bypassPower], BL_TEXT(0), FONT_9PT_BLACK_ON_WHITE, (char *) powerDescription[bypassPower]);
        displayString(inertiaX[bypassInertia], BL_TEXT(1), FONT_9PT_BLACK_ON_WHITE, (char *) inertiaDescription[bypassInertia]);
        displayString(insulationX[bypassInsulation], BL_TEXT(2), FONT_9PT_BLACK_ON_WHITE, (char *) insulationDescription[bypassInsulation]);

        while (1) {
          // Act on the tap
          switch(getTap(NO_SLEEP)) {
            case 0:
              if (bypassPower) {
                bypassPower--;
                tft.fillRect(97, BL_TEXT(0), 287, 24, WHITE);
                displayString(powerX[bypassPower], BL_TEXT(0), FONT_9PT_BLACK_ON_WHITE, (char *) powerDescription[bypassPower]);
              }
              break;
            case 1:
              if (bypassPower < NUM_POWER_SETTINGS-1) {
                bypassPower++;
                tft.fillRect(97, BL_TEXT(0), 287, 24, WHITE);
                displayString(powerX[bypassPower], BL_TEXT(0), FONT_9PT_BLACK_ON_WHITE, (char *) powerDescription[bypassPower]);
              }
              break;
            case 2:
              if (bypassInertia) {
                bypassInertia--;
                tft.fillRect(97, BL_TEXT(1), 287, 24, WHITE);
                displayString(inertiaX[bypassInertia], BL_TEXT(1), FONT_9PT_BLACK_ON_WHITE, (char *) inertiaDescription[bypassInertia]);
              }
              break;
            case 3:
              if (bypassInertia < NUM_INERTIA_SETTINGS-1) {
                bypassInertia++;
                tft.fillRect(97, BL_TEXT(1), 287, 24, WHITE);
                displayString(inertiaX[bypassInertia], BL_TEXT(1), FONT_9PT_BLACK_ON_WHITE, (char *) inertiaDescription[bypassInertia]);
              }
              break;
            case 4:
              if (bypassInsulation) {
                bypassInsulation--;
                tft.fillRect(97, BL_TEXT(2), 287, 24, WHITE);
                displayString(insulationX[bypassInsulation], BL_TEXT(2), FONT_9PT_BLACK_ON_WHITE, (char *) insulationDescription[bypassInsulation]);
              }
              break;
            case 5:
              if (bypassInsulation < NUM_INSULATION_SETTINGS-1) {
                bypassInsulation++;
                tft.fillRect(97, BL_TEXT(2), 287, 24, WHITE);
                displayString(insulationX[bypassInsulation], BL_TEXT(2), FONT_9PT_BLACK_ON_WHITE, (char *) insulationDescription[bypassInsulation]);
              }
              break;
            case 6: screen = SCREEN_SETTINGS; break;
            case 7: screen = SCREEN_HOME; break;
            case 8: showHelp(SCREEN_PID_TUNING); goto redraw;
          }
          if (screen != SCREEN_PID_TUNING) {
            // See if the learned values should be overwitten
            if (prefs.learningComplete == LEARNING_DONE) {
              // Ask the user if these values should overwrite the learned values
              drawThickRectangle(0, 90, 480, 230, 15, RED);
              tft.fillRect(15, 105, 450, 200, WHITE);
              displayString(159, 117, FONT_12PT_BLACK_ON_WHITE, (char *) "Overwrite");
              displayString(40, 150, FONT_9PT_BLACK_ON_WHITE, (char *) "Do you want to overwrite the");
              displayString(40, 180, FONT_9PT_BLACK_ON_WHITE, (char *) "learned values?");
              clearTouchTargets();
              drawTouchButton(60, 230, 160, 60, BUTTON_LARGE_FONT, (char *) "Yes");
              drawTouchButton(260, 230, 160, 43, BUTTON_LARGE_FONT, (char *) "No");
              if (getTap(SHOW_TEMPERATURE_IN_HEADER) == 0)
                prefs.learningComplete = LEARNING_BYPASSED;
            }

            // Save the values if learning not done or has been bypassed
            if (prefs.learningComplete == LEARNING_NOT_DONE || prefs.learningComplete == LEARNING_BYPASSED) {
              // User wants to overwrite learned values
              prefs.learningComplete = LEARNING_BYPASSED;
              prefs.learnedPower = powerMap[bypassPower];
              prefs.learnedInertia = inertiaMap[bypassInertia];
              prefs.learnedInsulation = insulationMap[bypassInsulation];
              savePrefs();
              SerialUSB.println("Saved user-selected learned values:");
              SerialUSB.println("Power=" + String(prefs.learnedPower) + "  Inertia=" + String(prefs.learnedInertia) + "  Insulation=" + String(prefs.learnedInsulation));
            }

            // Break out of the while loop
            break;
          }
        }
        break;

      case SCREEN_LEARNING:
        // Draw the screen
        displayHeader((char *) "Learn", false);
        
        // Are numbers available?  If so, display them
        switch (prefs.learningComplete) {
          case LEARNING_NOT_DONE:
            displayString(10, LINE(0), FONT_9PT_BLACK_ON_WHITE, (char *) "A learning run is necessary to measure");
            displayString(10, LINE(1), FONT_9PT_BLACK_ON_WHITE, (char *) "the performance of the heating");
            displayString(10, LINE(2), FONT_9PT_BLACK_ON_WHITE, (char *) "elements and insulation.  Learning");
            displayString(10, LINE(3), FONT_9PT_BLACK_ON_WHITE, (char *) "will take around 25 minutes.");
            break;
          case LEARNING_DONE:
            showLearnedNumbers();
            break;
          case LEARNING_BYPASSED:
            displayString(10, LINE(0), FONT_9PT_BLACK_ON_WHITE, (char *) "PID tuning has been entered manually");
            displayString(10, LINE(1), FONT_9PT_BLACK_ON_WHITE, (char *) "in the Settings menu.  You can have");
            displayString(10, LINE(2), FONT_9PT_BLACK_ON_WHITE, (char *) "your oven learn these numbers instead.");
            displayString(10, LINE(3), FONT_9PT_BLACK_ON_WHITE, (char *) "It will take around 25 minutes.");
            break;
        }

        drawTouchButton(100, 180, 280, 164, BUTTON_SMALL_FONT, (char *) "Start Learning");
        drawNavigationButtons(false, true);

        // Act on the tap
        switch(getTap(NO_SLEEP)) {
          case 0: learn(); break;
          case 1: screen = SCREEN_SETTINGS; break;
          case 2: screen = SCREEN_HOME; break;
          case 3: if (prefs.learningComplete == LEARNING_DONE)
                    showHelp(SCREEN_RESULTS);
                  else
                    showHelp(SCREEN_LEARNING);
                  goto redraw;
        }
        break;            
    }  // end of switch  
  } // end of while (1)
}


void drawTouchButton(uint16_t x, uint16_t y, uint16_t width, uint16_t textWidth, boolean useLargeFont, char *text) {
  drawButton(x, y, width, textWidth, useLargeFont, text);
  defineTouchArea(x, y, width, BUTTON_HEIGHT);
}


// Draw the naviation buttons at the bottom of the screen
// Use large tap targets if possible
void drawNavigationButtons(boolean addRightArrow, boolean largeTargets)
{
  uint16_t top, height;
  top = largeTargets? 239: 258;
  height = largeTargets? 80: 61;
  
  // Left Arrow button
  renderBitmap(BITMAP_LEFT_ARROW, 10, 275);
  defineTouchArea(0, top, 138, height);
  // Home button
  renderBitmap(BITMAP_HOME, 172, 275);
  defineTouchArea(139, top, 100, height);
  // Help button
  renderBitmap(BITMAP_HELP, 276, 275);
  defineTouchArea(240, top, 100, height);
  if (addRightArrow) {
    // Right Arrow button
    renderBitmap(BITMAP_RIGHT_ARROW, 378, 275);
    defineTouchArea(341, top, 138, height);
  }
}


// Erase the screen title
void eraseHeader()
{
  tft.fillRect(10, 5, 250, 32, WHITE);
}


// Display the screen title
void displayHeader(char *text, boolean isSetting)
{
  uint8_t offset = 10;

  // Should the "Settings" icon be added to the header?
  if (isSetting) {
    renderBitmap(BITMAP_SETTINGS, 10, 1);
    offset += 45;
  }
  displayString(offset, 5, FONT_12PT_BLACK_ON_WHITE, text);
  tft.fillRect(10, 38, 460, 4, LIGHT_GREY);
}


void drawThickRectangle(uint16_t x, uint16_t y, uint16_t width, uint16_t height, uint8_t thickness, uint16_t color)
{
  // Top bar
  tft.fillRect(x, y, width, thickness, color);
  // Left bar
  tft.fillRect(x, y + thickness, thickness, height - thickness, color);
  // Right bar
  tft.fillRect(x + width - thickness, y + thickness, thickness, height - thickness, color);
  // Bottom bar
  tft.fillRect(x + thickness, y + height - thickness, width - thickness - thickness, thickness, color);
}


// Draw arrow for increasing / decreasing settings - and add tap targets for them
void drawIncreaseDecreaseTapTargets(uint8_t targetType)
{
  // There are two settings that can be changed on this screen
  if (targetType == TWO_SETTINGS) {
    renderBitmap(BITMAP_DECREASE_ARROW, 123, LINE(1));
    renderBitmap(BITMAP_INCREASE_ARROW, 270, LINE(1));
    defineTouchArea(80, LINE(0), 160, 85);
    defineTouchArea(241, LINE(0), 160, 85);
    renderBitmap(BITMAP_DECREASE_ARROW, 123, LINE(4));
    renderBitmap(BITMAP_INCREASE_ARROW, 270, LINE(4));
    defineTouchArea(80, LINE(3), 160, 85);
    defineTouchArea(241, LINE(3), 160, 85);
  }
  else {
    // There is one setting that can be changed on this screen
    renderBitmap(BITMAP_DECREASE_ARROW, 123, targetType == ONE_SETTING_WITH_TEXT? LINE(3):LINE(2));
    renderBitmap(BITMAP_INCREASE_ARROW, 270, targetType == ONE_SETTING_WITH_TEXT? LINE(3):LINE(2));
    defineTouchArea(80, LINE(1), 160, targetType == ONE_SETTING_TEXT_BUTTON? 100:140);
    defineTouchArea(241, LINE(1), 160, targetType == ONE_SETTING_TEXT_BUTTON? 100:140);
  }
}


#define TEST_ICON_X      400
#define TEST_ICON_Y      50

uint8_t testOutputType;

// Used when user is testing their relays
void drawTestOutputIcon(uint8_t type, boolean outputIsOn)
{
  // Stop any touch callback used to animate icons
  setTouchIntervalCallback(0, 0);
  
  // Display an icon corresponding to the output
  switch(type) {
    case TYPE_UNUSED:
      // Erase the icon that might have been there
      tft.fillRect(TEST_ICON_X, TEST_ICON_Y, 32, 32, WHITE);
      break;
    case TYPE_BOTTOM_ELEMENT:
    case TYPE_TOP_ELEMENT:
    case TYPE_BOOST_ELEMENT:
      // Display the heating element bitmap
      renderBitmap(outputIsOn? BITMAP_ELEMENT_ON: BITMAP_ELEMENT_OFF, TEST_ICON_X, TEST_ICON_Y);
      break;
    default:
      // This is either a convection or cooling fan
      testOutputType = type;
      // Start a touch callback to animate the icon (15Hz = 67ms) if the output is on
      if (outputIsOn)
        setTouchIntervalCallback(testOutputIconAnimator, 67);
      // Call the callback now to display the icon
      testOutputIconAnimator();
  }
}


void testOutputIconAnimator()
{
  static uint8_t animationPhase = 0;

  // Draw the icon corresponding to the output
  if (testOutputType == TYPE_CONVECTION_FAN)
    renderBitmap(BITMAP_CONVECTION_FAN1 + animationPhase, TEST_ICON_X, TEST_ICON_Y);
  else
    renderBitmap(BITMAP_COOLING_FAN1 + animationPhase, TEST_ICON_X, TEST_ICON_Y);
  // Move to the next animation phase
  animationPhase = (animationPhase + 1) % 3;
}


// Pass in an array of increasing numbers and it will return the one that "value" is just smaller than
uint8_t mapValue(uint16_t value, uint16_t map[])
{
  uint8_t i = 0;
  while (map[i] != 0 && value > map[i])
    i++;
  if (map[i] == 0)
    i--;
//  SerialUSB.println("Value " + String(value) + " was mapped to " + String(i) + " = " + String(map[i]));
  return i;
} 

void enterSleep(){
  // Set the entire screen to Black
  tft.fillScreen(BLACK);
  // Clear the existing touch targets and add one that takes up the entire screen
  clearTouchTargets();
  defineTouchArea(0, 0, 479, 319);
  // Wait for any tap input to exit sleep
  while(getTap(CHECK_FOR_TAP_THEN_EXIT) == -1)
    delay(25);
  // Reset the screen to white and trigger a redraw after input detected
  tft.fillScreen(WHITE);
}
