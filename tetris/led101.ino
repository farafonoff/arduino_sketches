#include <FastGPIO.h>
#include <util/delay.h>

#define vstrobe 3
#define vdata 7
#define vclock 2
#define hstrobe 5
#define hdata 4
#define hclock 6
#define xenable 8

#define keyL 13
#define keyR 11
#define ROT 12

const uint64_t LETTERS[] = {
  0x6666667e66663c00,
  0x3e66663e66663e00,
  0x3c66060606663c00,
  0x3e66666666663e00,
  0x7e06063e06067e00,
  0x0606063e06067e00,
  0x3c66760606663c00,
  0x6666667e66666600,
  0x3c18181818183c00,
  0x1c36363030307800,
  0x66361e0e1e366600,
  0x7e06060606060600,
  0xc6c6c6d6feeec600,
  0xc6c6e6f6decec600,
  0x3c66666666663c00,
  0x06063e6666663e00,
  0x603c766666663c00,
  0x66361e3e66663e00,
  0x3c66603c06663c00,
  0x18181818185a7e00,
  0x7c66666666666600,
  0x183c666666666600,
  0xc6eefed6c6c6c600,
  0xc6c66c386cc6c600,
  0x1818183c66666600,
  0x7e060c1830607e00,
  0x0000000000000000,
  0x7c667c603c000000,
  0x3e66663e06060600,
  0x3c6606663c000000,
  0x7c66667c60606000,
  0x3c067e663c000000,
  0x0c0c3e0c0c6c3800,
  0x3c607c66667c0000,
  0x6666663e06060600,
  0x3c18181800180000,
  0x1c36363030003000,
  0x66361e3666060600,
  0x1818181818181800,
  0xd6d6feeec6000000,
  0x6666667e3e000000,
  0x3c6666663c000000,
  0x06063e66663e0000,
  0xf0b03c36363c0000,
  0x060666663e000000,
  0x3e403c027c000000,
  0x1818187e18180000,
  0x7c66666666000000,
  0x183c666600000000,
  0x7cd6d6d6c6000000,
  0x663c183c66000000,
  0x3c607c6666000000,
  0x3c0c18303c000000
};
const int LETTERS_LEN = sizeof(LETTERS) / 8;


#define LINES 8
#define COLS 24
#define SIZE ( LINES * COLS )
unsigned char screen[SIZE];
char text[] = "END";
int text_len = strlen(text);

const unsigned int TETRS[] = {
  0x0660,
  0x4444,
  0x0644,
  0x0622,
  0x0264,
  0x0462,
  0x0270
};

unsigned int tetr_c;
unsigned int cxoffset;
unsigned int cyoffset;
unsigned int tetr_n;
int gameover = 0;
long lastMillis;
long lastKeyMillis;

const int TETRS_LEN = sizeof(TETRS) / sizeof(unsigned int);

void putPixel(int y, int x, char level) {
  screen[y * COLS + x] = level;
}

char getPixel(int y, int x) {
  return screen[y * COLS + x];
}

void gradient() {
  int scale = 255 / COLS;
  for (int x = 0; x < COLS; ++x) {
    for (int y = 0; y < LINES; ++y) {
      putPixel(y, x, x * scale);
    }
  }
}

void fill() {
  for (int x = 0; x < COLS; ++x) {
    for (int y = 0; y < LINES; ++y) {
      putPixel(y, x, 255);
    }
  }
}

void frame() {
  for (int x = 0; x < COLS; ++x) {
    for (int y = 0; y < LINES; ++y) {
      if (x < 6 || y == 0 || y == LINES - 1 || x == COLS - 1) {
        if (x > 0 && x < 5 && y > 2 && y < LINES - 1) {
          putPixel(y, x, LOW);
        } else {
          putPixel(y, x, HIGH);
        }
      } else {
        putPixel(y, x, LOW);
      }
    }
  }
}

void tetrBlit(int xoffset, int yoffset, unsigned int tetr, char V) {
  for (int rn = 0; rn < 4; ++rn) {
    byte row = (tetr >> rn * 4) & 0xF;
    for (int j = 0; j < 4; ++j) {
      char xbit = bitRead(row, j);
      if (xbit) {
        putPixel(xoffset + rn, yoffset + j, V);
      }
    }
  }
}

