#include "LowPower.h"

#include <Watchdog.h>
Watchdog watchdog;

#define PIN_TX1 7
#define PIN_EN_TX1 12

#define Dual_Channel
//#define DFCW_TX1
#ifdef Dual_Channel
  //#define DFCW_TX2
#endif

// #ifdef DFCW_TX1
//   #define DF_TX1 8
// #endif

#ifdef Dual_Channel
  #define PIN_TX2 6
  #define PIN_EN_TX2 12
  
  // #ifdef DFCW_TX2
  //   #define DF_TX2 9
  // #endif
#endif

#define TempSensor
#ifdef TempSensor
  #include <OneWire.h>
  #include <DallasTemperature.h>
  #define ONE_WIRE_BUS 2
  OneWire oneWire (ONE_WIRE_BUS); // Setup a oneWire instance to communicate with any OneWire devices (not just Maxim/Dallas temperature ICs)
  DallasTemperature sensors (&oneWire); // Pass our oneWire reference to Dallas Temperature.
#endif

//#define VoltageControl
#ifdef VoltageControl
  #define ADC_pin A7
#endif


#define CW_WPM 16
uint16_t duration = 60000/(CW_WPM*50);    // Morse code typing speed in WPM

static char Channel_Num = 0;

#define TimeInterval 5 // Beacon interval in seconds
#define TimeBeaconInterval 2 // Beacon interval in seconds between channel

String Base_CW__Message = "VVV de R2AKT/B KO85XW";   // Your base message

