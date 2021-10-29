/* THIS IS THE FINAL VERSION OF THIS. REMOVED EVERYTHING BUT THE TREES. NO POT OR BUTTON ON THIS VERSION */



#ifdef __arm__
// should use uinstd.h to define sbrk but Due causes a conflict
extern "C" char* sbrk(int incr);
#else  // __ARM__
extern char *__brkval;
#endif  // __arm__

int freeMemory() {
  char top;
#ifdef __arm__
  return &top - reinterpret_cast<char*>(sbrk(0));
#elif defined(CORE_TEENSY) || (ARDUINO > 103 && ARDUINO != 151)
  return &top - __brkval;
#else  // __arm__
  return __brkval ? &top - __brkval : &top - __malloc_heap_start;
#endif  // __arm__
}

//#include <Adafruit_NeoPixel.h> // 790 bytes
#include <WS2812.h> //783 bytes



// Which pin on the Arduino is connected to the NeoPixels?
#define LED_PIN        6 // On Trinket or Gemma, suggest changing this to 1
#define buttonPin 2
#define MIC_PIN A0
#define POT_PIN A1
#define FLIP_SCREEN true // flip the screen up/down

#define MaxX  24
#define MaxY  20

// How many NeoPixels are attached to the Arduino?
#define NUMPIXELS 385 // Each pixel needs about 3 bytes, so 256 pixels means 768 bytes a bit over 1/3 of the memory on an Uno
int MaxBright = 255; // 0 - 255




#define HUE_ROTATION 0.3; // how quickly we rotate through hues (0.2 is a good number)
#define SPEED 50 // smaller is faster (number of miliseconds between frame, 32 is pretty quick)


// NEW TREE DEFINES

#define MAX_BRANCH_LENGTH 18 // length of branch before it fades 18 is a good number
#define MAX_BRANCHES 15 // Each tree takes about 5 bytes. 20 is a good number
#define TREE_GENERATION_SPEED 15 // speed that new trees are created (smaller is faster) 15 is a good number
#define LOOP_HORZONTAL false
#define WHITE_HEADS true // show a white pixel at the top of each branch
#define JUDDER_REDUCTION 6 // amount to increase chances of trees moving upward (2 means 50/50 chance that the branch won't move up on a particualr frame) 6 is a good number
#define TREE_TYPE 0 // 0 = organic, 1 = digital




int currentSpeed = SPEED;

int lastTreeCreated = 0;

// FACE DEFINES
int facePosX = 0;
int facePosY = 0;

WS2812 LED(NUMPIXELS);
cRGB value;


extern const uint8_t gamma8[];
extern const unsigned char randomnum[];

int debounce = -10;
int buttonState = 0;
int pixelOn = 0;


float globalHue = 0;
unsigned long my_time = 0;
unsigned long next_time = 0;





int getRnd(int rndLoc) {
  rndLoc = rndLoc % 64;
  return randomnum[rndLoc];
}

int rgb_colors[3];

void getRGB(int hue, int sat, int val, int colors[3]);
int getPixelLoc(int x, int y);
void setPixelColor(int loc, int r, int b, int g);


class tree_vector {
  public:
    unsigned char x;
    boolean inUse = false;

    tree_vector(int newtype);
    void render();
    void step();
    boolean isAvailable();
    void spawn(int xPos);
    void destroy();

  private:
    //unsigned char branchGrowthPos[MAX_BRANCH_LENGTH + MaxY]; // make storage for where the branch is going to grow
    unsigned char branchLength = 0;
    unsigned char branchHeight = 0;
    unsigned char luminanceAdj = 100;
    unsigned char startRnd = 0;
    //unsigned char branchAge = 0;

};

tree_vector::tree_vector(int newtype) {
  // type = newtype;
  inUse = false;
}
void tree_vector::spawn(int xPos) {
  x = xPos;
  branchLength = branchHeight = 0;
  luminanceAdj = (rand() % 3) * 25 + (100 - (25 * 2));
  inUse = true;
  startRnd = (rand() % 64);
  //Serial.println("spawn");
}

boolean tree_vector::isAvailable() {
  return !inUse;
}

