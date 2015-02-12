/************************************
 ODO/TRIP COUNTER for Z20 SOARER
 Version 0.99
 2015/2/6-
 Last Modify: 2015/2/12
*************************************/

/*
  pin-assign

    D2 : SPEED-Pulse in
    D4 : Software Serial RX
    D5 : Software Serial TX
    D10: SPI Chip-Select
    D11: SPI MOSI
    D12: SPI MISO
    D13: SPI SCK
    A4 : I2C SDA
    A5 : I2C SCL

*/

#define ACM 0       // 0:ST7032系 1:ACM1602系
#define CONTRAST 30 // ST7032-Contrast

#include <Wire.h>            // I2C Library
#include <SPI.h>             // SPI Library
#if ACM
#include <LcdCore.h>         // LCD Core Library
#include <LCD_ACM1602NI.h>   // ACM1602 Library
#else
#include <ST7032.h>          // ST7032 Library
#endif

// for RTC
#include <DS1307RTC.h>
#include <Time.h>

// for Serial
#include <SoftwareSerial.h>
SoftwareSerial mySerial(6,7);

// for FRAM COMMAND
const byte CMD_WREN = 0x06; //0000 0110 Set Write Enable Latch
const byte CMD_WRDI = 0x04; //0000 0100 Write Disable
const byte CMD_RDSR = 0x05; //0000 0101 Read Status Register
const byte CMD_WRSR = 0x01; //0000 0001 Write Status Register
const byte CMD_READ = 0x03; //0000 0011 Read Memory Data
const byte CMD_WRITE = 0x02; //0000 0010 Write Memory Data
const int FRAM_CS = 10; //chip select is D10 pin

#if ACM
LCD_ACM1602NI lcd(0xa0); // 0xa0はI2Cアドレス
#else
ST7032 lcd;
#endif

#define spdPin 2         // SPEED-Pulse Input-pin (D2/Int0)
#define COUNTUP 12740    // 20pulse

// Counter etc.
volatile long counter = 0;
volatile long odokm = 0;
volatile long trip0_counter = 0,tripkm=0;
volatile long trip0_counter_all = 0;
volatile long trip1_counter = 0,trip1_km=0;
volatile long trip2_counter = 0,trip2_km=0;
volatile long trip3_counter = 0,trip3_km=0;
char serial_str[128];
char start_time[30];

// FRAM Address
#define F_COUNTER 0
#define F_ODO     5
#define F_TRIP1   10
#define F_TRIP2   15
#define F_TRIP3   20
#define F_TRIP1_COUNTER 25
#define F_TRIP2_COUNTER 30
#define F_TRIP3_COUNTER 35
#define F_RAM_DAY(y,m,d) (y-2015)*1116+(m-1)*93+(d-1)+100

// fram read
long fram_read(byte addr){
  long rc;
  digitalWrite(FRAM_CS, LOW);
  SPI.transfer(CMD_READ);
  SPI.transfer(0);
  SPI.transfer(addr);
  rc=SPI.transfer(0x00);
  digitalWrite(FRAM_CS, HIGH);
  return(rc);
}

// fram read-trip
void fram_read_trip(int trip_no){
  long i1,i2,i3,trip;
  digitalWrite(FRAM_CS, LOW);
  SPI.transfer(CMD_READ);
  SPI.transfer(0);
  switch(trip_no){
    case 1: SPI.transfer(F_TRIP1);break;
    case 2: SPI.transfer(F_TRIP2);break;
    case 3: SPI.transfer(F_TRIP3);break;
    defalt: SPI.transfer(F_TRIP1);break;
  }
  i1=SPI.transfer(0x00);
  i2=SPI.transfer(0x00);
  i3=SPI.transfer(0x00);
  digitalWrite(FRAM_CS, HIGH);

  trip=(i3*256*256)+(i2*256)+i1;
  switch(trip_no){
    case 1: trip1_km=trip; break;
    case 2: trip2_km=trip; break;
    case 3: trip3_km=trip; break;
  }    
}

