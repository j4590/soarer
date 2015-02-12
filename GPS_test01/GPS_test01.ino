/*
SOARER SUPER LIMITED CLOCK

RJ11-A(LCD)
pin1: SCL(Yellow)
pin2: SDA(Green)
pin3: LCD-GND(Blue)
pin4: VDD/RST(P/Orange)
pin5: LED-GND(Brown)
pin6: LED-VDD(Red)

RJ11-B(GPS)

pin5-green-Arduino 6
pin6-yellow-Arduino 7
*/

#define pinGpsRx 7 //GPS Rx (Data Out) のピン番号
#define pinGpsTx 6 //GPS Tx (Data In) のピン番号
#define SerialBaudrate 9600 //シリアル通信のボーレート
#define GpsBaudrate 9600 //GPSのボーレート
#include <SoftwareSerial.h>
#include <Wire.h>
#include <ST7032.h>
#include <MsTimer2.h>

#define READBUFFERSIZE (256)
#define DELIMITER	(",")




SoftwareSerial g_gps(pinGpsRx, pinGpsTx);  // ソフトウェアシリアルの設定
char g_szReadBuffer[READBUFFERSIZE] = "";
int  g_iIndexChar = 0;
char lon[64],lat[64];
ST7032 lcd1;
int wd=0;

char tsec[13][17];
char WD[8][4];

void setup() {
  #if 0
  Serial.begin(SerialBaudrate);  // シリアル通信の初期化
  Serial.println("Software Serial Test Start!");  // 開始メッセージの出力
  #endif

  strcpy(tsec[0],"*            ");
  strcpy(tsec[1]," *           ");
  strcpy(tsec[2],"  *          ");
  strcpy(tsec[3],"   *         ");
  strcpy(tsec[4],"    *        ");
  strcpy(tsec[5],"     *       ");
  strcpy(tsec[6],"      *      ");
  strcpy(tsec[7],"       *     ");
  strcpy(tsec[8],"        *    ");
  strcpy(tsec[9],"         *   ");
  strcpy(tsec[10],"          *  ");
  strcpy(tsec[11],"           * ");
  
  strcpy(WD[1],"Sun");
  strcpy(WD[2],"Mon");
  strcpy(WD[3],"Tue");
  strcpy(WD[4],"Wed");
  strcpy(WD[5],"Thu");
  strcpy(WD[6],"Fri");
  strcpy(WD[0],"Sat");
  
  
  
  lon[0]=0;lat[0]=0;
  
  lcd1.begin(8, 2);
  lcd1.setContrast(32);            // コントラスト設定
  lcd1.setCursor(0, 0);
  lcd1.print("        ");
  lcd1.setCursor(1, 0);
  lcd1.print(" SOARER 3000GT");
  lcd1.setCursor(1, 1);
  lcd1.print(" Super LIMITED");

  //delay(5000);
  //delay(5000);
  delay(5000);
  delay(5000);
  lcd1.setCursor(1, 0);
  lcd1.print("              ");
  lcd1.setCursor(1, 1);
  lcd1.print("              ");

  g_gps.begin(GpsBaudrate);  // ソフトウェアシリアルの初期化
  
//  MsTimer2::set(500, dots); // 500msごとにオンオフ
//  MsTimer2::start();
}

void dots() {
    lcd1.setCursor(5, 1);
    lcd1.print(".");

}

// 閏年の定義
// 西暦年が4で割り切れる年は閏年
// ただし、西暦年が100で割り切れる年は平年
// ただし、西暦年が400で割り切れる年は閏年
int IsLeapYear( int iYear )
{
	if( ( 0 == iYear % 4
	   && 0 != iYear % 100 )
	 || 0 == iYear % 400 )
	{
		return 1;
	}
	return 0;
}

int chk_weekday(int yyyy, int mm, int dd){
   int yy1,yy2;
   int c1,c2,c3,c4;
   int res;
   
   yy1=yyyy/100;
   yy2=yyyy%100;
   
   c1=(yy1*21)/4;
   c2=(yy2*5)/4;
   c3=(mm+1)*13/5;
   c4=dd;

   res=(c1+c2+c3+c4)%7;
   return(res);
}

