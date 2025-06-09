//#include <TFT_eSPI.h>
//
//TFT_eSPI tft = TFT_eSPI();
//TFT_eSprite moonSprite = TFT_eSprite(&tft);
//
//void setup() {
//  tft.init();
//  tft.setRotation(1); // Adjust as needed
//  moonSprite.createSprite(128, 128); // Example size
//}
//
//void drawMoon(float latitude) {
//  moonSprite.fillScreen(TFT_BLACK);
//
//  // Calculate rotation angle using sinusoidal approximation
//  float theta = 90.0 * sin(PI * latitude / 180.0);
//
//  // Apply rotation
//  moonSprite.pushRotated(&moonSprite, theta, TFT_BLACK);
//
//  moonSprite.pushSprite(0, 0);
//}
//
//void loop() {
//  for (flost latitude = 90.; latitude < -90.; latitude -= 5.) {
//    //    float latitude = 45.0; // Example latitude
//    drawMoon(latitude);
//    delay(1000);
//  }
//}