// fram write for pulse-count
void fram_write_counter(){
  byte i1,i2;
  i1=(byte)((long)counter%256);
  i2=(byte)((long)counter/256);

  // Write Main-Counter
  digitalWrite(FRAM_CS, LOW);
  SPI.transfer(CMD_WREN);
  digitalWrite(FRAM_CS, HIGH); 

  digitalWrite(FRAM_CS, LOW);
  SPI.transfer(CMD_WRITE);
  SPI.transfer(0);
  SPI.transfer(F_COUNTER);
  SPI.transfer(i1);
  SPI.transfer(i2);
  digitalWrite(FRAM_CS, HIGH);

  digitalWrite(FRAM_CS, LOW);
  SPI.transfer(CMD_WRDI);
  digitalWrite(FRAM_CS, HIGH); 
  
  // write Trip1_Counter
  digitalWrite(FRAM_CS, LOW);
  SPI.transfer(CMD_WREN);
  digitalWrite(FRAM_CS, HIGH); 

  digitalWrite(FRAM_CS, LOW);
  SPI.transfer(CMD_WRITE);
  SPI.transfer(0);
  SPI.transfer(F_TRIP1_COUNTER);
  i1=(byte)((long)trip1_counter%256);
  i2=(byte)((long)trip1_counter/256);
  SPI.transfer(i1);
  SPI.transfer(i2);
  digitalWrite(FRAM_CS, HIGH);

  digitalWrite(FRAM_CS, LOW);
  SPI.transfer(CMD_WRDI);
  digitalWrite(FRAM_CS, HIGH); 
  
  // write Trip2_Counter
  digitalWrite(FRAM_CS, LOW);
  SPI.transfer(CMD_WREN);
  digitalWrite(FRAM_CS, HIGH); 

  digitalWrite(FRAM_CS, LOW);
  SPI.transfer(CMD_WRITE);
  SPI.transfer(0);
  SPI.transfer(F_TRIP2_COUNTER);
  i1=(byte)((long)trip2_counter%256);
  i2=(byte)((long)trip2_counter/256);
  SPI.transfer(i1);
  SPI.transfer(i2);
  digitalWrite(FRAM_CS, HIGH);

  digitalWrite(FRAM_CS, LOW);
  SPI.transfer(CMD_WRDI);
  digitalWrite(FRAM_CS, HIGH); 
  
  // write Trip3_Counter
  digitalWrite(FRAM_CS, LOW);
  SPI.transfer(CMD_WREN);
  digitalWrite(FRAM_CS, HIGH); 

  digitalWrite(FRAM_CS, LOW);
  SPI.transfer(CMD_WRITE);
  SPI.transfer(0);
  SPI.transfer(F_TRIP3_COUNTER);
  i1=(byte)((long)trip3_counter%256);
  i2=(byte)((long)trip3_counter/256);
  SPI.transfer(i1);
  SPI.transfer(i2);
  digitalWrite(FRAM_CS, HIGH);
 
  digitalWrite(FRAM_CS, LOW);
  SPI.transfer(CMD_WRDI);
  digitalWrite(FRAM_CS, HIGH); 
}

// fram write for ODO
void fram_write_odo(){
  long odo;
  byte i1,i2,i3;

  odo=odokm;
  i1=(byte)((long)odo%256);
  odo=odo/256;
  i2=(byte)((long)odo%256);
  odo=odo/256;
  i3=(byte)((long)odo%256);

  digitalWrite(FRAM_CS, LOW);
  SPI.transfer(CMD_WREN);
  digitalWrite(FRAM_CS, HIGH); 

  digitalWrite(FRAM_CS, LOW);
  SPI.transfer(CMD_WRITE);
  SPI.transfer(0);
  SPI.transfer(F_ODO);
  SPI.transfer(i1);
  SPI.transfer(i2);
  SPI.transfer(i3);
  digitalWrite(FRAM_CS, HIGH);
 
  digitalWrite(FRAM_CS, LOW);
  SPI.transfer(CMD_WRDI);
  digitalWrite(FRAM_CS, HIGH); 

  tmElements_t tm;

  if (RTC.read(tm)) {
    if( (tm.Year+1970)<=2042 ){
      digitalWrite(FRAM_CS, LOW);
      SPI.transfer(CMD_WREN);
      digitalWrite(FRAM_CS, HIGH); 

      digitalWrite(FRAM_CS, LOW);
      SPI.transfer(CMD_WRITE);
      SPI.transfer(0);
      SPI.transfer(F_RAM_DAY(tm.Year+1970,tm.Month,tm.Day));
      SPI.transfer(i1);
      SPI.transfer(i2);
      SPI.transfer(i3);
      digitalWrite(FRAM_CS, HIGH);
 
      digitalWrite(FRAM_CS, LOW);
      SPI.transfer(CMD_WRDI);
      digitalWrite(FRAM_CS, HIGH); 
    }
  }
}

