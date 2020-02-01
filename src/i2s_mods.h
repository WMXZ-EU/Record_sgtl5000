/* SGTL5000 Recorder for Teensy 
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
 
/*
 * NOTE: changing frequency impacts the macros 
 *      AudioProcessorUsage and AudioProcessorUsageMax
 * defined in stock AudioStream.h
 */

#ifndef _I2S_MODS_H
#define _I2S_MODS_H
 
//#include "kinetis.h"
#include "core_pins.h"

#if defined(__MK66FX1M0__)
// attempt to generate dividers programmatically 
// always better to check 
void I2S_dividers(uint32_t *iscl, uint32_t fsamp, uint32_t nbits) 
{ 
    int64_t i1 = 1; 
    int64_t i2 = 1; 
    int64_t i3 = iscl[2]+1;
    int fcpu=F_CPU; 
    if((F_CPU == 48000000) || (F_CPU == 24000000)) fcpu=96000000; 
    float A=fcpu/2.0f/i3/(2.0f*nbits*fsamp); 
    float mn=1.0;  
    for(int ii=1;ii<=128;ii++)  
    { float xx; 
      xx=A*ii-(int32_t)(A*ii);  
      if(xx<mn && A*ii<256.0) { mn=xx; i1=ii; i2=A*ii;} //select first candidate 
    } 
    iscl[0] = (int) (i1-1); 
    iscl[1] = (int) (i2-1); 
    iscl[2] = (int) (i3-1); 
} 
 
void I2S_modification(uint32_t fsamp, uint16_t nbits) 
{ uint32_t iscl[3]; 

  iscl[2]=1; // this is good for PJRC sgtl5000 //  
  
  I2S_dividers(iscl, fsamp ,nbits); 
  #if DO_DEBUG >0 
    int fcpu=F_CPU; 
    if((F_CPU==48000000) || (F_CPU==24000000)) fcpu=96000000; 
    float mclk = fcpu * (iscl[0]+1.0f) / (iscl[1]+1.0f);
    float fs = (mclk / 2.0f / (iscl[2]+1.0f) / (2.0f*nbits)); 
      Serial.printf("%d %d: %d %d %d %d %d %d %d\n\r", 
          F_CPU, fcpu, fsamp, (int)fs, (int) mclk, nbits,iscl[0]+1,iscl[1]+1,iscl[2]+1); 
  #endif 
  // stop I2S 
  I2S0_RCSR &= ~(I2S_RCSR_RE | I2S_RCSR_BCE); 
 
  // modify sampling frequency 
  I2S0_MDR = I2S_MDR_FRACT(iscl[0]) | I2S_MDR_DIVIDE(iscl[1]); 

  // configure transmitter 
  I2S0_TCR2 = I2S_TCR2_SYNC(0) | I2S_TCR2_BCP | I2S_TCR2_MSEL(1) 
    | I2S_TCR2_BCD | I2S_TCR2_DIV(iscl[2]); 

  // configure receiver (sync'd to transmitter clocks) 
  I2S0_RCR2 = I2S_RCR2_SYNC(1) | I2S_TCR2_BCP | I2S_RCR2_MSEL(1) 
    | I2S_RCR2_BCD | I2S_RCR2_DIV(iscl[2]); 
 
  //restart I2S 
  I2S0_RCSR |= I2S_RCSR_RE | I2S_RCSR_BCE; 
} 

void I2S_stopClock(void)
{
  SIM_SCGC6 &= ~SIM_SCGC6_I2S;
  SIM_SCGC7 &= ~SIM_SCGC7_DMA;
  SIM_SCGC6 &= ~SIM_SCGC6_DMAMUX;
}

void I2S_startClock(void)
{
  SIM_SCGC6 |= SIM_SCGC6_I2S;
  SIM_SCGC7 |= SIM_SCGC7_DMA;
  SIM_SCGC6 |= SIM_SCGC6_DMAMUX;

  CORE_PIN23_CONFIG = PORT_PCR_MUX(6); // pin 23, PTC2, I2S0_TX_FS (LRCLK)
  CORE_PIN9_CONFIG  = PORT_PCR_MUX(6); // pin  9, PTC3, I2S0_TX_BCLK
  CORE_PIN11_CONFIG = PORT_PCR_MUX(6); // pin 11, PTC6, I2S0_MCLK
  
}

