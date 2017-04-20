//**************************************LIB**************************************************
#include <SPI.h>
#include <Wire.h>
#include <Ethernet.h>
#include <RTClib.h>
#include <stdint.h>
#include <LiquidCrystal.h>
#include <TimerFreeTone.h>
#include <SD.h>
#include <Adafruit_Sensor.h>
#include <DHT.h>
#include <DHT_U.h>

//**************************************General Setup**************************************************
#define               nID                (77)                // Sensor number ID
#define               refresh            (1000)              // delay beetween screen refreshes 
#define               refresh2           (60000)             // delay beetween other refreshes
//**************************************DHT22 Setup**************************************************
#define               DHTPIN             (6)                 // connection to PIN
#define               DHTTYPE            DHT22               // DHT 22 (AM2302)
//**************************************Detector Setup**************************************************
#define               waitingtime        (30)                // waiting time before skip connection check
#define               maxcount           (10000000000)             // max count before stop
#define               DetectorPin        (5)                 // connection to PIN locked to pin 5 in this release
#define               PulseTimeMax       (9000)               // us of max pulse duration * 42
#define               N                  (9000)               // number of channels
#define               PulseTimeMin       (50)                 // channel cut (short pulses due to noise)
#define               MP                 (N / PulseTimeMax)  // chennel modification parameter
//**************************************SD Setup**************************************************
#define               WRITETIME          (3600)              //unixtime 3600 == 1h to write new file on SD
#define               SDPIN              (4)                 //SD PIN
//**************************************Tone Setup**************************************************
#define TONE_PIN                         (2)                // Pin you have speaker/piezo connected to (be sure to include a 100 ohm resistor).
#define GEIGERNOTE                       (110)               //note when particle hit
#define GEIGERNOTEDURATION               (10)                //duration of the note

uint32_t wait = 0;                                           // time waited for screen refresh
uint32_t wait2 = 0;                                          // time waited for other stuff
//**************************************DHT22 Setup**************************************************
float T = 0;                                                 // temperature
float H = 0;                                                 // humidity
uint32_t delayMS;
uint32_t THtime = 0;    
//**************************************Detector Setup**************************************************
volatile int channels[N];                                     // channels array
volatile int PulseTime = 0;                                   // duration of current pulse
bool cend = false;                                            // boolean to check if max count is reached
bool mapmode = false;                                         // true if number of channels is different from max pulse duration
volatile unsigned int count = 0;
volatile boolean P = false;
//**************************************Ethernet Setup***********************************************

uint32_t port = 8080;
IPAddress server(93, 104, 208, 151);
byte mac[] = {  0xDE, 0xAD, 0xBE, 0xEF, 0xFF, 0xEF};
IPAddress ip(151, 100, 44, 130);
IPAddress gateway(151, 100, 44, 1);
IPAddress DNS(8, 8, 8, 8);
IPAddress subnet(255, 255, 255, 0);

//**************************************INIT**************************************************
RTC_DS3231 rtc;
DHT_Unified dht(DHTPIN, DHTTYPE);
LiquidCrystal lcd(31, 33, 35, 37, 39, 41);
/* LCD RS pin to digital pin 12
   LCD Enable pin to digital pin 11
   LCD D4 pin to digital pin 5
   LCD D5 pin to digital pin 4
   LCD D6 pin to digital pin 3
   LCD D7 pin to digital pin 2
   LCD R/W pin to ground
   10K resistor:
   ends to +5V and ground
   wiper to LCD VO pin (pin 3)
*/
// initialize the library instance:
EthernetClient client;
DateTime now;
//SD data
File dataFile;
String filename;
long savetime;

//**************************************Starting functions**************************************************

void LCDSTART(void) {
  lcd.begin(16, 2);
  delay(500);
  lcd.clear();
  lcd.print(("Radon monitor"));
  lcd.setCursor(0, 1);
  lcd.print(("V 0.81"));
  delay(1000);
}
//**************************************Initializzation**********************************************






void SensorConnected(String A) {
  lcd.clear();
  delay(10);
  lcd.print(A);
  lcd.setCursor(0, 1);
  lcd.print("Connected");
  Serial.print(A);
  Serial.println(" Connected");
}

void ClockPL(void) {
  lcd.clear();
  delay(1000);
  lcd.print(("CLOCK POWER"));
  lcd.setCursor(0, 1);
  lcd.print(("LOST"));
  Serial.println(F("RTC lost power time reset"));
}