// fram write for Trip
void fram_write_trip(int trip_no){
  long trip;
  byte i1,i2,i3;

  trip=0;
  switch(trip_no){
    case 1: trip=trip1_km; break;
    case 2: trip=trip2_km; break;
    case 3: trip=trip3_km; break;
    default: return;
  }
  
  i1=(byte)((long)trip%256);
  trip=trip/256;
  i2=(byte)((long)trip%256);
  trip=trip/256;
  i3=(byte)((long)trip%256);
  digitalWrite(FRAM_CS, LOW);
  SPI.transfer(CMD_WREN);
  digitalWrite(FRAM_CS, HIGH); 

  digitalWrite(FRAM_CS, LOW);
  SPI.transfer(CMD_WRITE);
  SPI.transfer(0);
  switch(trip_no){
    case 1: SPI.transfer(F_TRIP1); break;
    case 2: SPI.transfer(F_TRIP2); break;
    case 3: SPI.transfer(F_TRIP3); break;
  }
  SPI.transfer(i1);
  SPI.transfer(i2);
  SPI.transfer(i3);
  digitalWrite(FRAM_CS, HIGH);
 
  digitalWrite(FRAM_CS, LOW);
  SPI.transfer(CMD_WRDI);
  digitalWrite(FRAM_CS, HIGH); 
}

// pulse-count from int0
void pulsechange(){
  counter++;
  trip0_counter++;
  trip0_counter_all++;
  trip1_counter++;
  trip2_counter++;
  trip3_counter++;
  if(counter==COUNTUP){
    odokm++;
    counter=0;
  }
  if(trip0_counter==COUNTUP){
    tripkm++;
    trip0_counter=0;
  }
  if(trip1_counter==COUNTUP){
    trip1_km++;
    trip1_counter=0;
  }
    if(trip2_counter==COUNTUP){
    trip2_km++;
    trip2_counter=0;
  }
    if(trip3_counter==COUNTUP){
    trip3_km++;
    trip3_counter=0;
  }
}

// setup
void setup() {
  pinMode(spdPin, INPUT);
  attachInterrupt(0, pulsechange, RISING);
//attachInterrupt(0, pulsechange, CHANGE);

  // lcd-init.
  #if ACM
  lcd.begin(16, 2);
  #else
  delay(5);
  lcd.begin(8, 2);
  lcd.setContrast(CONTRAST);
  #endif
  // for F-RAM
  pinMode(FRAM_CS, OUTPUT);
  digitalWrite(FRAM_CS, HIGH);
   //Setting up the SPI bus
  SPI.begin();
  SPI.setDataMode(SPI_MODE0);
  SPI.setBitOrder(MSBFIRST);
  SPI.setClockDivider(SPI_CLOCK_DIV2);
  
// restore from fram
  long i1,i2,i3;

  i1=fram_read(F_COUNTER);
  i2=fram_read(F_COUNTER+1);
  counter=i2*256+i1;

  i1=fram_read(F_TRIP1_COUNTER);
  i2=fram_read(F_TRIP1_COUNTER+1);
  trip1_counter=i2*256+i1;

  i1=fram_read(F_TRIP2_COUNTER);
  i2=fram_read(F_TRIP2_COUNTER+1);
  trip2_counter=i2*256+i1;

  i1=fram_read(F_TRIP3_COUNTER);
  i2=fram_read(F_TRIP3_COUNTER+1);
  trip3_counter=i2*256+i1;

  i1=fram_read(F_ODO);
  i2=fram_read(F_ODO+1);
  i3=fram_read(F_ODO+2);
  odokm=(i3*256*256)+(i2*256)+i1;

  fram_read_trip(1);
  fram_read_trip(2);
  fram_read_trip(3);
  
// lcd startup demo 
  lcd.setCursor(0, 0);
  lcd.print(" SOARER ");
  lcd.setCursor(0,1);
  lcd.print(counter);

  tmElements_t tm;

  start_time[0]=0;
  if (RTC.read(tm)) {
    char str[10];
    lcd.setCursor(0, 1);
    sprintf(str,"%2.2d:%2.2d",tm.Hour,tm.Minute);
    lcd.print(str);
    sprintf(start_time,"%4.4d/%2.2d/%2.2d %2.2d:%2.2d:%2.2d",
      tm.Year+1970,tm.Month,tm.Day,
      tm.Hour,tm.Minute,tm.Second);
  }
  delay(1000);
  lcd.setCursor(0, 0);
  lcd.print("        ");
  lcd.setCursor(0, 1);
  lcd.print("        ");
  mySerial.begin(9600);
  Serial.begin(9600);
  serial_str[0]=0;
}