void tree_vector::render() {
  // render this branch if its inUse
  if (inUse) {
    int rgb_colors[3];
    int tempX, tempY, offsetX, offsetY;
    float tempLum, increment;
    boolean showPixel;
    int rndLoc = startRnd;
    int rndLookup;
    tempX = x;
    tempY = 0;
    tempLum = 0;
    increment = 255 / (float) MAX_BRANCH_LENGTH;
    tempLum = (float)(MAX_BRANCH_LENGTH - branchLength) * increment; // set the starting Luminance (could be negative)

    //Serial.println(" ");
    //Serial.println("------------------");
    for (int i = 0; i <= branchLength; i++) {
      rndLoc += 7;
      showPixel = true;
      //Serial.println("--------");
      //Serial.println(i);
      if (tempLum > -25) {
        //Serial.println(tempLum);
        if (tempX >= MaxX) {
          if (LOOP_HORZONTAL) {
            tempX = tempX - MaxX;
          } else {
            showPixel = false;
          }
        }
        if (tempX < 0 ) {
          if (LOOP_HORZONTAL) {
            tempX = tempX + MaxX;
          } else {
            showPixel = false;
          }
        }
        if (tempY >= MaxY) {
          showPixel = false;
        }
        if (tempY < 0) {
          showPixel = false;
        }
        if (i > branchHeight && branchHeight != 0) {
          showPixel = false;
        }
        if (showPixel) {
          // Serial.println(tempLum);
          if (tempLum > 250 && WHITE_HEADS && luminanceAdj > 70) {
            rgb_colors[0] = rgb_colors[1] = rgb_colors[2] = MaxBright * luminanceAdj / 100;
          } else if (tempLum < 1) {
            rgb_colors[0] = rgb_colors[1] = rgb_colors[2] = 0;
          } else {
            getRGB((int)globalHue, 255, (int) (tempLum * luminanceAdj / 100 * MaxBright / 255), rgb_colors);
          }
          setPixelColor(getPixelLoc(tempX, tempY), pgm_read_byte(&gamma8[rgb_colors[0]]), pgm_read_byte(&gamma8[rgb_colors[1]]), pgm_read_byte(&gamma8[rgb_colors[2]]));
        } else {
          if (branchHeight > i || branchHeight == 0) {
            branchHeight = i;
          } // set to the last item that is off screen
        }
      }
      tempLum += increment; // add brightness into our pixels
      if (branchLength > i && i < MAX_BRANCH_LENGTH + MaxY) {
        // for all but the last branchPart
        if (TREE_TYPE == 0) {  // organic look
          offsetX = getRnd(rndLoc) % 3;
          offsetY = ceil((float)(getRnd(rndLoc + 1) % (JUDDER_REDUCTION + 1)) / JUDDER_REDUCTION) + 1;
          if (offsetX == 0 && offsetY == 1) {
            offsetY++;
          }
          tempX += offsetX - 1;
          tempY += offsetY - 1;





        } else if (TREE_TYPE == 1) { // digital look
          offsetX = getRnd(rndLoc) % 7;
          offsetY = 0;
          offsetX--;
          if (offsetX == 0 || offsetX > 1) {
            offsetY = 1;
            offsetX = 0;
          }
          rndLookup = rndLoc + 2;
          while (getRnd(rndLookup) % 3 > 0) {
            offsetX = getRnd(rndLookup - 2) % 7;
            offsetY = 0;
            offsetX--;
            if (offsetX == 0 || offsetX > 1) {
              offsetY = 1;
              offsetX = 0;
            }
            rndLookup -= 7;
          }
          tempX += offsetX;
          tempY += offsetY;
          /*Serial.print(offsetX);
            Serial.print("/");
            Serial.print(offsetY);
            Serial.print(" : ");*/

        }
      }
    }
  }
  if (branchHeight + MAX_BRANCH_LENGTH < branchLength && branchHeight != 0) {
    // the entire branch is offscreen now
    destroy(); // destroy this branch
  }

}

void tree_vector::step() {
  if (branchLength < MAX_BRANCH_LENGTH + MaxY) {
    //   branchGrowthPos[branchLength] = (newX << 4) | newY;
  } else {
    if (branchHeight == 0) {
      branchHeight = branchLength; // if we're still on screen, but exceeded our space we need to mark it as "complete"
    }
  }
  branchLength++;
}

void tree_vector::destroy() {
  branchLength = 0;
  //branchAge = 0;
  branchHeight = 0;
  inUse = false;
}

tree_vector* TreeVect[MAX_BRANCHES];

void tree_mode() {
  if (rand() % TREE_GENERATION_SPEED < lastTreeCreated) {
    //Serial.println("attempting to create branch");
    generate_branch();
    lastTreeCreated = 0;
  }
  for (int i = 0; i < MAX_BRANCHES; i++) {
    TreeVect[i]->step();
    TreeVect[i]->render();
  }
  lastTreeCreated++;
}

void generate_branch() {
  //generate_branch
  boolean branch_generated = false;
  int xPos = rand() % MaxX;
  for (int i = 0; i < MAX_BRANCHES; i++) {
    if (TreeVect[i]->isAvailable()) {
      TreeVect[i]->spawn(xPos);
      branch_generated = true;
      if ( rand() % 5 > 0 || TREE_TYPE > 0) {
        break;
      }
    }
  }
  if (!branch_generated) {
    //Serial.println("ran out of available branches");
  }
}