//----------
void _delay (unsigned long DelayTime) {
  for (unsigned int DelayCount = 0; DelayCount <= (DelayTime/SLEEP_15MS); DelayCount++) {
    LowPower.idle (SLEEP_15MS, ADC_OFF, TIMER2_OFF, TIMER1_OFF, TIMER0_OFF, SPI_OFF, USART0_OFF, TWI_OFF);
  }
}
//----------
void setup () {
  pinMode (PIN_TX1, OUTPUT);
  digitalWrite (PIN_TX1, LOW);
 #ifdef Dual_Channel
    pinMode (PIN_TX2, OUTPUT);
    digitalWrite (PIN_TX2, LOW);
 #endif

  #ifdef VoltageControl
    analogReference (DEFAULT); // ADC reference
  #endif
  
  pinMode (PIN_EN_TX1, OUTPUT);
  digitalWrite (PIN_EN_TX1, LOW);
  #ifdef Dual_Channel
    pinMode (PIN_EN_TX2, OUTPUT);
    digitalWrite (PIN_EN_TX2, LOW);
  #endif
  
  #ifdef TempSensor
    #ifdef DEBUG
      Serial.begin (9600);
      Serial.print ("Locating sensor devices...");
      sensors.begin();
      Serial.print ("Found ");
      Serial.print (sensors.getDeviceCount(), DEC);
      Serial.println (" devices.");
    #endif
  #endif

  // Setup watchdog
  watchdog.enable (Watchdog::TIMEOUT_8S);
}
//----------
void loop () {
  String cw_message = Base_CW__Message;
  
  //
  // do {
  //   digitalWrite (PIN_EN_TX1, HIGH); // Power On Generator 1
  //   digitalWrite (PIN_EN_TX2, HIGH); // Power On Generator 2
  //   digitalWrite (PIN_TX1, HIGH);
  //   digitalWrite (PIN_TX2, HIGH);
  //   digitalWrite (13, HIGH);
  // } while (1);
  //
  #ifdef TempSensor
    sensors.requestTemperatures(); // Send the command to get temperatures
    float tempC = sensors.getTempCByIndex(0);
    if (tempC != DEVICE_DISCONNECTED_C) {
      #ifdef DEBUG
        Serial.print("Temperature for the device 1 (index 0) is: ");
        Serial.println (tempC);
      #endif
      cw_message += " / Temp = " + String (tempC);
    } else {
      #ifdef DEBUG
        Serial.println("Error: Could not read temperature data");
        #endif
    }
  #endif

  #ifdef VoltageControl
  int Volt = analogRead (ADC_pin);
    cw_message += " / Volt = " + String ((float)(Volt*5)/1024);
  #endif
  
  #ifdef DEBUG
    Serial.println ();
  #endif

  cw_message.toUpperCase(); // Changes all letters to upper case

  digitalWrite (PIN_EN_TX1, HIGH); // Power On Generator 1
  Channel_Num = 1;
  delay (2000); // Wait generator
   
  cw (true);
  delay (2000);                         // Duration of the long signal at the end - in milliseconds

  cw (false);
  delay (1000);                          // Duration of the pause at the end after the long signal - in milliseconds
  
  cw_string_proc (cw_message);
  delay (500);                           // Duration of the break at the end before the long signal - in milliseconds

  cw (true);
  delay (2000);                         // Duration of the long signal at the end - in milliseconds

  cw(false);
  delay (1000);                          // Duration of the pause at the end after the long signal - in milliseconds

  Channel_Num = 0;
  digitalWrite (PIN_EN_TX1, LOW); // Power Off Generator 1
  
  #ifdef Dual_Channel
    for (int BeaconInterval = 0; BeaconInterval < TimeBeaconInterval; BeaconInterval++) {
      LowPower.idle (SLEEP_1S, ADC_OFF, TIMER2_OFF, TIMER1_OFF, TIMER0_OFF, SPI_OFF, USART0_OFF, TWI_OFF);
      watchdog.reset();
    }
    
    digitalWrite (PIN_EN_TX2, HIGH); // Power On Generator 2
    Channel_Num = 2;
    delay (2000); // Wait generator
     
    cw (true);
    delay (2000);                         // Duration of the long signal at the end - in milliseconds
  
    cw (false);
    delay (1000);                          // Duration of the pause at the end after the long signal - in milliseconds
    
    cw_string_proc (cw_message);
    delay (500);                           // Duration of the break at the end before the long signal - in milliseconds
  
    cw (true);
    delay (2000);                         // Duration of the long signal at the end - in milliseconds
  
    cw(false);
    delay (1000);                          // Duration of the pause at the end after the long signal - in milliseconds
  
    Channel_Num = 0;
    digitalWrite (PIN_EN_TX2, LOW); // Power Off Generator 2
  #endif

  for (int DelayCount = 0; DelayCount < TimeInterval; DelayCount++) {
    LowPower.idle (SLEEP_1S, ADC_OFF, TIMER2_OFF, TIMER1_OFF, TIMER0_OFF, SPI_OFF, USART0_OFF, TWI_OFF);
    watchdog.reset();
  }
}
//----------
void cw_string_proc (String str) {                      // Processing string to characters
  for (uint8_t j = 0; j < str.length (); j++)
    cw_char_proc (str[j]);
}
//----------
void cw_char_proc (char m) {                            // Processing characters to Morse symbols
  String s;

  if (m == ' ') {                                      // Pause between words
    word_space ();
    return;
  }

  if (m > 96)                                          // ACSII, case change a-z to A-Z
    if (m < 123)
      m -= 32;

  switch (m) {                                         // Morse
    case 'A': s = ".-#";     break;
    case 'B': s = "-...#";   break;
    case 'C': s = "-.-.#";   break;
    case 'D': s = "-..#";    break;
    case 'E': s = ".#";      break;
    case 'F': s = "..-.#";   break;
    case 'G': s = "--.#";    break;
    case 'H': s = "....#";   break;
    case 'I': s = "..#";     break;
    case 'J': s = ".---#";   break;
    case 'K': s = "-.-#";    break;
    case 'L': s = ".-..#";   break;
    case 'M': s = "--#";     break;
    case 'N': s = "-.#";     break;
    case 'O': s = "---#";    break;
    case 'P': s = ".--.#";   break;
    case 'Q': s = "--.-#";   break;
    case 'R': s = ".-.#";    break;
    case 'S': s = "...#";    break;
    case 'T': s = "-#";      break;
    case 'U': s = "..-#";    break;
    case 'V': s = "...-#";   break;
    case 'W': s = ".--#";    break;
    case 'X': s = "-..-#";   break;
    case 'Y': s = "-.--#";   break;
    case 'Z': s = "--..#";   break;

    case 'А': s = ".-#";     break;
    case 'Б': s = "-...#";   break;
    case 'В': s = ".--#";    break;
    case 'Г': s = "--.#";    break;
    case 'Д': s = "-..#";    break;
    case 'Е': s = ".#";      break;
    case 'Ё': s = ".#";      break;
    case 'Ж': s = "...-#";   break;
    case 'З': s = "--..#";   break;
    case 'И': s = "..#";     break;
    case 'Й': s = ".---#";   break;
    case 'К': s = "-.-#";    break;
    case 'Л': s = ".-..#";   break;
    case 'М': s = "--#";     break;
    case 'Н': s = "-.#";     break;
    case 'О': s = "---#";    break;
    case 'П': s = ".--.#";   break;
    case 'Р': s = ".-.#";    break;
    case 'С': s = "...#";    break;
    case 'Т': s = "-#";      break;    
    case 'У': s = "..-#";    break;
    case 'Ф': s = "..-.#";   break;
    case 'Х': s = "....#";   break;
    case 'Ц': s = "-.-.#";   break;
    case 'Ч': s = "---.#";   break;
    case 'Ш': s = "----#";   break;
    case 'Щ': s = "--.-#";   break;
    case 'Ь': s = "-..-#";   break;
    case 'Ы': s = "-.--#";   break;
    case 'Ъ': s = "-..-#";   break;
    case 'Э': s = "..-..#";   break;
    case 'Ю': s = "..--#";   break;
    case 'Я': s = ".-.-#";   break;

    case '1': s = ".----#";  break;
    case '2': s = "..---#";  break;
    case '3': s = "...--#";  break;
    case '4': s = "....-#";  break;
    case '5': s = ".....#";  break;
    case '6': s = "-....#";  break;
    case '7': s = "--...#";  break;
    case '8': s = "---..#";  break;
    case '9': s = "----.#";  break;
    case '0': s = "-----#";  break;

    case '-': s = "-....-#"; break;
    case '+': s = ".-.-.#"; break;
    case '=': s = "-...-#"; break;
    case '.': s = ".-.-.-#"; break;
    case ',': s = "--..--#"; break;
    case '?': s = "..--..#"; break;
    case '/': s = "-..-.#";  break;
    case ':': s = "---...#"; break;
    case '@': s = ".--.-.#"; break;
    case '%': s = "-..-.#"; break;
    case '(': s = "-.--.#"; break;
    case ')': s = "-.--.-#"; break;  
  }

  for (uint8_t i = 0; i < 7; i++) {
    switch (s[i]) {
      case '.': di ();  break;                          // di
      case '-': dah ();  break;                          // dah
      case '#': char_space (); return;                  // end of morse symbol
    }
  }
}
//----------
void di () {
  cw (true);                                            // TX di
  delay(duration);

  cw (false);                                           // stop TX di
  delay (duration);
}
//----------
void dah() {
  cw (true);                                            // TX dah
  delay (3 * duration);

  cw (false);                                           // stop TX dah
  delay (duration);
}
//----------
void char_space () {                                    // 3x, pause between letters
  delay  (2 * duration);                                 // 1x from end of character + 2x from the beginning of new character
}
//----------
void word_space() {                                    // 7x, pause between words
  delay (6 * duration);                                 // 1x from end of the word + 6x from the beginning of new word
}
//----------
void cw (bool state) {                                  // TX-CW, TX-LED
  watchdog.reset();
  if (state) {
    if (Channel_Num == 1) {
      digitalWrite (PIN_TX1, HIGH);
      digitalWrite (13, HIGH);
       #ifdef DFCW_TX1
        digitalWrite (DF_TX1, LOW);
      #endif
    }
    #ifdef Dual_Channel
      if (Channel_Num == 2) {
        digitalWrite (PIN_TX2, HIGH);
        digitalWrite (13, HIGH);
        #ifdef DFCW_TX2
          digitalWrite (DF_TX2, LOW);
        #endif
      }
    #endif
  } else {
    if (Channel_Num == 1) {
      #ifdef DFCW_TX1
        digitalWrite (DF_TX1, HIGH);
      #else
        digitalWrite (PIN_TX1, LOW);
        digitalWrite (13, LOW);
      #endif
    }
    #ifdef Dual_Channel
      if (Channel_Num == 2) {
        #ifdef DFCW_TX2
          digitalWrite (DF_TX2, HIGH);
        #else
          digitalWrite (PIN_TX2, LOW);
          digitalWrite (13, LOW);
        #endif
      }
    #endif
  }
}