void I2S_stop(void)
{
    I2S0_RCSR &= ~(I2S_RCSR_RE | I2S_RCSR_BCE);
}

 #elif defined(__IMXRT1062__)
 #include "imxrt_hw.h"
 void I2S_start(void)
 {
  I2S1_RCSR |= (I2S_RCSR_RE | I2S_RCSR_BCE); 
  I2S1_TCSR |= (I2S_TCSR_TE | I2S_TCSR_BCE); 
 }
 void I2S_stop(void)
 {
   // stop I2S 
  I2S1_RCSR &= ~(I2S_RCSR_RE | I2S_RCSR_BCE); 
  I2S1_TCSR &= ~(I2S_TCSR_TE | I2S_TCSR_BCE); 
 }

 void setAudioFrequency(int fs)
 {
   	// PLL between 27*24 = 648MHz und 54*24=1296MHz
	int n1 = 4; //SAI prescaler 4 => (n1*n2) = multiple of 4
	int n2 = 1 + (24000000 * 27) / (fs * 256 * n1);

	double C = ((double)fs * 256 * n1 * n2) / 24000000;
	int c0 = C;
	int c2 = 10000;
	int c1 = C * c2 - (c0 * c2);
	set_audioClock(c0, c1, c2, 1);

  	// clear SAI1_CLK register locations
	CCM_CSCMR1 = (CCM_CSCMR1 & ~(CCM_CSCMR1_SAI1_CLK_SEL_MASK))
		   | CCM_CSCMR1_SAI1_CLK_SEL(2); // &0x03 // (0,1,2): PLL3PFD0, PLL5, PLL4
	CCM_CS1CDR = (CCM_CS1CDR & ~(CCM_CS1CDR_SAI1_CLK_PRED_MASK | CCM_CS1CDR_SAI1_CLK_PODF_MASK))
		   | CCM_CS1CDR_SAI1_CLK_PRED(n1-1) // &0x07
		   | CCM_CS1CDR_SAI1_CLK_PODF(n2-1); // &0x3f
	// Select MCLK
	IOMUXC_GPR_GPR1 = (IOMUXC_GPR_GPR1
		& ~(IOMUXC_GPR_GPR1_SAI1_MCLK1_SEL_MASK))
		| (IOMUXC_GPR_GPR1_SAI1_MCLK_DIR | IOMUXC_GPR_GPR1_SAI1_MCLK1_SEL(0));

 }

 void I2S_modification(uint32_t fsamp, uint16_t nbits) 
{
  I2S_stop();
  // modify sampling frequency 
	CCM_CCGR5 |= CCM_CCGR5_SAI1(CCM_CCGR_ON);
//PLL:
	int fs = fsamp;
  setAudioFrequency(fs);
/*	// PLL between 27*24 = 648MHz und 54*24=1296MHz
	int n1 = 4; //SAI prescaler 4 => (n1*n2) = multiple of 4
	int n2 = 1 + (24000000 * 27) / (fs * 256 * n1);

	double C = ((double)fs * 256 * n1 * n2) / 24000000;
	int c0 = C;
	int c2 = 10000;
	int c1 = C * c2 - (c0 * c2);
	set_audioClock(c0, c1, c2, true);

	// clear SAI1_CLK register locations
	CCM_CSCMR1 = (CCM_CSCMR1 & ~(CCM_CSCMR1_SAI1_CLK_SEL_MASK))
		   | CCM_CSCMR1_SAI1_CLK_SEL(2); // &0x03 // (0,1,2): PLL3PFD0, PLL5, PLL4
	CCM_CS1CDR = (CCM_CS1CDR & ~(CCM_CS1CDR_SAI1_CLK_PRED_MASK | CCM_CS1CDR_SAI1_CLK_PODF_MASK))
		   | CCM_CS1CDR_SAI1_CLK_PRED(n1-1) // &0x07
		   | CCM_CS1CDR_SAI1_CLK_PODF(n2-1); // &0x3f

	IOMUXC_GPR_GPR1 = (IOMUXC_GPR_GPR1 & ~(IOMUXC_GPR_GPR1_SAI1_MCLK1_SEL_MASK))
			| (IOMUXC_GPR_GPR1_SAI1_MCLK_DIR | IOMUXC_GPR_GPR1_SAI1_MCLK1_SEL(0));	//Select MCLK
*/
  // restart I2S 
  I2S_start();
}