void SensorNotConnected(String A, int B) {
  lcd.clear();
  delay(10);
  lcd.print(A);
  lcd.setCursor(0, 1);
  lcd.print(("NOT Connected"));
  lcd.print(B);
  Serial.print(A);
  Serial.print(F(" Connection Problem"));
  Serial.print(B);
  delay(1000);
}

void LCDConnected(void) {
  lcd.clear();
  delay(10);
  lcd.print(("Connected"));
  Serial.println(F("Connected"));
  delay(1000);
}

void LCDConnecing(void) {
  lcd.clear();
  delay(10);
  lcd.print(("Connecting.."));
  Serial.println(F("Connecting.."));
  delay(1000);
}

void LCDFailed(void) {
  lcd.clear();
  delay(10);
  lcd.print(F("Failed"));
  Serial.println(F("Failed"));
  delay(1000);
}





void BuzzerLCD(void) {
  Serial.println(F("Buzzer test TONE"));
  lcd.clear();
  lcd.print(F("Buzzer test TONE"));
  delay(1000);
}


void THPrint(bool chk) {

  if (chk == true) {
    Serial.print(F("Humidity: "));
    Serial.print(H);
    Serial.println(F(" RH"));
    Serial.print(F("Temperature: "));
    Serial.print(T);
    Serial.println(F(" *C"));
    //lcd.home();
    lcd.setCursor(0,1);
    lcd.print(("T "));
    lcd.print(int(T));
    lcd.print((" H "));
    lcd.print(int(H));
    lcd.print((" "));
  }
  else {
    Serial.println(F("Error reading DHT!"));
    //lcd.home();
    lcd.setCursor(0, 2);
    lcd.print(F("DHT ERROR    "));
  }
}


void LCDendloop(void) {
  lcd.clear();
  TimePrint();
  lcd.setCursor(0, 1);
  lcd.print("STOP T " + String(int(T)) + " H " + String(int(H)) + " C" + String(count));
  delay(10);
}


void LCDcount(void) {
  Serial.print(PulseTime);
  Serial.println(" us");
  lcd.setCursor(9, 1);
  lcd.println(" C" + String(count) + "         ");
}


void LCDSDstart() {
  lcd.clear();
  lcd.print(F("Initializing SD..."));
  delay(800);
}

void LCDSDinit(bool ck) {
  lcd.clear();
  if (!ck) {
    lcd.print(F("Initializing fail"));
    Serial.println(F("initialization fail!"));
    delay(800);
    return;
  }
  else {
    lcd.print(F("Initializing ok"));
    Serial.println(F("SD pin ok"));
    delay(800);
  }
}

void LCDSDwrite(void) {
  Serial.println(F("file writing..."));
  lcd.clear();
  lcd.print(F("file writing..."));

}


void LCDSDfile(bool ck) {

  if (!ck) {
    Serial.println(F("file writing failed!"));
    lcd.clear();
    lcd.print(F("file fail"));

  }
  else {
    Serial.println(F("file writing done."));
    lcd.clear();
    lcd.print(F("file ok"));

  }

}


void RTCstart(void) {
  // clock start routine
  if (! rtc.begin()) {
    Serial.println(F("Couldn't find RTC"));
    while (1);
  }
  Serial.println(F("started RTC"));
  delay(800);
}

void RTClostPower(void) {
  //refresh clock to last state if power lost event
  if (rtc.lostPower()) {
    ClockPL();
    // following line sets the RTC to the date & time this sketch was compiled
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
  }
}

void timenow(void) {
  //update time funcion
  TimePrint();
}

void TimePrint(void) {
  now = rtc.now();
  Serial.print(now.year(), DEC);
  Serial.print('/');
  Serial.print(now.month(), DEC);
  Serial.print('/');
  Serial.print(now.day(), DEC);
  Serial.print('-');
  Serial.print(now.hour(), DEC);
  Serial.print(':');
  Serial.print(now.minute(), DEC);
  Serial.print(':');
  Serial.print(now.second(), DEC);
  Serial.println();
  lcd.home();
  lcd.print(now.hour(), DEC);
  lcd.print((':'));
  lcd.print(now.minute(), DEC);
  lcd.print((':'));
  lcd.print(now.second(), DEC);
  lcd.print((' '));

  lcd.print(now.day(), DEC);
  lcd.print(('/'));
  lcd.print(now.month(), DEC);
  lcd.print(('/'));
  lcd.print(int(now.year()) - 2000);
  lcd.println((' '));
}
/*
  interrupt callback. TC6=timer counter TC2 channel 0
*/
void TC6_Handler(void)
{
  static uint32_t ra = 0;
  static uint32_t rb = 0;
  //reads the interrupt. necessary to clear the interrupt flag.
  const uint32_t status = TC_GetStatus(TC2, 0);
  //input capture
  const bool inputcaptureA = status & TC_SR_LDRAS;
  const bool inputcaptureB = status & TC_SR_LDRBS;
  //loading overrun
  const bool loadoverrun = status & TC_SR_LOVRS;
  TC_ReadCV(TC2, 0);
  //read LDRA. If we dont, we will get overflow (TC_SR_LOVRS)
  if (inputcaptureA && !loadoverrun) {
    ra = TC2->TC_CHANNEL[0].TC_RA;
  }
  //read LDRB. If we dont, we will get overflow (TC_SR_LOVRS)
  if (inputcaptureB && !loadoverrun) {
    rb = TC2->TC_CHANNEL[0].TC_RB;
    PulseTime = (ra - rb);
    P = true;
    count++;
  }



}