// 日付を１日進める。
void IncrementDay( int& riYear, int& riMonth, int& riDay )
{
	switch( riMonth )
	{
	case 1:
		if( 31 == riDay){ riMonth = 2; riDay = 1; }
		else		    { riDay++; }
		break;
	case 2:
		if( 28 == riDay || 29 == riDay )
		{
			if( IsLeapYear( riYear ) )
			{	// 閏年（2月は29日まである）
				if( 29 == riDay){ riMonth = 3; riDay = 1; }
				else			{ riDay++; }
			}
			else
			{	// 閏年じゃない（2月は28日まで）
				riMonth = 3; riDay = 1;
			}
		}
		else			{ riDay++; }
		break;
	case 3:
		if( 31 == riDay){ riMonth = 4; riDay = 1; }
		else			{ riDay++; }
		break;
	case 4:
		if( 30 == riDay){ riMonth = 5; riDay = 1; }
		else			{ riDay++; }
		break;
	case 5:
		if( 31 == riDay){ riMonth = 6; riDay = 1; }
		else			{ riDay++; }
		break;
	case 6:
		if( 30 == riDay){ riMonth = 7; riDay = 1; }
		else			{ riDay++; }
		break;
	case 7:
		if( 31 == riDay){ riMonth = 8; riDay = 1; }
		else			{ riDay++; }
		break;
	case 8:
		if( 31 == riDay){ riMonth = 9; riDay = 1; }
		else			{ riDay++; }
		break;
	case 9:
		if( 30 == riDay){ riMonth = 10; riDay = 1; }
		else			{ riDay++; }
		break;
	case 10:
		if( 30 == riDay){ riMonth = 11; riDay = 1; }
		else			{ riDay++; }
		break;
	case 11:
		if( 30 == riDay){ riMonth = 12; riDay = 1; }
		else			{ riDay++; }
		break;
	case 12:
		if( 31 == riDay){ riYear++; riMonth = 1; riDay = 1; }
		else			{ riDay++; }
		break;
	}
}

// センテンスの解析。
// $GPRMCの場合、引数変数に、緯度、経度を入れ、戻り値 1 を返す。
// $GPRMC以外の場合、戻り値は 0 を返す。
int AnalyzeLineString( char szLineString[],
					   int& riYear, int& riMonth, int& riDay,
					   int& riHour, int& riMin, int& riSec )
{
	riYear = 0; riMonth = 0; riDay = 0;
	riHour = 0; riMin = 0; riSec = 0;

	// $GPRMC
	if( 0 != strncmp( "$GPRMC", szLineString, 6 ) )
	{
		return 0;
	}
	
	// $GPRMC,085120.307,A,3541.1493,N,13945.3994,E,000.0,240.3,181211,,,A*6A
	strtok( szLineString, DELIMITER );	// $GPRMC
	char* pszUTCTime = strtok( NULL, DELIMITER );	// UTC時刻(hhmmss.ss)
	strtok( NULL, DELIMITER );	// ステータス
	char* LON = strtok( NULL, DELIMITER );	// 緯度
	strtok( NULL, DELIMITER );	// 北緯か南緯か
	char* LAT = strtok( NULL, DELIMITER );	// 経度
	strtok( NULL, DELIMITER );	// 東経か西経か
	strtok( NULL, DELIMITER );	// 移動の速度
	strtok( NULL, DELIMITER );	// 移動の真方位
	char* pszUTCDate = strtok( NULL, DELIMITER );	// UTC日付(ddmmyy)

        sprintf(lon,"%-2.2s.%s",LON,LON+2);
        sprintf(lat,"%-3.3s.%s",LAT,LAT+3);
        
	if( NULL == pszUTCDate )
	{
		return 0;
	}

	long hhmmss = atol(pszUTCTime);
	riHour = hhmmss / 10000;
	riMin =  (hhmmss / 100) % 100;
	riSec =  hhmmss % 100;
	
	long ddmmyy = atol(pszUTCDate);
	riYear = 2000 + ddmmyy % 100;
	riMonth = (ddmmyy / 100) % 100;
	riDay = ddmmyy / 10000;

	// 日本標準時は、UTC（世界標準時）よりも９時間進んでいる。
	riHour += 9;
	if( 24 <= riHour )
	{	// 日付変更
		riHour - 24;
		IncrementDay( riYear, riMonth, riDay );
	}
	
	return 1;
}

