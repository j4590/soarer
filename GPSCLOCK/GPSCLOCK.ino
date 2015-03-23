/*
    GPS Clock For Z20 Version 2.0
    2015/3/21

 pin assign
   D04: Software-Serial RX (from GPS)
   D05: Software-Serial TX (n/a)
   D07: CS(SD)
   D08: RESET(TFT)
   D09: RS/DC(TFT)
   D10: CS(TFT)
   D11: MOSI(TFT/SD)
   D12: MISO(SD)
   D13: SCLK(TFT/SD)

   A04: I2C-SDA(RTC)
   A05: I2C-SCL(RTC)

   Vcc(5V): to TFT,SD,RTC from 7805(etc.)
   Gnd: to TFT/SD,RTC from GND
*/

#define DEBUG 0

#include <TFT.h>                   // LCD library
#include <SPI.h>                   // SPI Library
#include <SD.h>                    // SD Library
#include <Wire.h>                  // I2C Library
#include <DS1307RTC.h>             // RTC DS1307 Library
#include <Time.h>
#include <SoftwareSerial.h>        // Software Serial Library

#define TFT_CS 10
#define DC   9
#define RST  8  
#define SD_CS  7
#define RX 4
#define TX 5
#define GpsBaudrate 4800

#define off_x1 14
#define off_y1 8
#define off_x2 14
#define off_y2 42

TFT TFTscreen = TFT(TFT_CS, DC, RST);
SoftwareSerial mySerial(RX,TX);

tmElements_t tm;
char nowstr[20];
char before[20];
#define gps_max 80
char gps_buffer[gps_max+1];
byte gps_pos=0;
byte chkd=0;
File myFile;

void setup() {
  byte i,rc;

  pinMode(TFT_CS,OUTPUT);
  pinMode(SD_CS,OUTPUT);

  TFTscreen.begin();
  TFTscreen.background(0, 0, 0);
  TFTscreen.stroke(255,255,255);
  TFTscreen.setTextSize(1);

  // logo read and draw from SD
  if(SD.begin(SD_CS)){
    myFile = SD.open("logo.dat", FILE_READ);

    if(myFile){
      int x=0,y=0;
      int ch;
      while(myFile.available()){
        ch=myFile.read();
        if(ch==10){
          x=0;y++;
        } else {
          switch(ch){
            case ' ': TFTscreen.stroke(0,0,0);break;
            case '#': TFTscreen.stroke(255,255,255);break;
            case '.': TFTscreen.stroke(0xaa,0xaa,0xaa);break;
            case 'X': TFTscreen.stroke(0xbb,0xbb,0xbb);break;
            case 'o': TFTscreen.stroke(0xcc,0xcc,0xcc);break;
            case 'O': TFTscreen.stroke(0xdd,0xdd,0xdd);break;
            case '+': TFTscreen.stroke(0xee,0xee,0xee);break;
            case '@': TFTscreen.stroke(0xff,0xff,0xff);break;
            default: TFTscreen.stroke(0,0,0);break;
          }
           TFTscreen.point(off_x1+x,off_y1+y);
           x++;
        }
      }
      myFile.close();
    }
  } else {
    TFTscreen.text("SD Init. Fail!!",0,0);
  }

  if (!RTC.read(tm)){
    tm.Year=2015-1970;
    tm.Month=3;
    tm.Day=10;
    tm.Hour=23;
    tm.Minute=54;
    tm.Second=10;
  }
  sprintf(nowstr,"%-2.2d:%-2.2d:%-2.2d",tm.Hour,tm.Minute,tm.Second);
  strcpy(before,nowstr);

  mySerial.begin(GpsBaudrate);
  gps_buffer[0]=0;
  gps_pos=0;
  chkd=0;
  
  #if DEBUG
  Serial.begin(9600);
  #endif 

  delay(500);
}