//tree_vector TreeVect[1] = {tree_vector(UNUSED_BRANCH)};


void setup() {

  randomSeed(10); // we should find a better seed for this (mic maybe)


  LED.setOutput(LED_PIN);
  Serial.begin(115200);

  for (int i = 0; i < MAX_BRANCHES; i++) {
    TreeVect[i] = new tree_vector(3);
  }

  clearPixels(); // Set all pixel colors to 'off'
  next_time = millis();

  Serial.println("--------------------------------------------");
  Serial.print(" Memory for screen : ");
  Serial.println(NUMPIXELS * 3);
  Serial.print(" Memory for trees : ");
  Serial.println(5 * MAX_BRANCHES);
  Serial.print(" TOTAL  : ");
  Serial.print((5 * MAX_BRANCHES + NUMPIXELS * 3) / 20.48);
  Serial.println("%");
  Serial.println("--------------------------------------------");
  Serial.print(" Available Memory : ");
  Serial.print(freeMemory() / 20.48);
  Serial.print("% (");
  Serial.print(freeMemory());
  Serial.println(" bytes)");
  Serial.println("--------------------------------------------");

 
}

int getPixelLoc(int x, int y) {
  
  /*
  THIS IS FOR A STANDARD NEOPIXEL MATRIX

 int loc;
 
 x = (x + (MaxX * 10)) % MaxX;
  y = (y + (MaxY * 10)) % MaxY;
  loc = x * MaxY;
  if (x % 2 == 1) {
    //odd row
    loc += MaxY - y - 1;
  } else {
    //even row
    loc += y;
  }
  
   //Serial.println(loc);
  return loc;

  */

  // HANDLE SCREEN ROTATION
  switch (FLIP_SCREEN){
      case 1:
        // UP
        y = MaxY - y;
        break;
        
    
  }
  

  

  // THIS IS MY MASK LAYOUT
  int JLEDstart[25] = {0, 11, 23, 35, 49, 64, 80, 97, 115, 134, 153, 173, 193, 213, 233, 252, 270, 288, 305, 321, 336, 350, 362, 374,386}; // STARTING LED NUMBER FOR EACH COLUMN
  short JLEDoffset[24] = {2, 2, 2, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 2, 2, 2}; // OFFSET FOR EACH COLUMN (how far down to the first actual LED

  int JLEDnumber = y-JLEDoffset[x]; // Rough estimate by taking my Y and subtracting the offset (basically aligns the vertical).
  if (JLEDnumber > -1){ // make sure we're not trying to write to a pixel in the dead area above the column
    JLEDnumber+=JLEDstart[x]; // add in the start position to get the actual pixel number
    if (JLEDnumber > JLEDstart[x+1]){ // make sure that it's not in the dead area under the column (if the LED number is in the next column, then we shouldn't be writing to it
      return -1; // return -1 for pixels we can ignore
    }
    return JLEDnumber;
  } else {
    return -1; 
  } 
 }




void setPixelColor(int loc, int r, int b, int g) {
  //.pixels.setPixelColor(loc, pixels.Color(r,g,b));
  if (loc > -1 && loc < NUMPIXELS){ // only load this in if it's a valid LED
    value.r = r;
    value.g = g;
    value.b = b;
    LED.set_crgb_at(loc, value);
  }
}

void showPixels() {
  //pixels.show();
  LED.sync();
  //*LED.ws2812_port_reg |= LED.pinMask;
  //LED.ws2812_sendarray_mask(LED.pixels,3*LED.count_led,LED.pinMask,(uint8_t*) LED.ws2812_port,(uint8_t*) LED.ws2812_port_reg );
}

void clearPixels() {
  //pixels.clear();
  value.r = value.g = value.b = 0;
  for (int i = 0; i < NUMPIXELS; i++)
  {
    LED.set_crgb_at(i, value);
  }
  // Sends the data to the LEDs
  LED.sync();
}

void test_mode() {
  int rgb_colors[3];
  getRGB((int)globalHue, 255, MaxBright, rgb_colors);


  setPixelColor(getPixelLoc(rand() % MaxX, rand() % MaxY), pgm_read_byte(&gamma8[rgb_colors[0]]), pgm_read_byte(&gamma8[rgb_colors[1]]), pgm_read_byte(&gamma8[rgb_colors[2]]));
  setPixelColor(getPixelLoc(rand() % MaxX, rand() % MaxY), 0, 0, 0);

}