void startTimer(Tc *tc, uint32_t channel, IRQn_Type irq) {
  //see 37.7.9
  REG_TC2_WPMR = 0x54494D00;
  //enable configuring the io registers. see 32.7.42
  REG_PIOC_WPMR = 0x50494F00;
  //we need to configure the pin to be controlled by the right peripheral.
  //pin 5 is port C. PIOC_PDR is defined in hardware/arduino/sam/system/CMSIS/Device/ATMEL/sam3xa/include/instance/instance_pioc.h
  //and PIO_PDR_P25 is defined in hardware/arduino/sam/system/CMSIS/Device/ATMEL/sam3xa/include/component/component_pio.h
  //this disables the pio from controlling the pin. see 32.7.2
  REG_PIOC_PDR |= PIO_PDR_P25;
  //next thing is to assign the io line to the peripheral. See 32.7.24.
  //we need to know which peripheral we should use. Read table 37-4 in section 37.5.1.
  //TIOA6 is peripheral B, so we want to set that bit to 1.
  //REG_PIOC_ABSR is defined in hardware/arduino/sam/system/CMSIS/Device/ATMEL/sam3xa/include/instance/instance_pioc.h
  //PIO_ABSR_P25 is defined in hardware/arduino/sam/system/CMSIS/Device/ATMEL/sam3xa/include/component/component_pio.h
  REG_PIOC_ABSR |= PIO_ABSR_P25;

  //allow configuring the clock.
  pmc_set_writeprotect(false);
  /*
    Every peripheral in the SAM3X is off by default (to save power) and
    should be turned on.
  */
  pmc_enable_periph_clk(ID_TC6);
  /*
    configure the timer. All this is about setting TC_CMRx, see 37.7.10 in atmel pdf.
    We use CLOCK1 at 42 MHz to get the best possible resolution.
  */
  TC_Configure(tc, channel,  TC_CMR_TCCLKS_TIMER_CLOCK1 | TC_CMR_LDRA_FALLING | TC_CMR_LDRB_RISING);
  //set the interrupt flags. We want interrupt on overflow and TIOA6 (pin 5) going high.
  const uint32_t flags = TC_IER_COVFS  | TC_IER_LDRAS;
  tc->TC_CHANNEL[channel].TC_IER = flags;
  tc->TC_CHANNEL[channel].TC_IDR = ~flags; //assume IER and IDR are equally defined.
  NVIC_EnableIRQ(irq);
  //start the timer
  TC_Start(tc, channel);
}


void TimerSetup(void) {

  startTimer(TC2, 0, TC6_IRQn); //TC2 channel 0, the IRQ for that channel
  delay(50);
  TC_ReadCV(TC2, 0); //Test reading
}


void DetectorStart(void) {
  //setting up pin
  /*pinMode(DetectorPin, INPUT);
  //sensor presence check
  int i = 0;
  while (digitalRead(DetectorPin) == HIGH) {
    SensorNotConnected(F("Detector"), ((int)waitingtime - i) );
    delay(1000);
    i++;
    if (digitalRead(DetectorPin) == LOW) {
      SensorConnected(F("Detector"));
      delay(500);
      break;
    }
    if (i == waitingtime) {
      break;
    }
  }
  */
  //control if channel number is equal to pulse duration for mapping function purposes
  if (PulseTimeMax != N) {
    mapmode = true;
  }
  else {
    mapmode = false;
  }
  TimerSetup();
}