//**************//
// main loop    //
//**************//

void loop() {
  char str[40];
  static int serial_pos=0;
  static long before_counter=-1;
  static long before_odokm=-1;
  static long before_trip1_km=-1,before_trip2_km=-1,before_trip3_km=-1;
  static uint8_t before_sec=60;
  static int before_m=-1;
  tmElements_t tm;
  int ch;
  
  if(before_counter!=counter ){
    fram_write_counter();
    sprintf(str,"%5.5ld",counter);
    lcd.setCursor(0, 0);
    lcd.print("        ");
    lcd.setCursor(0, 0);
    lcd.print(str);
    before_counter=counter;
  }
  
  if(before_odokm!=odokm || before_m!=(counter/(COUNTUP/10))){
    if(before_odokm!=odokm){
      fram_write_odo();
    }
    before_m=(counter/(COUNTUP/10));
    sprintf(str,"%6ld.%d",odokm,before_m);
    lcd.setCursor(0, 1);
    lcd.print(str);
    before_odokm=odokm;
  }

  // Trip print & write
  if(before_trip1_km!=trip1_km){
    fram_write_trip(1);
  }
  if(before_trip2_km!=trip2_km){
    fram_write_trip(2);
  }
  if(before_trip3_km!=trip3_km){
    fram_write_trip(3);
  }

  if((millis()%1000)>0 && (millis()%1000)<4 ){
    if (RTC.read(tm)) {
      if(tm.Second!=before_sec){
        lcd.setCursor(0, 0);
        sprintf(str,"%2.2d:%2.2d:%2.2d",tm.Hour,tm.Minute,tm.Second);
        lcd.print(str);
        before_sec=tm.Second;
      }
    }
  }

// for Host Communication
    if(mySerial.available()>0){
      ch=mySerial.read();
      if(ch==10 || ch==13){
        if(strcmp(serial_str,"now")==0){
          if (RTC.read(tm)) {
            sprintf(str,"%2.2d:%2.2d:%2.2d",tm.Hour,tm.Minute,tm.Second);
            mySerial.print(str);
          } else {
            mySerial.print("??:??:??");
          } 
          int trip_m;
          mySerial.print(" ");
          mySerial.print(odokm);
          mySerial.print(".");
          mySerial.print(before_m);
          mySerial.print(" ");

          trip_m=(trip0_counter/(COUNTUP/10));
          mySerial.print(start_time);
          mySerial.print(" ");
          mySerial.print(tripkm);
          mySerial.print(".");
          mySerial.print(trip_m);
          mySerial.print(" ");
          mySerial.print(trip0_counter_all);
          mySerial.print(" ");

          trip_m=(trip1_counter/(COUNTUP/10));
          mySerial.print(trip1_km);
          mySerial.print(".");
          mySerial.print(trip_m);
          mySerial.print(" ");

          trip_m=(trip2_counter/(COUNTUP/10));
          mySerial.print(trip2_km);
          mySerial.print(".");
          mySerial.print(trip_m);
          mySerial.print(" ");

          trip_m=(trip3_counter/(COUNTUP/10));
          mySerial.print(trip3_km);
          mySerial.print(".");
          mySerial.print(trip_m);
          mySerial.print(" ");
          mySerial.print(F_RAM_DAY(tm.Year+1970,tm.Month,tm.Day));
          mySerial.print("\n");

        } else if(strstr(serial_str,"setodo:")){
          sprintf(str,"%s",strstr(serial_str,"setodo:")+7);
          odokm=atol(str);
          counter=0;
          lcd.setCursor(0,1);
          lcd.print("        ");
          lcd.setCursor(0,1);
          lcd.print(str);
          delay(500);
        } else if(strstr(serial_str,"trip1reset")){
          trip1_km=0;
          trip1_counter=0;
        } else if(strstr(serial_str,"trip2reset")){
          trip2_km=0;
          trip2_counter=0;
        } else if(strstr(serial_str,"trip3reset")){
          trip3_km=0;
          trip3_counter=0;
        }
        serial_pos=0;
        serial_str[0]=0;
      } else if(ch>9 && ch<128){
        if(serial_pos<40){
          serial_str[serial_pos]=ch;
          serial_pos++;
          serial_str[serial_pos]=0;
        }
      }
  }
  delay(1);
}