// １行文字列の読み込み
// 0 : 読み取り途中。1 : 読み取り完了。
int ReadLineString(	SoftwareSerial& serial,
					char szReadBuffer[], const int ciReadBufferSize, int& riIndexChar,
					char szLineString[], const int ciLineStringSize )
{
	while( 1 )
	{
		char c = serial.read();
		if( -1 == c )
		{
			break;
		}
		if( '\r' == c  )
		{	// 終端
			szReadBuffer[riIndexChar] = '\0';
			strncpy( szLineString, szReadBuffer, ciLineStringSize - 1 );
			szLineString[ciLineStringSize - 1] = '\0';
			riIndexChar = 0;
			return 1;
		}
		else if( '\n' == c )
		{
			;	// 何もしない
		}
		else
		{	// 途中
			if( (ciReadBufferSize - 1) > riIndexChar )
			{
				szReadBuffer[riIndexChar] = c;
				riIndexChar++;
			}
		}
	}

	return 0;
}

void loop2() {
  if (g_gps.available()) { 
    Serial.write(g_gps.read());
  }
}


void loop()
{
        char ss[2];
	char szLineString[READBUFFERSIZE];
	if( !ReadLineString( g_gps,
						 g_szReadBuffer, READBUFFERSIZE, g_iIndexChar,
						 szLineString, READBUFFERSIZE ) )
	{	// 読み取り途中
		return;
	}
	// 読み取り完了

	int iYear, iMonth, iDay;
	int iHour, iMin, iSec;
	if( !AnalyzeLineString( szLineString,
							iYear, iMonth, iDay,
							iHour, iMin, iSec ) )
	{
		return;
	}
	// 日時を読み取れた。

	// LCD出力
	char szBuffer[16];
	//sprintf( szBuffer, "%02d %02d/%02d\n", iYear-2000, iMonth, iDay );
	//sprintf( szBuffer, "%02d/%02d'%02d",  iMonth, iDay,iYear-2000 );
	sprintf( szBuffer, "%04d%02d%02d", iYear, iMonth, iDay );

#if 0
    Serial.write(szBuffer);
#endif

    lcd1.setCursor(3, 1);
      sprintf(szBuffer,"%-12.12s",tsec[iSec/5]);
        lcd1.print(szBuffer);

	// g_lcd.setCursor( 0, 0 );
	// g_lcd.print( szBuffer );
         if( (iSec%2)==0 )strcpy(ss,":");
         else strcpy(ss,".");
         
        if(iHour>23)iHour=iHour-24;
        /*
         if(iHour<12){
	    sprintf( szBuffer, "AM %02d%s%02d  %d  %s %s\n", iHour, ss,iMin, iSec,lon,lat );
         } else {
             sprintf( szBuffer, "PM %02d%s%02d\n", iHour-12,ss, iMin, iSec );
      
         }
         */
         wd= chk_weekday(iYear, iMonth, iDay);
         //sprintf(szBuffer,"%2d%s%02d %s",iHour,ss,iMin,WD[wd]);
         sprintf(szBuffer,"%2d/%02d %3s %2d%s%02d  ",iMonth,iDay,WD[wd],iHour,ss,iMin);

#if 0
    Serial.write(szBuffer);
    Serial.write("\n");
#endif 
    lcd1.setCursor(0, 0);
    lcd1.print(szBuffer);

	// g_lcd.setCursor( 0, 1 );
	//g_lcd.print( szBuffer );
   
}