void THSETUP(void) {
  Serial.println(F("DHT start"));
  dht.begin();
  delay(2000);
  // Print temperature sensor details.
  sensor_t sensor;
  dht.temperature().getSensor(&sensor);
  Serial.println(F("------------------------------------"));
  Serial.println(F("Temperature"));
  Serial.print  (F("Sensor:       ")); Serial.println(sensor.name);
  Serial.print  (F("Driver Ver:   ")); Serial.println(sensor.version);
  Serial.print  (F("Unique ID:    ")); Serial.println(sensor.sensor_id);
  Serial.print  (F("Max Value:    ")); Serial.print(sensor.max_value); Serial.println(F(" *C"));
  Serial.print  (F("Min Value:    ")); Serial.print(sensor.min_value); Serial.println(F(" *C"));
  Serial.print  (F("Resolution:   ")); Serial.print(sensor.resolution); Serial.println(F(" *C"));
  Serial.println(F("------------------------------------"));
  // Print humidity sensor details.
  dht.humidity().getSensor(&sensor);
  Serial.println(F("Humidity"));
  Serial.print  (F("Sensor:       ")); Serial.println(sensor.name);
  Serial.print  (F("Driver Ver:   ")); Serial.println(sensor.version);
  Serial.print  (F("Unique ID:    ")); Serial.println(sensor.sensor_id);
  Serial.print  (F("Max Value:    ")); Serial.print(sensor.max_value); Serial.println(F("%"));
  Serial.print  (F("Min Value:    ")); Serial.print(sensor.min_value); Serial.println(F("%"));
  Serial.print  (F("Resolution:   ")); Serial.print(sensor.resolution); Serial.println(F("%"));
  Serial.println(F("------------------------------------"));
  // Set delay between sensor readings based on sensor details.
  delayMS = sensor.min_delay / 1000;
}

void SDsetup() {
  LCDSDstart();
  if (!SD.begin(SDPIN)) {
    LCDSDinit(false);
    return;
  }
  else {
    LCDSDinit(true);
  }
  // open the file. note that only one file can be open at a time,
  // so you have to close this one before opening another.
  DateTime now = rtc.now();
  savetime = now.unixtime();
  filename = String("D" + String(now.day(), DEC) + "h" + String(now.hour(), DEC) + String(now.minute(), DEC) + ".txt");

  Serial.print(F("filename: ") );
  char fileNameCharArray[filename.length() + 1];
  filename.toCharArray(fileNameCharArray, filename.length() + 1);
  Serial.println(fileNameCharArray);

  File dataFile = SD.open(fileNameCharArray, FILE_WRITE);
  delay(10);
  Serial.println(F("Creating file..."));
  LCDSDwrite();

  if (dataFile) {
    Serial.print(F("writing file.."));
    dataFile.println(("Radon sensor v0.7"));
    dataFile.println(("Date    Time          T     H   Channels  "));
    dataFile.close();
    LCDSDfile(true);
  }
  else {
    // if the file didn't open, print an error:
    LCDSDfile(false);
  }
}

void BUZZERsetup(void) {
  int melody[] = { 262, 196, 196, 220, 196, 0, 247, 262 };
  int duration[] = { 250, 125, 125, 250, 250, 250, 250, 250 };
  BuzzerLCD();
  for (int thisNote = 0; thisNote < 8; thisNote++)
  { // Loop through the notes in the array.
    TimerFreeTone(TONE_PIN, melody[thisNote], duration[thisNote]); // Play thisNote for duration.
    delay(50); // Short delay between notes.
  }
}


void EthernetStart(void) {
  Ethernet.begin(mac, ip, DNS, gateway, subnet);
  // give the Ethernet shield a second to initialize:
  delay(1000);
  Serial.print(F("My IP address: "));
  Serial.println(Ethernet.localIP());
  int i = 1;
  LCDConnecing();
  delay(800);
  while (i <= 5) {
    i++;
    if (client.available()) {
      LCDConnected();
      i = 6;
    }
    else {
      // kf you didn't get a connection to the server:
      LCDFailed();
    }
    delay(800);
  }
  client.stop();
}


void setup(void) {
  LCDSTART();
  //Start Serial comunication
  Serial.begin(115200);
  //start RTC
  RTCstart();
  //reset time if power lost
  RTClostPower();
  //Start the SPI and ethernet library:
  SPI.begin();
  EthernetStart();
  SDsetup();
  // DHT sensor reading
  THSETUP();
  //buzzer starup tone
  BUZZERsetup();
  //Start detector
  DetectorStart();
  //time print at end
  timenow();
}






