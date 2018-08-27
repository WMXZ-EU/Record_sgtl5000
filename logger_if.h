/* SGTL5000 Recorder for Teensy 3.X
 * Copyright (c) 2018, Walter Zimmer
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice, development funding notice, and this permission
 * notice shall be included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */
 

#ifndef _LOGGER_IF_H
#define _LOGGER_IF_H

#include "kinetis.h"
#include "core_pins.h"

//==================== local uSD interface ========================================
// this implementation used SdFs from Bill Greiman
// which needs to be installed as local library 
//
uint32_t record_or_sleep(void);


#ifndef MAXFILE
  #define MAXFILE 100
#endif
#ifdef MAXBUF
  #define MAXBUF 200
#endif

#ifndef BUFFERSIZE
  #define BUFFERSIZE (8*1024)
#endif
int16_t diskBuffer[BUFFERSIZE];
int16_t *outptr = diskBuffer;


#include "mfs.h"

class c_uSD
{
  public:
    c_uSD(void): state(-1), closing(0) {;}
    void init(void);
    int16_t write(int16_t * data, int32_t ndat);
    uint16_t getNbuf(void) {return nbuf;}
    void setClosing(void) {closing=1;}

    int16_t close(void);
    void exit(void);
    
  private:
    int16_t state; // 0 initialized; 1 file open; 2 data written; 3 to be closed
    int16_t nbuf;
    int16_t closing;

    c_mFS mFS;

};
c_uSD uSD;


/*
 *  Logging interface support / implementation functions 
 */
 /*
//_______________________________ For File Time settings _______________________
#include <time.h>
#define EPOCH_YEAR 1970 //T3 RTC
#define LEAP_YEAR(Y) (((EPOCH_YEAR+Y)>0) && !((EPOCH_YEAR+Y)%4) && ( ((EPOCH_YEAR+Y)%100) || !((EPOCH_YEAR+Y)%400) ) )
static  const uint8_t monthDays[]={31,28,31,30,31,30,31,31,30,31,30,31}; 
*/
/*  int  tm_sec;
  int tm_min;
  int tm_hour;
  int tm_mday;
  int tm_mon;
  int tm_year;
  int tm_wday;
  int tm_yday;
  int tm_isdst;
*/
/*
struct tm seconds2tm(uint32_t tt)
{ struct tm tx;
  tx.tm_sec   = tt % 60;    tt /= 60; // now it is minutes
  tx.tm_min   = tt % 60;    tt /= 60; // now it is hours
  tx.tm_hour  = tt % 24;    tt /= 24; // now it is days
  tx.tm_wday  = ((tt + 4) % 7) + 1;   // Sunday is day 1 (tbv)

  // tt is now days since EPOCH_Year (1970)
  uint32_t year = 0;  
  uint32_t days = 0;
  while((unsigned)(days += (LEAP_YEAR(year) ? 366 : 365)) <= tt) year++;

  tx.tm_year = 1970+year; 

  // correct for last (actual) year
  days -= (LEAP_YEAR(year) ? 366 : 365);
  tt  -= days; // now tt is days in this year, starting at 0
  
  uint32_t mm=0;
  uint32_t monthLength=0;
  for (mm=0; mm<12; mm++) 
  { monthLength = monthDays[mm];
    if ((mm==1) & LEAP_YEAR(year)) monthLength++; 
    if (tt<monthLength) break;
    tt -= monthLength;
  }
  tx.tm_mon = mm + 1;   // jan is month 1  
  tx.tm_mday = tt + 1;     // day of month
  return tx;
}

uint32_t tm2seconds (struct tm *tx) 
{
  uint32_t tt;
  tt=tx->tm_sec+tx->tm_min*60+tx->tm_hour*3600;  

  // count days size epoch until previous midnight
  uint32_t days=tx->tm_mday;

  uint32_t mm=0;
  uint32_t monthLength=0;
  for (mm=0; mm<(tx->tm_mon-1); mm++) days+=monthDays[mm]; 
  if(tx->tm_mon>2 && LEAP_YEAR(tx->tm_year-1970)) days++;

  uint32_t years=0;
  while(years++ < (tx->tm_year-1970)) days += (LEAP_YEAR(years) ? 366 : 365);
  //  
  tt+=(days*24*3600);
  return tt;
}

// Call back for file timestamps (used by FS).  Only called for file create and sync().
void dateTime(uint16_t* date, uint16_t* time) 
{
  struct tm tx=seconds2tm(RTC_TSR);
    
  // Return date using FS_DATE macro to format fields.
  *date = FS_DATE(tx.tm_year, tx.tm_mon, tx.tm_mday);

  // Return time using FS_TIME macro to format fields.
  *time = FS_TIME(tx.tm_hour, tx.tm_min, tx.tm_sec);
}
*/

char *makeFilename(void)
{ static int ifl=0;
  static char filename[40];

  ifl++;
  if (ifl>MAXFILE) return 0;
//  sprintf(filename,"File%04d.raw",ifl);
//
  struct tm tx = seconds2tm(RTC_TSR);
//  sprintf(filename, "WMXZ_%04d_%02d_%02d_%02d_%02d_%02d", tx.tm_year, tx.tm_mon, tx.tm_mday, tx.tm_hour, tx.tm_min, tx.tm_sec);
  sprintf(filename, "%02d_%02d_%02d", tx.tm_hour, tx.tm_min, tx.tm_sec);
  #if DO_DEBUG>0
    Serial.println(filename);
  #endif
  return filename;  
}

char * headerUpdate(void);

//____________________________ FS Interface implementation______________________
void c_uSD::init(void)
{
  mFS.init();
  //
  nbuf=0;
  state=0;
}

void c_uSD::exit(void)
{ mFS.exit();
}

int16_t c_uSD::write(int16_t *data, int32_t ndat)
{
  if(state == 0)
  { // open file
    char *filename = makeFilename();
    if(!filename) {state=-1; return state;} // flag to do nothing anymore
    //
    mFS.open(filename);

    state=1; // flag that file is open
    nbuf=0;
  }
  
  if(state == 1 || state == 2)
  {  // write to disk
    state=2;
    mFS.write((unsigned char *) data, 2*ndat);
    nbuf++;
    if(nbuf==MAXBUF) state=3; // flag to close file
    record_or_sleep();  // check if record time is over
    if(closing) {closing=0; state=3;}
  }
  
  if(state == 3)
  {
    state=close();
  }
  return state;
}

int16_t c_uSD::close(void)
{
  mFS.close();
  
  state=0;  // flag to open new file
  return state;
}
#endif