void I2S_stopClock(void)
{
//  SIM_SCGC6 &= ~SIM_SCGC6_I2S;
//  SIM_SCGC7 &= ~SIM_SCGC7_DMA;
//  SIM_SCGC6 &= ~SIM_SCGC6_DMAMUX;
}

void I2S_startClock(void)
{
//  SIM_SCGC6 |= SIM_SCGC6_I2S;
//  SIM_SCGC7 |= SIM_SCGC7_DMA;
//  SIM_SCGC6 |= SIM_SCGC6_DMAMUX;

	CORE_PIN23_CONFIG = 3;  //1:MCLK
	CORE_PIN21_CONFIG = 3;  //1:RX_BCLK
	CORE_PIN20_CONFIG = 3;  //1:RX_SYNC
	CORE_PIN7_CONFIG  = 3;  //1:TX_DATA0	  
	CORE_PIN8_CONFIG  = 3;  //1:RX_DATA0
}

 #endif


// ********************************************** following is to change SGTL5000 samling rates ********************
#define SGTL5000_I2C_ADDR  0x0A  // CTRL_ADR0_CS pin low (normal configuration)
#define CHIP_DIG_POWER		0x0002
#define CHIP_CLK_CTRL     0x0004
#define CHIP_I2S_CTRL     0x0006
#define CHIP_ANA_POWER    0x0030 

#include "Wire.h"
unsigned int chipRead(unsigned int reg)
{
  unsigned int val;
  Wire.beginTransmission(SGTL5000_I2C_ADDR);
  Wire.write(reg >> 8);
  Wire.write(reg);
  if (Wire.endTransmission(false) != 0) return 0;
  if (Wire.requestFrom(SGTL5000_I2C_ADDR, 2) < 2) return 0;
  val = Wire.read() << 8;
  val |= Wire.read();
  return val;
}

bool chipWrite(unsigned int reg, unsigned int val)
{
  Wire.beginTransmission(SGTL5000_I2C_ADDR);
  Wire.write(reg >> 8);
  Wire.write(reg);
  Wire.write(val >> 8);
  Wire.write(val);
  if (Wire.endTransmission() == 0) return true;
  return false;
}

unsigned int chipModify(unsigned int reg, unsigned int val, unsigned int iMask)
{
  unsigned int val1 = (chipRead(reg)&(~iMask))|val;
  if(!chipWrite(reg,val1)) return 0;
  return val1;
}

void SGTL5000_modification(uint32_t fs_mode)
{ int sgtl_mode=(fs_mode-2); 
  if(sgtl_mode>3) sgtl_mode = 3; 
  if(sgtl_mode<0) sgtl_mode = 0;
  
//  write(CHIP_CLK_CTRL, 0x0004);  // 44.1 kHz, 256*Fs
//	write(CHIP_I2S_CTRL, 0x0130); // SCLK=32*Fs, 16bit, I2S format
  chipWrite(CHIP_CLK_CTRL, (sgtl_mode<<2));  // 256*Fs| sgtl_mode = 0:32 kHz; 1:44.1 kHz; 2:48 kHz; 3:96 kHz
}

void SGTL5000_enable(void)
{
  chipWrite(CHIP_ANA_POWER, 0x40FF); 
  chipWrite(CHIP_DIG_POWER, 0x0073); 
}

void SGTL5000_disable(void)
{
  chipWrite(CHIP_DIG_POWER, 0); 
  chipWrite(CHIP_ANA_POWER, 0); 
}

#endif