void THRead(void) {
  
  // Delay between measurements.
  if ((millis()-THtime) >delayMS ) {
    // Get temperature event and print its value.
    sensors_event_t event;
    dht.temperature().getEvent(&event);
    T = event.temperature;
    dht.humidity().getEvent(&event);
    H = event.relative_humidity;
    if (isnan(T) || isnan(H)) {
      T = 0;
      H = 0;
      THPrint(false);
    }
    else {
      THPrint(true);
    }
    THtime = millis();
  }
  
}

void Tone(void) {
  TimerFreeTone(TONE_PIN, GEIGERNOTE, GEIGERNOTEDURATION); // Play thisNote for duration.
}

void DetectorUpdate(void) {
  if (P != 0) {
    if (PulseTime >= PulseTimeMin && PulseTime <= PulseTimeMax) {
      P = false;
      LCDcount();

      if (mapmode = true ) {
        PulseTime = MP * PulseTime;
      }
      channels[PulseTime]++;
    }
    Tone();
    if ( count >= maxcount) {
      cend = true;
      LCDendloop();
    }
  }
}

void ethernet(void) {
  // Use WiFiClient class to create TCP connections
  if (client.available()) {
    client.stop();
    int a = client.connect(server, port);
    if ( a == 1) {
      int i;
      Serial.println(F("-> Connecting"));
      // Make a HTTP request:
      client.print(("GET /vss/hopes/?"));
      client.print(("n="));
      client.print( nID );
      client.print(("&&"));
      client.print(("T="));
      client.print( T );
      client.print(("&&"));
      client.print(("H="));
      client.print( H );
      client.print(("&&"));
      client.print(("C="));
      for (i = 0; i < N; i++) {
        client.print(channels[i]); client.print(("&&"));
      }
      client.println( (" HTTP/1.1"));
      client.print( ("Host: " ));
      for (i = 0; i < 4; i++) {
        client.print(server[i]); client.print(F("."));
      }
      client.println(("Connection: close" ));
      client.println();
      client.println();
      client.stop();
    }
    else{
      lcd.clear();
      lcd.print(F("Server error         " ));
      Serial.println(F("Server error:"));
      if (a == -1){
       Serial.println( "TIMED_OUT ");
      }
      if (a == -2){
        Serial.println( "INVALID_SERVER ");
      }
      if (a == -3){
        Serial.println( "TRUNCATED ");
      }
      else{
        Serial.println( "INVALID_RESPONSE ");
      }
    }
  }
  else {
    lcd.clear();
    lcd.print(F("CONN error         " ));
    Serial.println(F("Connection error"));
    delay(200);
    // you didn't get a connection to the server:
  }
}





void SDwrite(void) {
  LCDSDwrite();

  DateTime now = rtc.now();
  if (now.unixtime() - savetime >= WRITETIME) {
    savetime = now.unixtime();
    Serial.print(F("New filename: ") );
    filename = String(String(now.day(), DEC) + "h" + String(now.hour(), DEC) + "m" + String(now.minute(), DEC) + ".txt");
  }
  else {
    Serial.print(F("Old filename: ") );
  }
  char fileNameCharArray[filename.length() + 1];
  filename.toCharArray(fileNameCharArray, filename.length() + 1);
  File dataFile = SD.open(fileNameCharArray, FILE_WRITE);
  Serial.println(fileNameCharArray);
  if (dataFile) {
    int i;
    Serial.print(F("writing.."));
    //Time
    dataFile.print(now.year(), DEC);
    dataFile.print('/');
    dataFile.print(now.month(), DEC);
    dataFile.print('/');
    dataFile.print(now.day(), DEC);
    dataFile.print((' '));
    dataFile.print(now.hour(), DEC);
    dataFile.print(':');
    dataFile.print(now.minute(), DEC);
    dataFile.print(':');
    dataFile.print(now.second(), DEC);
    dataFile.print((' '));
    //HIH
    dataFile.print(T, 2);
    dataFile.print((' '));
    dataFile.print(H, 2);
    dataFile.print((' '));
    //channels
    for (i = 0; i < N; i++) {
      dataFile.print(String(channels[i]));
      dataFile.print((' '));
    }
    dataFile.println();
    LCDSDfile(true);
    
  }
  else {
    // if the file didn't open, print an error:
    LCDSDfile(false);
  }
  dataFile.close();

}








void loop(void) {

  DetectorUpdate();
  if (millis() - wait >= refresh) {
    Serial.println(F(" refresh"));
    timenow();
    THRead();
    wait = millis();
  }
  if (millis() - wait2 >= refresh2) {
    Serial.println(F("UPDATE DATA"));
    TimerFreeTone(TONE_PIN, 100, 1);
    SDwrite();
    ethernet();
    wait2 = millis();
  }
}








