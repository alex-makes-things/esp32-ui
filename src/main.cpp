#include <Arduino.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ST7735.h>
#include <SPI.h>
#include <images/home_images.h>
#include <images/nicerlandscape.h>
#include <UIAssist.h>
#include <HardwareAid.h>

#define SDA 21
#define SCL 22
#define DC 23
#define RST 17
#define SCREENHEIGHT 64
#define SCREENWIDTH 128

using namespace ButtonUtils;

SPIClass spi = SPIClass(VSPI);
Adafruit_ST7735 tft = Adafruit_ST7735(&spi, -1, DC, RST);
GFXcanvas16 canvas = GFXcanvas16(SCREENWIDTH, SCREENHEIGHT);


//--------------------------UI SETUP-----------------------------//
Button button1(26);
Button button2(25);
Button button3(27);
std::vector<Button*> buttons = {&button1, &button2, &button3};

Image playTest(HOME_LARGE_TEST_SIZE, HOME_LARGE_TEST_SIZE, home_large_test);
Image smallPlayTest(HOME_SMALL_TEST_SIZE, HOME_SMALL_TEST_SIZE, home_small_test);

Image largeGallery(HOME_LARGE_GALLERY_SIZE, HOME_LARGE_GALLERY_SIZE, home_large_gallery);
Image smallGallery(HOME_SMALL_GALLERY_SIZE, HOME_SMALL_GALLERY_SIZE, home_small_gallery);

Image largeSettings(HOME_LARGE_SETTINGS_SIZE, HOME_LARGE_SETTINGS_SIZE, home_large_settings);
Image smallSettings(HOME_SMALL_SETTINGS_SIZE, HOME_SMALL_SETTINGS_SIZE, home_small_settings);


AnimatedApp play(smallPlayTest, playTest, 64, 32, true, FocusStyle::Outline);
AnimatedApp settings(smallSettings, largeSettings, 25, 32, true, FocusStyle::Outline);
AnimatedApp gallery(smallGallery, largeGallery, 103, 32, true, FocusStyle::Outline);

std::vector<UIElement*> elements = {&play, &settings, &gallery};
Scene home(elements, play);
UI ui(home, canvas);
//--------------------------UI SETUP-----------------------------//

TaskHandle_t serialComms;
void handleComms( void *pvParameters){
  Serial.setTimeout(250);

  while(true){
    if (Serial.available()){
      String input = Serial.readString();
      input.trim();
      if (input == "freemem")
      {
        Serial.printf("Used heap: %fkb / %fkb\n", (static_cast<float>(ESP.getHeapSize()) / 1000.0) - static_cast<float>(ESP.getFreeHeap()) / 1000.0, static_cast<float>(ESP.getHeapSize()) / 1000.0);
      }
      else if(input == "version"){
        Serial.println(ESP.getSdkVersion());
      }
      else if (input == "reset")
      {
        Serial.println("Rebooting...");
        ESP.restart();
      }
      else if (input == "uptime")
      {
        Serial.printf("Uptime: %lu seconds\n", millis() / 1000);
      }
      else if (input == "freq")
      {
        Serial.printf("Frequency: %dMHz\n", ESP.getCpuFreqMHz());
      }
      else if (input == "debugui")
      {
        UIElement *obj = ui.focus.focusedScene->elements.at(ui.focus.focusedElementID);
        Serial.printf("ID: %s\n", ui.focus.focusedElementID.c_str());
        Serial.printf("isAtStart: %s\n", obj->anim.getStart() ? "true" : "false");
        Serial.printf("isDone: %s\n", obj->anim.getDone() ? "true" : "false");
        Serial.printf("isEnabled: %s\n", obj->anim.getIsEnabled() ? "true" : "false");
        Serial.printf("Reverse: %s\n", obj->anim.getDirection() ? "true" : "false");
        Serial.printf("Draw: %s\n", obj->draw ? "true" : "false");
        if(obj->type == ElementType::UIElement)
          Serial.println("Type: UIElement");
        else if (obj->type == ElementType::UIImage)
          Serial.println("Type: UIImage");
        else 
          Serial.println("Type: AnimatedApp");
      }
    }
    vTaskDelay(pdMS_TO_TICKS(100));
  }
}

void setup() {
  setupButtons(buttons);
  Serial.begin(115200);
  spi.begin(SCL, -1, SDA, -1);
  tft.initR(INITR_GREENTAB);
  tft.setSPISpeed(78000000); //Absolute fastest speed tested, errors at 80000000
  tft.fillScreen(ST7735_BLACK);
  #if defined(ESP32)
  xTaskCreatePinnedToCore(handleComms, "Comms", 2000, NULL, 1, &serialComms, 0);
  #endif
  play.outline.border_distance = 2;
  settings.outline.border_distance = 2;
  gallery.outline.border_distance = 2;

  play.outline.radius = 3;
  settings.outline.radius = 3;
  gallery.outline.radius = 3;

  play.outline.thickness = 2;
  settings.outline.thickness = 2;
  gallery.outline.thickness = 2;
}

//-------------BEFORE LOOP----------------//
uint64_t start = micros(), calcStart = micros();
//-------------BEFORE LOOP----------------//


//-------------SETTINGS----------------//
bool render_frametime = true;
unsigned int frameTime=0;
unsigned int calculationsTime=0;
uint16_t debugColor = 0xfc60;

//-------------SETTINGS----------------//

void framerate(bool render){
  if(render){
    canvas.setCursor(0,50);
    canvas.setTextSize(2);
    canvas.setTextColor(ST7735_GREEN);
    canvas.setTextWrap(false);
    canvas.print(frameTime);
  }
}
void computeTime(bool render){
  if(render){
    canvas.setCursor(54,50);
    canvas.setTextSize(2);
    canvas.setTextColor(ST7735_RED);
    canvas.setTextWrap(false);
    canvas.print(calculationsTime);
  }
}
/**************************************************************************/
/*!
   @brief      A blazingly fast method for drawing an RGB bitmap to the screen
    @param    x   Top left corner x coordinate
    @param    y   Top left corner y coordinate
    @param    bitmap  byte array with monochrome bitmap
    @param    w   Width of bitmap in pixels
    @param    h   Height of bitmap in pixels
*/
/**************************************************************************/
inline __attribute__((always_inline))
void fastRender(int16_t x, int16_t y, uint16_t *bitmap, int16_t w, int16_t h)
{
  tft.setAddrWindow(x, y, w, h);
  tft.writePixels(bitmap, 8192, false);
}

//uint16_t buffer[8192];

//Image current(128, 64, canvas.getBuffer());
//Image prev(128, 64, buffer);

void loop() {
    canvas.fillScreen(0x0000); //Fill the background with a black frame

    updateButtons(buttons);  //Update button states for every button
    
    if (button1.clickedOnce && !button2.clickedOnce ) {
      ui.focusDirection(RIGHT, FocusingAlgorithm::Cone);
    }
    if (button2.clickedOnce&& !button1.clickedOnce ) {
      ui.focusDirection(LEFT, FocusingAlgorithm::Cone);
    }

    calcStart = micros();
    ui.render();
    calculationsTime = micros() - calcStart;

    computeTime(render_frametime);
    framerate(render_frametime);  //Render the framerate in the bottom-left corner on top of everything

    //if(!dirtyRects(prev, current))
    fastRender(0,0,canvas.getBuffer(),SCREENWIDTH,SCREENHEIGHT); //RENDER THE FRAME

    
  //TEMPORAL VARIABLES AND FUNCTIONS
    rememberButtons(buttons);
    ui.update();
    frameTime = micros()-start;
    Serial.println(frameTime);
    start = micros();
}