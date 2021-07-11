#include "funshield.h"

constexpr byte LETTER_GLYPH[] {
  0b10001000,   // A
  0b10000011,   // b
  0b11000110,   // C
  0b10100001,   // d
  0b10000110,   // E
  0b10001110,   // F
  0b10000010,   // G
  0b10001001,   // H
  0b11111001,   // I
  0b11100001,   // J
  0b10000101,   // K
  0b11000111,   // L
  0b11001000,   // M
  0b10101011,   // n
  0b10100011,   // o
  0b10001100,   // P
  0b10011000,   // q
  0b10101111,   // r
  0b10010010,   // S
  0b10000111,   // t
  0b11000001,   // U
  0b11100011,   // v
  0b10000001,   // W
  0b10110110,   // ksi
  0b10010001,   // Y
  0b10100100,   // Z
};
constexpr byte EMPTY_GLYPH = 0b11111111;

constexpr int btn1 = 0;
constexpr int btn2 = 1;
constexpr int btn3 = 2;

enum states {NORMAL, CONFIG, GENERATING};

struct Dice {
  states current_state;
  
  struct Sides {
     int index;
     int possible_sides[6] = {4, 6, 8, 10, 12, 100};
     int size;


     void setUp() {
        size = sizeof(possible_sides)/sizeof(possible_sides[0]);
        index = 0;
     }

     int getSides() {
        return possible_sides[index % size]; 
     }

     void nextDice() {
        index++;
        index %= size; 
     }
  };

  struct Throws {
    int amount;

    void setUp() {
        amount = 1;  
    }

    void increment() {
       amount++;
       if (amount >= 10) {
         amount = 1;  
       }  
    }
  };

  struct Generator{
    unsigned long startingTime;
    
    void setUp(unsigned long now) {
        startingTime = now;
    }

    unsigned int generate(unsigned long now, int sides, int throws) {
        randomSeed(now - startingTime);

        unsigned int sum = 0;
        
        for (int i = 0; i < throws; ++i) {
            sum += random(1, sides + 1);
        }

        return sum;
    }
    
  };

  Throws throws;
  Sides side;
  Generator generator;

  int latestResult;

  void setUp() {
    current_state = states::NORMAL;
    latestResult = 1234;
    throws.setUp();
    side.setUp();
    generator.setUp(0);
  }

  void generate(unsigned long now) {
    if (current_state == states::NORMAL) {
        generator.setUp(now);
        current_state = states::GENERATING;
    }
  }

  void updateLatestResult(unsigned long now) {
      if (current_state == states::GENERATING) {
          latestResult = generator.generate(now, side.getSides(), throws.amount);
          current_state = states::NORMAL;
      }
  }

  void onButtonOnePressed(unsigned long now) {
    if (current_state == states::NORMAL) {
        generate(now);
    } else {
      current_state = states::NORMAL;
    }  
  }

  void onButtonTwoPressed() {
    if (current_state == states::CONFIG) {
       throws.increment();
    } else {
      current_state = states::CONFIG;  
    }
  }

  void onButtonThreePressed() {
    if (current_state == states::CONFIG) {
       side.nextDice();
    } else {
      current_state = states::CONFIG;  
    }
    
  }
  
};

struct Button {
  int pin;
  boolean pressed;
  unsigned long time_of_press;

  Button(int p) {
    pin = p;
    pressed = false;
  }

  void set_up() {
    pinMode(pin, INPUT);
  }

  boolean pressedNow() {
    return digitalRead(pin);
  }

};

struct Buttons {

  Button butts[3] = {Button(button1_pin), Button(button2_pin), Button(button3_pin)};
  int siz = sizeof(butts) / sizeof(butts[0]);

  void set_up() {
    for (int i = 0; i < siz; ++i) {
      butts[i].set_up();
    }
  }

  boolean released(int index) {
      index %= siz;
      boolean pr = butts[index].pressedNow();
      if (pr && butts[index].pressed) {
        butts[index].pressed = false;
        return true;
      }
      
      return false;
  }

  boolean pressed(int index, unsigned long now) {
    index %= siz;
    if (butts[index].time_of_press + 20 <= now) {
      boolean pr = butts[index].pressedNow();
      if (!pr && !butts[index].pressed) {
        butts[index].pressed = true;
        butts[index].time_of_press = now;
        return true;
      } else if (index > 0 && pr && butts[index].pressed) {
         butts[index].pressed = false;  
      }
    }
    return false;
  }