void loop() {
  char m;
  char str[2];
  char ch;
  
  // for gps-read
  while (mySerial.available() && chkd==0){
    ch=mySerial.read();
    if(ch==10 || ch==13){
      gps_buffer[gps_pos]=0;
      gps_pos=0;
      if(strncmp(gps_buffer,"$GPRMC,",7)==0){
        if(strstr(gps_buffer,",A*") || strstr(gps_buffer,",E*")){
          if(strstr(gps_buffer,",A,") ){
            chk_date();
          }
        }
      }
    } else if (gps_pos<gps_max) {
      gps_buffer[gps_pos]=ch;
      gps_pos++;
      gps_buffer[gps_pos]=0;
    }
  }

  // 現在時刻チェック 
  if (RTC.read(tm)){
    // OK
  } 

  sprintf(nowstr,"%-2.2d:%-2.2d:%-2.2d",tm.Hour,tm.Minute,tm.Second);
  if(chkd==0){
    m=' ';
  }else if(tm.Second%2==0){
    m=' ';
  } else {
    m='.';
  }
  sprintf(nowstr,"%2d/%2d %-2.2d:%-2.2d%c",tm.Month,tm.Day,tm.Hour,tm.Minute,m);

  // 時刻表示更新
  if(strcmp(nowstr,before)!=0){
    TFTscreen.setTextSize(2);
    TFTscreen.stroke(255,255,255);

    for(int i=0;i<strlen(nowstr);i++){
      if(nowstr[i]!=before[i]){
        TFTscreen.stroke(0,0,0);
        str[0]=before[i];str[1]=0;
        TFTscreen.text(str,off_x2+i*(6*2),off_y2);
      }
      TFTscreen.stroke(0,255,0);
      str[0]=nowstr[i];str[1]=0;
      TFTscreen.text(str,off_x2+i*(6*2),off_y2);
    }
    strcpy(before,nowstr);
  }
  if(chkd==1)delay(10);
}

void chk_date(){
  int i,st,cnt=0;  
  char buf[64],buf2[3];
  int YY,MM,DD,hh,mm,ss;
  int days[]={0,31,28,31,30,31,30,31,31,30,31,30,31};

  for(i=7;i<strlen(gps_buffer);i++){
    if(gps_buffer[i]==','){
       st=i+1;
       break;
    } else if(gps_buffer[i]=='.'){
       buf[i-7]=0;
    } else {
       buf[i-7]=gps_buffer[i];
       buf[i-6]=0;
    }
  }
  
  strncpy(buf2,buf,2);
  hh=atoi(buf2);
  strncpy(buf2,buf+2,2);
  mm=atoi(buf2);
  strncpy(buf2,buf+4,2);
  ss=atoi(buf2);

  for(i=st;i<strlen(gps_buffer);i++){
    if(gps_buffer[i]==','){
      cnt++;
      if(cnt==7){
        st=i+1;
        break;
      }
    }
  }

  for(i=st;i<strlen(gps_buffer);i++){
    if(gps_buffer[i]==','){
      break;
    }
    buf[i-st]=gps_buffer[i];
    buf[i-st+1]=0;
  }

  strncpy(buf2,buf,2);
  DD=atoi(buf2);
  strncpy(buf2,buf+2,2);
  MM=atoi(buf2);
  strncpy(buf2,buf+4,2);
  YY=atoi(buf2);

  hh=hh+9;

  if(MM==2){
    days[2]=28+(1/( (YY+2000)%4 + 1)) * (1 - 1 / ((YY+2000) % 100 + 1)) + (1 / ((YY+2000) % 400 + 1));
  }

  if(hh>23){
    hh=hh-24;
    if(DD==days[MM]){
       DD=1;
       if(MM==12){
         MM=1;
         YY++;
       } else {
         MM++;
       }
    } else {
       DD++;
    }
  }

#if DEBUG
  sprintf(buf,"GPS:%2.2d/%2.2d/%2.2d %2.2d:%2.2d:%2.2d ",YY,MM,DD,hh,mm,ss);
  Serial.print(buf);
  sprintf(buf,"RTC:%2.2d/%2.2d/%2.2d %2.2d:%2.2d:%2.2d\n",
    tm.Year,tm.Month,tm.Day,tm.Hour,tm.Minute,tm.Second);
  Serial.print(buf);
  Serial.print("\n");
#endif

  tm.Year=YY+30;
  tm.Month=MM;
  tm.Day=DD;
  tm.Hour=hh;
  tm.Minute=mm;
  tm.Second=ss;
  RTC.write(tm);
  chkd=1;
}