/****************************************************************************************
 *                                                                                      *
                                         MAIN LOOP
 *                                                                                      *
 ****************************************************************************************/

void loop() {

  my_time = millis();
  if (next_time < my_time) {
    next_time = my_time + currentSpeed;

    globalHue += HUE_ROTATION;
    if (globalHue < 0 ) {
      globalHue += 360;
    }
    if (globalHue > 359) {
      globalHue -= 360;
    }
    //Serial.println(globalHue);

    tree_mode();
 


    MaxBright = (analogRead(POT_PIN) / 2.5);
    if (MaxBright > 255 || analogRead(POT_PIN) == 0) { // use pot to control brightness. If it's disconnected just set to max.
      MaxBright = 255;
    }

    //Serial.println(potVal);

    showPixels();   // Send the updated pixel colors to the hardware.
    currentSpeed = SPEED;
  }
}


void getRGB(int hue, int sat, int val, int colors[3]) {
  /* convert hue, saturation and brightness ( HSB/HSV ) to RGB
     The dim_curve is used only on brightness/value and on saturation (inverted).
     This looks the most natural.
  */
  val = (val + 256 * 10) % 256; // normalize the values (make sure they're in the 0-255 range
  sat = (sat + 256 * 10) % 256; // normalize the values (make sure they're in the 0-359 range
  hue = (hue + 360 * 10) % 360; // normalize the values (make sure they're in the 0-255 range
  int r;
  int g;
  int b;
  int base;

  if (sat == 0) { // Acromatic color (gray). Hue doesn't mind.
    colors[0] = val;
    colors[1] = val;
    colors[2] = val;
  } else  {

    base = ((255 - sat) * val) >> 8;

    switch (hue / 60) {
      case 0:
        r = val;
        g = (((val - base) * hue) / 60) + base;
        b = base;
        break;

      case 1:
        r = (((val - base) * (60 - (hue % 60))) / 60) + base;
        g = val;
        b = base;
        break;

      case 2:
        r = base;
        g = val;
        b = (((val - base) * (hue % 60)) / 60) + base;
        break;

      case 3:
        r = base;
        g = (((val - base) * (60 - (hue % 60))) / 60) + base;
        b = val;
        break;

      case 4:
        r = (((val - base) * (hue % 60)) / 60) + base;
        g = base;
        b = val;
        break;

      case 5:
        r = val;
        g = base;
        b = (((val - base) * (60 - (hue % 60))) / 60) + base;
        break;
    }

    colors[0] = r;
    colors[1] = g;
    colors[2] = b;
  }
}

const unsigned char randomnum[] = {229, 55, 137, 246, 237, 162, 73, 119, 181, 32, 30, 10, 142, 242, 208, 32, 120, 75, 34, 37, 7, 133, 16, 190, 53, 147, 50, 82, 110, 11, 76, 47, 235, 233, 61, 232, 189, 252, 37, 105, 66, 73, 225, 24, 33, 35, 161, 195, 45, 135, 189, 138, 32, 103, 166, 250, 118, 217, 54, 113, 51, 221, 124, 142};

const uint8_t PROGMEM gamma8[] = {
  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  1,  1,  1,  1,
  1,  1,  1,  1,  1,  1,  1,  1,  1,  2,  2,  2,  2,  2,  2,  2,
  2,  3,  3,  3,  3,  3,  3,  3,  4,  4,  4,  4,  4,  5,  5,  5,
  5,  6,  6,  6,  6,  7,  7,  7,  7,  8,  8,  8,  9,  9,  9, 10,
  10, 10, 11, 11, 11, 12, 12, 13, 13, 13, 14, 14, 15, 15, 16, 16,
  17, 17, 18, 18, 19, 19, 20, 20, 21, 21, 22, 22, 23, 24, 24, 25,
  25, 26, 27, 27, 28, 29, 29, 30, 31, 32, 32, 33, 34, 35, 35, 36,
  37, 38, 39, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 50,
  51, 52, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63, 64, 66, 67, 68,
  69, 70, 72, 73, 74, 75, 77, 78, 79, 81, 82, 83, 85, 86, 87, 89,
  90, 92, 93, 95, 96, 98, 99, 101, 102, 104, 105, 107, 109, 110, 112, 114,
  115, 117, 119, 120, 122, 124, 126, 127, 129, 131, 133, 135, 137, 138, 140, 142,
  144, 146, 148, 150, 152, 154, 156, 158, 160, 162, 164, 167, 169, 171, 173, 175,
  177, 180, 182, 184, 186, 189, 191, 193, 196, 198, 200, 203, 205, 208, 210, 213,
  215, 218, 220, 223, 225, 228, 231, 233, 236, 239, 241, 244, 247, 249, 252, 255
};