  boolean hold(int index, unsigned long now) {
    index %= siz;
    boolean pr = butts[index].pressedNow();
    if (!pr && butts[index].pressed && butts[index].time_of_press + 100 < now) {
        return true;
    }
    return false;
  }
};

struct SevDis {

  unsigned int showingPosition;
  const int sizeDisplay = sizeof(digit_muxpos) / sizeof(digit_muxpos[0]);

  const int letter_d = 0b10100001;

  const int dividers[4] = {1000, 100, 10, 1};

  const char* runningMessage = "   generating numbers";
  unsigned int lengthOfMessage;
  unsigned long lastUpdate;
  const int space = 300;
  unsigned int endOfShowingMessage;
  
  Dice* dice;
  
  void set_up(Dice *d) {
    pinMode(latch_pin, OUTPUT);
    pinMode(clock_pin, OUTPUT);
    pinMode(data_pin, OUTPUT);
    
    showingPosition = 0;
    dice = d;
    
    lastUpdate = 0;
    endOfShowingMessage = sizeDisplay;
    lengthOfMessage = getStrLength(runningMessage);
  }

  void update(unsigned long now) {
    if (dice->current_state == states::NORMAL) {
      showLatestResult();
    } else if (dice->current_state == states::CONFIG) {
      showConfig();
    }  else if (dice->current_state == states::GENERATING) {
      runGeneratingMessage(now);
    }
  }

  void showConfig() {
    switch (showingPosition % sizeDisplay) {
      case 0:
          WriteNumberToSegment(digit_muxpos[0], digits[dice->throws.amount % 10]);
          break;
          
      case 1:
          displayChar('d', 1);
          break;
          
      case 2:
          WriteNumberToSegment(digit_muxpos[2], digits[(dice->side.getSides()/10) % 10]);
          break;
          
      case 3:
          WriteNumberToSegment(digit_muxpos[3], digits[(dice->side.getSides()) % 10]);
          break;
          
      default:
          break;
    } 
    showingPosition++;
    showingPosition %= sizeDisplay;
  }

  void showLatestResult() {
    int digit = digits[(dice->latestResult / dividers[showingPosition % sizeDisplay]) % 10];
    WriteNumberToSegment(digit_muxpos[showingPosition % sizeDisplay], digit);
    showingPosition++;
    showingPosition %= sizeDisplay;
  }

  void runGeneratingMessage(unsigned long now) {
    if (lastUpdate + space <= now) {
        endOfShowingMessage++;
        endOfShowingMessage %= lengthOfMessage;
        lastUpdate = now;
    }

    showingPosition++;
    showingPosition %= sizeDisplay;
    
    int characterShowing = endOfShowingMessage - sizeDisplay + showingPosition;
    if (characterShowing < 0) {
      characterShowing += lengthOfMessage;  
    }
    characterShowing %= lengthOfMessage;
    
    displayChar(runningMessage[characterShowing], showingPosition);   
  }

  void WriteNumberToSegment(byte Segment, byte Value) {
    digitalWrite(latch_pin, LOW);
    shiftOut(data_pin, clock_pin, MSBFIRST, Value);
    shiftOut(data_pin, clock_pin, MSBFIRST, Segment);
    digitalWrite(latch_pin, HIGH);
  }

  void displayChar(char ch, byte pos) {
      byte glyph = EMPTY_GLYPH;
      if (isAlpha(ch)) {
        glyph = LETTER_GLYPH[ ch - (isUpperCase(ch) ? 'A' : 'a') ];
      }
      
      digitalWrite(latch_pin, LOW);
      shiftOut(data_pin, clock_pin, MSBFIRST, glyph);
      shiftOut(data_pin, clock_pin, MSBFIRST, 1 << pos);
      digitalWrite(latch_pin, HIGH);
  }

  int getStrLength(const char *str) {
    int i= 0;
    while(str[i] != '\0') {
      ++i; 
    }
    return i;
  }
};

Buttons buttons;
Dice dice;
SevDis display;


void setup() {
  buttons.set_up();
  dice.setUp();
  display.set_up(&dice);
}

void loop() {
  unsigned long now = millis();

  if (buttons.released(btn1)) {
    dice.updateLatestResult(now);
    
  } 
 
  
  if (buttons.pressed(btn1, now)) {
    dice.onButtonOnePressed(now);
    
  } else if (buttons.hold(btn1, now)) {
    dice.generate(now);
    
  } else if (buttons.pressed(btn2, now)) {
    dice.onButtonTwoPressed();
    
  } else if (buttons.pressed(btn3, now)) {
    dice.onButtonThreePressed();
    
  }
  
  display.update(now);
}