bool tetrCanBlit(int xoffset, int yoffset, unsigned int tetr) {
  bool result = true;
  for (int rn = 0; rn < 4; ++rn) {
    byte row = (tetr >> rn * 4) & 0xF;
    for (int j = 0; j < 4; ++j) {
      char xbit = bitRead(row, j);
      if (xbit) {
        result = result && (!getPixel(xoffset + rn, yoffset + j));
      }
    }
  }
  return result;
}

int nextT() {
  return random(TETRS_LEN);
}

void generate() {
  tetrBlit(3, 1, TETRS[tetr_n], LOW);
  tetr_c = TETRS[tetr_n];
  tetr_n = nextT();
  tetrBlit(3, 1, TETRS[tetr_n], HIGH);
  cxoffset = 2;
  cyoffset = 6;
  if (!tetrCanBlit(cxoffset, cyoffset + 1, tetr_c)) {
    gameover = 1;
  }
}

void shiftLine(int el) {
  Serial.println(el);
  for (int x = el; x > 6; --x) {
    for (int y = 1; y < LINES-1; ++y) {
      putPixel(y,x,getPixel(y,x-1));
    }
  }  
}

void shiftLines() {
  for (int x = 6; x < COLS-1; ++x) {
    int del = 1;
    for (int y = 1; y < LINES-1; ++y) {
      if (getPixel(y,x)==LOW) {
        del = 0;
      }
    }
    if (del==1) {
      shiftLine(x);      
    }
  }  
}

void stepGame() {
  tetrBlit(cxoffset, cyoffset, tetr_c, LOW);
  if (tetrCanBlit(cxoffset, cyoffset + 1, tetr_c)) {
    cyoffset += 1;
    tetrBlit(cxoffset, cyoffset, tetr_c, HIGH);
  } else {
    tetrBlit(cxoffset, cyoffset, tetr_c, HIGH);
    shiftLines();
    generate();
  }
}

void setup() {
  Serial.begin(9600);
  pinMode(vstrobe, OUTPUT);
  pinMode(vdata, OUTPUT);
  pinMode(vclock, OUTPUT);
  pinMode(hstrobe, OUTPUT);
  pinMode(hdata, OUTPUT);
  pinMode(hclock, OUTPUT);
  pinMode(xenable, OUTPUT);
  pinMode(keyL, INPUT_PULLUP);
  pinMode(keyR, INPUT_PULLUP);
  pinMode(ROT, INPUT_PULLUP);
  digitalWrite(hstrobe, HIGH);
  digitalWrite(vstrobe, HIGH);
  memset(screen, LOW, SIZE);
  randomSeed(analogRead(0));
  //putPixel(1,1,LOW);
  frame();
  //tetrBlit(3, 1, TETRS[0]);
  lastMillis = millis();
  lastKeyMillis = millis();
  tetr_n = nextT();
  generate();
}

int dd = 1;
int DELAY = 100;

void pushH() {
  //digitalWrite(hclock, HIGH);
  //digitalWrite(hclock, LOW);
  _delay_us(1);
  FastGPIO::Pin<hclock>::setOutput(HIGH);
  FastGPIO::Pin<hclock>::setOutput(LOW);
}

void pushV() {
  _delay_us(1);
  FastGPIO::Pin<vclock>::setOutput(HIGH);
  FastGPIO::Pin<vclock>::setOutput(LOW);
}

/*void drawCol(int n) {
  int start = n*LINES;
  for(int i=0;i<LINES;++i) {
    digitalWrite(hdata, screen[start+i]);
    pushH();
  }
  delay(1);
  }
*/

/* raster row */
void drawSoftRasterRow(int n) {
  int start = n * COLS;
  for (int i = 0; i < COLS; ++i) {
    unsigned char value = screen[start + i];
    unsigned char rnd = random(0, 255);
    FastGPIO::Pin<vdata>::setOutput(rnd < value ? LOW : HIGH);
    pushV();
  }
}
void drawHardRasterRow(int n) {
  int start = n * COLS;
  for (int i = 0; i < COLS; ++i) {
    unsigned char value = screen[start + i];
    FastGPIO::Pin<vdata>::setOutput(value != 0 ? LOW : HIGH);
    pushV();
  }
}

void completeRow(int points) {
  for (int i = 0; i < points; ++i) {
    pushV();
  }
}

/* text row */
void drawTextRowNormal(int n) {
  int i = 0;
  while (i < COLS) {
    int ch = i / 8;
    if (ch >= text_len) {
      FastGPIO::Pin<vdata>::setOutput(HIGH);
      pushV();
      ++i;
      continue;
    }
    char out = text[2 - ch];
    char letter;
    if (out >= 'A' && out <= 'Z') {
      letter = out - 'A';
    }
    if (out >= 'a' && out <= 'z') {
      letter = out - 'a' + ('Z' - 'A' + 2);
    }
    uint64_t image = LETTERS[letter];
    byte row = (image >> n * 8) & 0xFF;
    for (int j = 7; j >= 0; --j) {
      FastGPIO::Pin<vdata>::setOutput(!bitRead(row, j));
      pushV();
      i += 1;
    }
  }
}

void drawTextRowCondensed(int n) {
  int i = 0;
  while (i < COLS) {
    int ch = i / 6;
    if (ch >= text_len) {
      FastGPIO::Pin<vdata>::setOutput(HIGH);
      pushV();
      ++i;
      continue;
    }
    char out = text[3 - ch];
    char letter;
    if (out >= 'A' && out <= 'Z') {
      letter = out - 'A';
    }
    if (out >= 'a' && out <= 'z') {
      letter = out - 'a' + ('Z' - 'A' + 2);
    }
    uint64_t image = LETTERS[letter];
    byte row = (image >> n * 8) & 0xFF;
    for (int j = 6; j >= 1; --j) {
      FastGPIO::Pin<vdata>::setOutput(!bitRead(row, j));
      pushV();
      i += 1;
    }
  }
}


void drawRaster() {
  digitalWrite(hdata, HIGH);
  pushH();
  digitalWrite(hdata, LOW);
  for (int i = 0; i < LINES; ++i) {
    drawHardRasterRow(i);
    FastGPIO::Pin<xenable>::setOutput(HIGH);
    //delay(1);
    _delay_us(500);
    FastGPIO::Pin<xenable>::setOutput(LOW);
    pushH();
  }
}

void drawTextRaster() {
  digitalWrite(hdata, HIGH);
  pushH();
  digitalWrite(hdata, LOW);
  for (int i = 0; i < LINES; ++i) {
    drawTextRowNormal(i);
    FastGPIO::Pin<xenable>::setOutput(HIGH);
    //delay(1);
    _delay_us(500);
    FastGPIO::Pin<xenable>::setOutput(LOW);
    pushH();
  }
}

void shift(int dir) {
  tetrBlit(cxoffset, cyoffset, tetr_c, LOW);
  if (tetrCanBlit(cxoffset+dir, cyoffset, tetr_c)) {
    cxoffset+=dir;    
  }
  tetrBlit(cxoffset, cyoffset, tetr_c, HIGH);
}

void rotate() {
  tetrBlit(cxoffset, cyoffset, tetr_c, LOW);
  unsigned int result = 0;
  for(int rn = 0;rn<4;++rn) {
    char row = (tetr_c >> rn*4) & 0xF;
    unsigned int col = (bitRead(row, 0) << 12) | (bitRead(row, 1) << 8) | (bitRead(row, 2) << 4) | (bitRead(row, 3));
    result = result | (col << rn);
  }
  if (tetrCanBlit(cxoffset, cyoffset, result)) {
     tetr_c = result;
     tetrBlit(cxoffset, cyoffset, tetr_c, HIGH); 
  }
}

unsigned long keyMs[] = {0,0,0};
unsigned long dering = 20;
unsigned long repeat = 100;
bool readKey(int key, int index) {
  int state = digitalRead(key);    
  if (state == HIGH) {
    keyMs[index] = 0;
  }
  else {
    if (keyMs[index] == 0) {
      keyMs[index] = millis();
    } else {
      if (millis()>keyMs[index]&&millis()-keyMs[index]>dering) {
        keyMs[index]=millis()+repeat;
        return true;
      }      
    }
  }
  return false;
}
void readKeys() {
  if (readKey(keyR, 0)) {
    shift(1);
  }
  if (readKey(keyL, 1)) {
    shift(-1);
  }
  if (readKey(ROT, 2)) {
    rotate();
  }
}

void loop() {
  //drawCol(0);
  
  if (gameover == 0) {
    readKeys();
    if ((millis() - lastMillis) > 500) {
      stepGame();
      lastMillis = millis();
    }
    drawRaster();
  } else {
    drawTextRaster();
  }
  //drawRaster();
  /*digitalWrite(hdata, LOW);
    digitalWrite(vdata, dd);*/
  //drawRaster();
}


