/*******************************************************
This program was created by the
CodeWizardAVR V3.12 Advanced
Automatic Program Generator
� Copyright 1998-2014 Pavel Haiduc, HP InfoTech s.r.l.
http://www.hpinfotech.com

Project : BioPrinter 
Version : 2.0
Date    : 7/3/2021
Author  : S.Esteki R.SIAMAKI
Company : Hitech 
Comments: 
This is a test program to setup different modules


Chip type               : ATmega2560
Program type            : Application
AVR Core Clock frequency: 16.000000 MHz
Memory model            : Small
External RAM size       : 0
Data Stack size         : 2048
*******************************************************/

#include <mega2560.h>
#include <io.h>

// Graphic Display functions
#include <glcd.h>
#include <delay.h>
#include <stdio.h>

#include <rtouch.h>

// Font used for displaying text
// on the graphic display
#include <font5x7.h>

#include "glcd_libs\glcdsia.h"
#include "glcd_fonts\arial_narrow_bold.h"
#include "glcd_fonts\arial_narrow.h"
#include "glcd_fonts\arial.h"
#include "glcd_icons\right.h"                          
#include "glcd_icons\left.h"
#include "glcd_icons\status.h"
#include "glcd_icons\time.h"
#include "glcd_icons\intensity.h"
#include "glcd_icons\desired.h"
#include "glcd_icons\on.h"
#include "glcd_icons\off.h"
#include "glcd_icons\select.h"
#include "glcd_icons\up.h"
#include "glcd_icons\down.h"
#include "glcd_icons\data.h"
#include "glcd_icons\logo.h"

#define ON 1
#define OFF 0
#define PAUSE 2

#define epsilon 0.1
#define dt 0.25 //100ms loop time


#define KpHG 10.0
#define KdHG 3.1
#define KiHG 0.6

//#define HEATER_RLY  PORTK.7


// Declare your global variables here
signed char item = 0, tab=0, maxtab=0;
char maxNumOfItems=5;// total count of modules embededd in device to be controlled
char lcdtext[100],string[50];
char tempstr[34];
int i;
char itemNamestr[24];
signed char  selectedItemID=0;
unsigned int startAddress;
int number = 0,option,MinuteDuration, SecondDuration,Group = 1 , PreventGroup = 0;
char cooldown_heater=0,cooldown_co2=0;
char bedFunction = 0,motorFunction = 0;
char heater_rly =OFF,co2_rly=OFF;
int currentvalue;			/*currentvalue situation */

int desiredvalue = 0;

int diff;
int startflag, powerstat;
int m;



char sensorstr[72];
//signed char currentvalue; 
//signed char  desiredvalue,powerstat=1,diff=0;
struct Item 
{
 char name[24];
 char ID;//byte 24
 char  durationminute;//25 
 char durationsecond; 
 char intensity;//27
 char desiredValue,//automatic control temperature //28
      currentSensorValue;  //desired temporal temperature //29
 char status;//30
 char maxIntensity //31
     ,minIntensity;//32
};
#define TOTAL_NUMBER_OF_ITEMS 16

//eeprom struct Item inletFan;
struct Item itemStruct;//declared only to better readablity of code when inquering the sizeof parameter.

eeprom struct Item hydrigelPump;
eeprom struct Item HGExtruderTemp;
eeprom struct Item coolerHeater;
eeprom struct Item Heating;
eeprom struct Item CO2valve;
eeprom struct Item Magnet;
eeprom struct Item Faninput;
eeprom struct Item Fanoutput;
eeprom struct Item chamberUV;
eeprom struct Item PlantLED;
eeprom struct Item Crossuv;
eeprom struct Item Plasma;
eeprom struct Item Pizo;
eeprom struct Item electroSpinPump;
eeprom struct Item electroSpinPower;
eeprom struct Item Laser;

eeprom char *itemName,*itemMaxIntensity,*itemMinIntensity,*previusItemName,*nextItemName;
eeprom signed char *itemIntensity ,*itemDesiredValue,*itemStatus,*itemID,*itemcurrentValue;
eeprom unsigned char *itemDurationminute , *itemDurationsecond ;
//eeprom struct Item inletFan;
//eeprom struct Item inletFan;
int hydrogelmeanTemp[5]={0,0,0,0,0},hotbedmeanTemp[5]={0,0,0,0,0},chambermeanTemp[5]={0,0,0,0,0},co2meanppm[5]={0,0,0,0,0};
int i;
void rtouchxy(unsigned short *xc,unsigned short *yc);
unsigned short touchedx,touchedy;
void SelectItem();
void initialDisplay();
void updateDisplayItems();
void updateDisplayValues();
float PIDcal(int setpoint,int actual_temp);
void Digitalvolume(char goal);
float PIDcalHG(int setpoint,int actual_temp);
float PIDcalBed(int setpoint,int actual_temp);

//float PIDcalChamber(int setpoint,int actual_temp);
//float PIDcalCO2(int setpoint,int actual_temp);

void strcpytoeep(eeprom char *dest,char *src)  // copies string src from RAM to dest in eeprom 
{
char i=0;
 while(src[i] != '\0')
 {
  dest[i]=src[i];
  i++;
 }
 dest[i]='\0';
}

void strcpyfromeep(char *dest,eeprom char *src)  // copies string src from eeprom to dest in RAM 
{
char i=0;
 while(src[i] != '\0')
 {
  dest[i]=src[i];
  i++;
 }
 dest[i]='\0';
}


signed char isNumber(char ch)//returns -1 if character is not in range of 0 to 9 
{
 signed char digit; 
 digit = ch-48; 
   
digit =  (digit<0 || digit>9)? -1 : digit;  //in range of 0to9
       return digit;
}

int extractElement(char str[],flash char element[])
{
 int number=0; 
 char ch_pos=0,i=0;
 signed char digit; 
 ch_pos = strstrf(str,element) -str ;// returns a pointer to the character in str1 where str2 begins
                                     //then, to calculate index(position) the pointer should be substracted from pointer representing beginig of the str 
ch_pos =  ch_pos+strlenf(element) + 1;   // ex. "goal=254 +1 is for '=' sign
     while((digit = isNumber(str[ch_pos+i])) >= 0)
     {
         number = number*10+digit;
         i++;
     }
 return number;
}
// Voltage Reference: AVCC pin
#define ADC_VREF_TYPE ((0<<REFS1) | (1<<REFS0) | (0<<ADLAR))

// Read the AD conversion result
unsigned int read_adc(unsigned char adc_input)
{
ADMUX=(adc_input & 0x1f) | ADC_VREF_TYPE;
if (adc_input & 0x20) ADCSRB|=(1<<MUX5);
else ADCSRB&=~(1<<MUX5);
// Delay needed for the stabilization of the ADC input voltage
delay_us(10);
// Start the AD conversion
ADCSRA|=(1<<ADSC);
// Wait for the AD conversion to complete
while ((ADCSRA & (1<<ADIF))==0);
ADCSRA|=(1<<ADIF);
return ADCW;
}

#define DATA_REGISTER_EMPTY (1<<UDRE0)
#define RX_COMPLETE (1<<RXC0)
#define FRAMING_ERROR (1<<FE0)
#define PARITY_ERROR (1<<UPE0)
#define DATA_OVERRUN (1<<DOR0)

// USART1 Receiver buffer
#define RX_BUFFER_SIZE1 128
char rx_buffer1[RX_BUFFER_SIZE1];

#if RX_BUFFER_SIZE1 <= 256
unsigned char rx_wr_index1=0,rx_rd_index1=0;
#else
unsigned int rx_wr_index1=0,rx_rd_index1=0;
#endif

#if RX_BUFFER_SIZE1 < 256
unsigned char rx_counter1=0;
#else
unsigned int rx_counter1=0;
#endif

// This flag is set on USART1 Receiver buffer overflow
bit usartReceived = false;

// USART1 Receiver interrupt service routine
interrupt [USART1_RXC] void usart1_rx_isr(void)
{
char status,data;
status=UCSR1A;
data=UDR1;
      usartReceived = true;
if ((status & (FRAMING_ERROR | PARITY_ERROR | DATA_OVERRUN))==0)
   {
    if (rx_wr_index1 == RX_BUFFER_SIZE1 || data == '#' ){
    rx_wr_index1=0;  
    }
    rx_buffer1[rx_wr_index1]=data;   
    rx_wr_index1++  ; 
          // usartReceived = true; 

   } 
            
if (rx_buffer1[rx_wr_index1-1] == '*' && rx_buffer1[0] =='#') //complete command has been recieved
   {  
      selectedItemID = extractElement(rx_buffer1,"ID");
       SelectItem(); 
       delay_ms(1);
       *itemIntensity = extractElement(rx_buffer1,"int");
       *itemDesiredValue = extractElement(rx_buffer1,"goal");
       *itemStatus = extractElement(rx_buffer1,"cmd");
       *itemDurationsecond = extractElement(rx_buffer1,"sec");
       *itemDurationminute = extractElement(rx_buffer1,"min"); 
       bedFunction = extractElement(rx_buffer1,"bedfun");
       rx_buffer1[0]= ''; 
       usartReceived = true; 
   }
}

// Get a character from the USART1 Receiver buffer
#pragma used+
char getchar1(void)
{
char data;
while (rx_counter1==0);
data=rx_buffer1[rx_rd_index1++];
#if RX_BUFFER_SIZE1 != 256
if (rx_rd_index1 == RX_BUFFER_SIZE1) rx_rd_index1=0;
#endif
#asm("cli")
--rx_counter1;
#asm("sei")
return data;
}
#pragma used-
// Write a character to the USART1 Transmitter
#pragma used+
void putchar1(char c)
{
while ((UCSR1A & DATA_REGISTER_EMPTY)==0);
UDR1=c;
}
#pragma used-

// USART2 Receiver buffer
#define RX_BUFFER_SIZE2 8
char rx_buffer2[RX_BUFFER_SIZE2];

#if RX_BUFFER_SIZE2 <= 256
unsigned char rx_wr_index2=0,rx_rd_index2=0;
#else
unsigned int rx_wr_index2=0,rx_rd_index2=0;
#endif

#if RX_BUFFER_SIZE2 < 256
unsigned char rx_counter2=0;
#else
unsigned int rx_counter2=0;
#endif

// This flag is set on USART2 Receiver buffer overflow
bit rx_buffer_overflow2;

// USART2 Receiver interrupt service routine
interrupt [USART2_RXC] void usart2_rx_isr(void)
{
char status,data;
status=UCSR2A;
data=UDR2;
if ((status & (FRAMING_ERROR | PARITY_ERROR | DATA_OVERRUN))==0)
   {
   rx_buffer2[rx_wr_index2++]=data;
#if RX_BUFFER_SIZE2 == 256
   // special case for receiver buffer size=256
   if (++rx_counter2 == 0) rx_buffer_overflow2=1;
#else
   if (rx_wr_index2 == RX_BUFFER_SIZE2) rx_wr_index2=0;
   if (++rx_counter2 == RX_BUFFER_SIZE2)
      {
      rx_counter2=0;
      rx_buffer_overflow2=1;
      }
#endif
   }
}

// Get a character from the USART2 Receiver buffer
#pragma used+
char getchar2(void)
{
char data;
while (rx_counter2==0);
data=rx_buffer2[rx_rd_index2++];
#if RX_BUFFER_SIZE2 != 256
if (rx_rd_index2 == RX_BUFFER_SIZE2) rx_rd_index2=0;
#endif
#asm("cli")
--rx_counter2;
#asm("sei")
return data;
}
#pragma used-

// Write a character to the USART2 Transmitter
#pragma used+
void putchar2(char c)
{
while ((UCSR2A & DATA_REGISTER_EMPTY)==0);
UDR2=c;
}
#pragma used-

void Puts1(char *st){
int n;

    for(n=0;n<strlen(st);n++){
        putchar1(st[n]);
    }
return;
}
// Timer 0 overflow interrupt service routine
interrupt [TIM0_OVF] void timer0_ovf_isr(void)
{
// Reinitialize Timer 0 value
TCNT0=0x70;
number++;
// Sense the Resistive Touchscreen Controller's /PENIRQ signal
rt_timerproc();
// Place your code here

if(number==25 || number==50 ||  number==75 || number==100)
{

for ( i=0;i<4;i++) {
     hydrogelmeanTemp[i] = hydrogelmeanTemp[i+1]; //moving average filter   
     hotbedmeanTemp[i] = hotbedmeanTemp[i+1]; //moving average filter 
     chambermeanTemp[i] = chambermeanTemp[i+1]; //moving average filter 
     co2meanppm[i] = co2meanppm[i+1]; //moving average filter 
    }
        hydrogelmeanTemp[4] = read_adc(1)*100*5/1024;
        hotbedmeanTemp[4] = read_adc(0)*100*5/1024;
        chambermeanTemp[4] = read_adc(2)*100*5/1024;
        co2meanppm[4] = ((read_adc(7)*100*5/1024)-40)*5/1.6;
        
        HGExtruderTemp.currentSensorValue = (hydrogelmeanTemp[0] + hydrogelmeanTemp[1] +hydrogelmeanTemp[2] + hydrogelmeanTemp[3] + hydrogelmeanTemp[4])/ 5;
        coolerHeater.currentSensorValue = (hotbedmeanTemp[0] + hotbedmeanTemp[1] +hotbedmeanTemp[2] + hotbedmeanTemp[3] + hotbedmeanTemp[4])/ 5; 
        Heating.currentSensorValue =  (chambermeanTemp[0] + chambermeanTemp[1] +chambermeanTemp[2] + chambermeanTemp[3] + chambermeanTemp[4])/ 5;
        CO2valve.currentSensorValue = (co2meanppm[0] + co2meanppm[1] +co2meanppm[2] + co2meanppm[3] + co2meanppm[4])/ 5; 
        
        
    if (HGExtruderTemp.status== ON)  //P7 CONNECTOR  
    {     
     OCR4AH = 0x00;
     OCR4AL = (int)PIDcalHG(HGExtruderTemp.desiredValue,HGExtruderTemp.currentSensorValue);    
    }
    else {
     OCR4AH = 0x00;
     OCR4AL = 0x00;
    }  
    if (coolerHeater.status == ON)
    {
     OCR1AH=0x00;
     OCR1AL=(int)PIDcalBed(coolerHeater.desiredValue,coolerHeater.currentSensorValue); ; 
    } 
    else {
     OCR1AH=0x00;
     OCR1AL=0x00;
    }
       
    }
// setting for duration of Items   (per 1 sec)
if (number == 100){
// itemStatus is off
//if ( *itemStatus == O) *itemDuration=0;
if ( hydrigelPump.status == ON ){   
 if (hydrigelPump.durationsecond == 0){ 
 if ( hydrigelPump.durationsecond ==0 && hydrigelPump.durationminute==0)
 hydrigelPump.status = OFF;
 else { hydrigelPump.durationminute = hydrigelPump.durationminute -1;
 hydrigelPump.durationsecond = 59; 
 }
 }
 else hydrigelPump.durationsecond= hydrigelPump.durationsecond-1;
}
if ( Magnet.status == ON ){
  
 if (Magnet.durationsecond == 0){ 
 if ( Magnet.durationsecond ==0 && Magnet.durationminute==0)
 Magnet.status = OFF;
 else { Magnet.durationminute = Magnet.durationminute -1;
 Magnet.durationsecond = 59; 
 }
 }
 else Magnet.durationsecond= Magnet.durationsecond-1;  
}
 if ( Faninput.status == ON ){
   
 if (Faninput.durationsecond == 0){ 
 if ( Faninput.durationsecond ==0 && Faninput.durationminute==0)
 Faninput.status = OFF;
 else { Faninput.durationminute = Faninput.durationminute -1;
 Faninput.durationsecond = 59; 
 }
 }     
 else Faninput.durationsecond= Faninput.durationsecond-1; 
}
 if ( Fanoutput.status == ON ){
  
 if (Fanoutput.durationsecond == 0){ 
 if ( Fanoutput.durationsecond ==0 && Fanoutput.durationminute==0)
 Fanoutput.status = OFF;
 else { Fanoutput.durationminute = Fanoutput.durationminute -1;
 Fanoutput.durationsecond = 59; 
 }
 }
 else  Fanoutput.durationsecond= Fanoutput.durationsecond-1; 
}
 if ( chamberUV.status == ON ){
  
 if (chamberUV.durationsecond == 0){ 
 if ( chamberUV.durationsecond ==0 && chamberUV.durationminute==0)
 chamberUV.status = OFF;
 else { chamberUV.durationminute = chamberUV.durationminute -1;
 chamberUV.durationsecond = 59; 
 }
 }    
 else chamberUV.durationsecond= chamberUV.durationsecond-1;  
}
if ( PlantLED.status == ON ){
 
 if (PlantLED.durationsecond == 0){ 
 if ( PlantLED.durationsecond ==0 && PlantLED.durationminute==0)
 PlantLED.status = OFF;
 else { PlantLED.durationminute = PlantLED.durationminute -1;
 PlantLED.durationsecond = 59; 
 }
 }  
 else PlantLED.durationsecond= PlantLED.durationsecond-1;   
}
if ( Crossuv.status == ON ){
 
 if (Crossuv.durationsecond == 0){ 
 if ( Crossuv.durationsecond ==0 && Crossuv.durationminute==0)
 Crossuv.status = OFF;
 else { Crossuv.durationminute = Crossuv.durationminute -1;
 Crossuv.durationsecond = 59; 
 }
 } 
 else  Crossuv.durationsecond= Crossuv.durationsecond-1;   
}
if ( Plasma.status == ON ){
 
 if (Plasma.durationsecond == 0){ 
 if ( Plasma.durationsecond ==0 && Plasma.durationminute==0)
 Plasma.status = OFF;
 else { Plasma.durationminute = Plasma.durationminute -1;
 Plasma.durationsecond = 59; 
 }
 }
 else Plasma.durationsecond= Plasma.durationsecond-1;   
}
if ( Pizo.status == ON ){
   
 if (Pizo.durationsecond == 0){ 
 if ( Pizo.durationsecond ==0 && Pizo.durationminute==0)
 Pizo.status = OFF;
 else { Pizo.durationminute = Pizo.durationminute -1;
 Pizo.durationsecond = 59; 
 }
 }  
 else Pizo.durationsecond= Pizo.durationsecond-1; 
}
if ( electroSpinPump.status == ON ){
 
 if (electroSpinPump.durationsecond == 0){ 
 if ( electroSpinPump.durationsecond ==0 && electroSpinPump.durationminute==0)
 electroSpinPump.status = OFF;
 else { electroSpinPump.durationminute = electroSpinPump.durationminute -1;
 electroSpinPump.durationsecond = 59; 
 }
 }
 else electroSpinPump.durationsecond= electroSpinPump.durationsecond-1;   
}
if ( electroSpinPower.status == ON ){
   
 if (electroSpinPower.durationsecond == 0){ 
 if ( electroSpinPower.durationsecond ==0 && electroSpinPower.durationminute==0)
 electroSpinPower.status = OFF;
 else { electroSpinPower.durationminute = electroSpinPower.durationminute -1;
 electroSpinPower.durationsecond = 59; 
 }
 } 
 else electroSpinPower.durationsecond= electroSpinPower.durationsecond-1; 
}
if ( Laser.status == ON ){
   
 if (Laser.durationsecond == 0){ 
 if ( Laser.durationsecond ==0 && Laser.durationminute==0)
 Laser.status = OFF;
 else { Laser.durationminute = Laser.durationminute -1;
 Laser.durationsecond = 59; 
 }
 } 
 else Laser.durationsecond= Laser.durationsecond-1; 
}

        if (cooldown_heater == 0)   { 
               if ( Heating.status == ON ) {
                if ( Heating.currentSensorValue < Heating.desiredValue)
                {
                   if ( heater_rly == OFF){
                   PORTK = ((1<<PORTK7) | PORTK);
                    cooldown_heater = 5;
                    heater_rly = ON; 
                  } 
                }
                else {
                 if (heater_rly == ON) {
                    PORTK = (PORTK & ~(1<<PORTK7));  
                    heater_rly = OFF;
                    cooldown_heater = 5; 
                 }   
                }
              }
              else {
               PORTK = (PORTK & ~(1<<PORTK7));
               heater_rly = OFF;
              } 
              
        }
        else {
            cooldown_heater--;
        }    
        
        if (cooldown_co2 == 0)   { 
               if ( CO2valve.status == ON ) {
                if ( CO2valve.currentSensorValue < CO2valve.desiredValue)
                {
                   if ( co2_rly == OFF){
                   DDRC.1=1;
                   PORTC.1 = 1;
                    cooldown_co2 = 5;
                    co2_rly = ON; 
                  } 
                }
                else {
                 if ( co2_rly == ON) {
                    PORTC.1 = 0;  
                    cooldown_co2 = OFF;
                    co2_rly = 5; 
                 }   
                }
              }
              else {
               PORTC.1 = 0;
               co2_rly = ON; 
              } 
              
        }
        else {
            cooldown_co2--;
        }
number =0; 
sprintf(sensorstr,"#ID1=%d&ID2=%d&ID3=%d&ID4=%d**",HGExtruderTemp.currentSensorValue,coolerHeater.currentSensorValue,Heating.currentSensorValue,CO2valve.currentSensorValue);
Puts1(sensorstr);
}
}

void initiateItems(){
//name :
strcpytoeep(hydrigelPump.name,"Hydrogel Pump");
strcpytoeep(HGExtruderTemp.name,"Hydrogel Temp.");
strcpytoeep(coolerHeater.name,"Cool/Heating Bed");
strcpytoeep(Heating.name,"Chamber Temp");
strcpytoeep(CO2valve.name,"Carbon Dioxide");
strcpytoeep(Magnet.name,"Magnetic Field");
strcpytoeep(Faninput.name,"Input Fan"); //6
strcpytoeep(Fanoutput.name,"Output Fan"); //7
strcpytoeep(chamberUV.name,"UV Sterilization");//8
strcpytoeep(PlantLED.name,"Chamber LED"); //9
strcpytoeep(Crossuv.name,"Crosslink UV");
strcpytoeep(Plasma.name,"Plasma ");
strcpytoeep(Pizo.name,"Piezoelectric");
strcpytoeep(electroSpinPump.name,"ElectroSpin Pump");
strcpytoeep(electroSpinPower.name,"ElectroSpin Power");
strcpytoeep(Laser.name,"Laser"); //15
// ID:
Laser.ID =15 ;electroSpinPower.ID=14;electroSpinPump.ID=13;Pizo.ID=12;  Plasma.ID = 11; Crossuv.ID = 10;PlantLED.ID=9;
chamberUV.ID=8;Fanoutput.ID=7;Faninput.ID=6;Magnet.ID=5;CO2valve.ID = 4;Heating.ID = 3;coolerHeater.ID=2;HGExtruderTemp.ID=1;hydrigelPump.ID=0;
// status:
Laser.status=HGExtruderTemp.status=Pizo.status= CO2valve.status=Plasma.status=Heating.status= Crossuv.status=PlantLED.status=chamberUV.status=Fanoutput.status=Faninput.status=coolerHeater.status=Magnet.status=electroSpinPower.status=electroSpinPump.status=hydrigelPump.status=0; // off

Laser.intensity = PlantLED.intensity = chamberUV.intensity = Fanoutput.intensity = Faninput.intensity = 100;

}


void main(void)
{

// Variable used to store graphic display
// controller initialization data
GLCDINIT_t glcd_init_data;

// Crystal Oscillator division factor: 1
#pragma optsize-
CLKPR=(1<<CLKPCE);
CLKPR=(0<<CLKPCE) | (0<<CLKPS3) | (0<<CLKPS2) | (0<<CLKPS1) | (0<<CLKPS0);
#ifdef _OPTIMIZE_SIZE_
#pragma optsize+
#endif

// Reset Source checking
if (MCUSR & (1<<PORF))
   {
   // Power-on Reset
   MCUSR=0;
   // Place your code here

   }
else if (MCUSR & (1<<EXTRF))
   {
   // External Reset
   MCUSR=0;
   // Place your code here

   }
else if (MCUSR & (1<<BORF))
   {
   // Brown-Out Reset
   MCUSR=0;
   // Place your code here

   }
else if (MCUSR & (1<<WDRF))
   {
   // Watchdog Reset
   MCUSR=0;
   // Place your code here

   }
else if (MCUSR & (1<<JTRF))
   {
   // JTAG Reset
   MCUSR=0;
   // Place your code here

   }

// Input/Output Ports initialization
// Port A initialization
// Function: Bit7=In Bit6=In Bit5=In Bit4=In Bit3=In Bit2=In Bit1=In Bit0=In 
DDRA=(0<<DDA7) | (0<<DDA6) | (0<<DDA5) | (0<<DDA4) | (0<<DDA3) | (1<<DDA2) | (1<<DDA1) | (1<<DDA0);
// State: Bit7=T Bit6=T Bit5=T Bit4=T Bit3=T Bit2=T Bit1=T Bit0=T 
PORTA=(0<<PORTA7) | (0<<PORTA6) | (0<<PORTA5) | (0<<PORTA4) | (0<<PORTA3) | (1<<PORTA2) | (1<<PORTA1) | (0<<PORTA0);

// Port B initialization
// Function: Bit7=In Bit6=Out Bit5=Out Bit4=In Bit3=In Bit2=In Bit1=In Bit0=In 
DDRB=(0<<DDB7) | (1<<DDB6) | (1<<DDB5) | (1<<DDB4) | (0<<DDB3) | (0<<DDB2) | (0<<DDB1) | (0<<DDB0);
// State: Bit7=T Bit6=0 Bit5=0 Bit4=T Bit3=T Bit2=T Bit1=T Bit0=T 
PORTB=(0<<PORTB7) | (0<<PORTB6) | (0<<PORTB5) | (0<<PORTB4) | (0<<PORTB3) | (0<<PORTB2) | (0<<PORTB1) | (0<<PORTB0);

// Port C initialization
// Function: Bit7=In Bit6=In Bit5=In Bit4=In Bit3=In Bit2=In Bit1=In Bit0=In 
DDRC=(0<<DDC7) | (0<<DDC6) | (0<<DDC5) | (0<<DDC4) | (0<<DDC3) | (0<<DDC2) | (1<<DDC1) | (1<<DDC0);
// State: Bit7=T Bit6=T Bit5=T Bit4=T Bit3=T Bit2=T Bit1=T Bit0=T 
PORTC=(0<<PORTC7) | (0<<PORTC6) | (0<<PORTC5) | (0<<PORTC4) | (0<<PORTC3) | (0<<PORTC2) | (1<<PORTC1) | (1<<PORTC0);

// Port D initialization
// Function: Bit7=In Bit6=In Bit5=In Bit4=In Bit3=In Bit2=In Bit1=In Bit0=In 
DDRD=(0<<DDD7) | (0<<DDD6) | (0<<DDD5) | (0<<DDD4) | (0<<DDD3) | (0<<DDD2) | (0<<DDD1) | (0<<DDD0);
// State: Bit7=T Bit6=T Bit5=T Bit4=T Bit3=T Bit2=T Bit1=T Bit0=T 
PORTD=(0<<PORTD7) | (0<<PORTD6) | (0<<PORTD5) | (0<<PORTD4) | (0<<PORTD3) | (0<<PORTD2) | (0<<PORTD1) | (0<<PORTD0);

// Port E initialization
// Function: Bit7=In Bit6=In Bit5=In Bit4=In Bit3=In Bit2=In Bit1=In Bit0=In 
DDRE=(0<<DDE7) | (0<<DDE6) | (1<<DDE5) | (1<<DDE4) | (1<<DDE3) | (0<<DDE2) | (0<<DDE1) | (0<<DDE0);
// State: Bit7=T Bit6=T Bit5=T Bit4=T Bit3=T Bit2=T Bit1=T Bit0=T 
PORTE=(0<<PORTE7) | (0<<PORTE6) | (0<<PORTE5) | (0<<PORTE4) | (0<<PORTE3) | (0<<PORTE2) | (0<<PORTE1) | (0<<PORTE0);

// Port F initialization
// Function: Bit7=In Bit6=In Bit5=In Bit4=In Bit3=In Bit2=In Bit1=In Bit0=In 
DDRF=(0<<DDF7) | (0<<DDF6) | (0<<DDF5) | (0<<DDF4) | (0<<DDF3) | (0<<DDF2) | (0<<DDF1) | (0<<DDF0);
// State: Bit7=T Bit6=T Bit5=T Bit4=T Bit3=T Bit2=T Bit1=T Bit0=T 
PORTF=(0<<PORTF7) | (0<<PORTF6) | (0<<PORTF5) | (0<<PORTF4) | (0<<PORTF3) | (0<<PORTF2) | (0<<PORTF1) | (0<<PORTF0);

// Port G initialization
// Function: Bit5=In Bit4=In Bit3=In Bit2=In Bit1=In Bit0=In 
DDRG=(0<<DDG5) | (0<<DDG4) | (0<<DDG3) | (0<<DDG2) | (0<<DDG1) | (0<<DDG0);
// State: Bit5=T Bit4=T Bit3=T Bit2=T Bit1=T Bit0=T 
PORTG=(0<<PORTG5) | (0<<PORTG4) | (0<<PORTG3) | (0<<PORTG2) | (0<<PORTG1) | (0<<PORTG0);

// Port H initialization
// Function: Bit7=In Bit6=In Bit5=Out Bit4=Out Bit3=Out Bit2=In Bit1=In Bit0=In 
DDRH=(0<<DDH7) | (1<<DDH6) | (1<<DDH5) | (1<<DDH4) | (1<<DDH3) | (0<<DDH2) | (0<<DDH1) | (0<<DDH0);
// State: Bit7=T Bit6=T Bit5=T Bit4=T Bit3=T Bit2=T Bit1=T Bit0=T 
PORTH=(0<<PORTH7) | (0<<PORTH6) | (0<<PORTH5) | (0<<PORTH4) | (0<<PORTH3) | (0<<PORTH2) | (0<<PORTH1) | (0<<PORTH0);

// Port J initialization
// Function: Bit7=In Bit6=In Bit5=In Bit4=In Bit3=In Bit2=In Bit1=In Bit0=In 
DDRJ=(1<<DDJ7) | (0<<DDJ6) | (0<<DDJ5) | (0<<DDJ4) | (0<<DDJ3) | (0<<DDJ2) | (0<<DDJ1) | (0<<DDJ0);
// State: Bit7=T Bit6=T Bit5=T Bit4=T Bit3=T Bit2=T Bit1=T Bit0=T 
PORTJ=(0<<PORTJ7) | (0<<PORTJ6) | (0<<PORTJ5) | (0<<PORTJ4) | (0<<PORTJ3) | (0<<PORTJ2) | (0<<PORTJ1) | (0<<PORTJ0);

// Port K initialization
// Function: Bit7=In Bit6=In Bit5=In Bit4=In Bit3=In Bit2=In Bit1=In Bit0=In 
DDRK=(1<<DDK7) | (1<<DDK6) | (0<<DDK5) | (0<<DDK4) | (0<<DDK3) | (0<<DDK2) | (1<<DDK1) | (1<<DDK0);
// State: Bit7=T Bit6=T Bit5=T Bit4=T Bit3=T Bit2=T Bit1=T Bit0=T 
PORTK=(0<<PORTK7) | (0<<PORTK6) | (0<<PORTK5) | (0<<PORTK4) | (0<<PORTK3) | (0<<PORTK2) | (0<<PORTK1) | (0<<PORTK0);

// Port L initialization
// Function: Bit7=In Bit6=In Bit5=In Bit4=In Bit3=In Bit2=In Bit1=In Bit0=In 
DDRL=(0<<DDL7) | (0<<DDL6) | (0<<DDL5) | (0<<DDL4) | (0<<DDL3) | (0<<DDL2) | (1<<DDL1) | (1<<DDL0);
// State: Bit7=T Bit6=T Bit5=T Bit4=T Bit3=T Bit2=T Bit1=T Bit0=T 
PORTL=(0<<PORTL7) | (0<<PORTL6) | (0<<PORTL5) | (0<<PORTL4) | (0<<PORTL3) | (0<<PORTL2) | (1<<PORTL1) | (1<<PORTL0);


// Timer/Counter 0 initialization
// Clock source: System Clock
// Clock value: 15..... kHz
// Mode: Normal top=0xFF
// OC0A output: Disconnected
// OC0B output: Disconnected
// Timer Period: 9.984 ms
TCCR0A=(0<<COM0A1) | (0<<COM0A0) | (0<<COM0B1) | (0<<COM0B0) | (0<<WGM01) | (0<<WGM00);
TCCR0B=(0<<WGM02) | (1<<CS02) | (0<<CS01) | (1<<CS00);
TCNT0=0x64;
OCR0A=0x00;
OCR0B=0x00;

// Timer/Counter 1 initialization
// Clock source: System Clock
// Clock value: 15.625 kHz
// Mode: Ph. correct PWM top=0x00FF
// OC1A output: Non-Inverted PWM
// OC1B output: Non-Inverted PWM
// OC1C output: Disconnected
// Noise Canceler: Off
// Input Capture on Falling Edge
// Timer Period: 32.64 ms
// Output Pulse(s):
// OC1A Period: 32.64 ms Width: 0 us// OC1B Period: 32.64 ms Width: 0 us
// Timer1 Overflow Interrupt: Off
// Input Capture Interrupt: Off
// Compare A Match Interrupt: Off
// Compare B Match Interrupt: Off
// Compare C Match Interrupt: Off
TCCR1A=(1<<COM1A1) | (0<<COM1A0) | (1<<COM1B1) | (0<<COM1B0) | (0<<COM1C1) | (0<<COM1C0) | (0<<WGM11) | (1<<WGM10);
TCCR1B=(0<<ICNC1) | (0<<ICES1) | (0<<WGM13) | (0<<WGM12) | (1<<CS12) | (0<<CS11) | (1<<CS10);
TCNT1H=0x00;
TCNT1L=0x00;
ICR1H=0x00;
ICR1L=0x00;
OCR1AH=0x00;
OCR1AL=0x00;
OCR1BH=0x00;
OCR1BL=0x00;
OCR1CH=0x00;
OCR1CL=0x00;


// Timer/Counter 2 initialization
// Clock source: System Clock
// Clock value: 15.625 kHz
// Mode: CTC top=OCR2A
// OC2A output: Toggle on compare match
// OC2B output: Toggle on compare match
// Timer Period: 0.064 ms
// Output Pulse(s):
// OC2A Period: 0.128 ms Width: 0.064 ms
// OC2B Period: 0.128 ms Width: 0.064 ms
ASSR=(0<<EXCLK) | (0<<AS2);
TCCR2A=(0<<COM2A1) | (1<<COM2A0) | (0<<COM2B1) | (1<<COM2B0) | (1<<WGM21) | (0<<WGM20);
TCCR2B=(0<<WGM22) | (1<<CS22) | (1<<CS21) | (1<<CS20);
TCNT2=0x00;
OCR2A=0x00;
OCR2B=0x00;

// Timer/Counter 3 initialization
// Clock source: System Clock
// Clock value: 62.500 kHz
// Mode: Ph. correct PWM top=0x00FF
// OC3A output: Non-Inverted PWM
// OC3B output: Non-Inverted PWM
// OC3C output: Non-Inverted
// Noise Canceler: Off
// Input Capture on Falling Edge
// Timer Period: 8.16 ms
// Output Pulse(s):
// OC3A Period: 8.16 ms Width: 0 us
// OC3B Period: 8.16 ms Width: 0 us
// OC3C Period: 8.16 ms Width: 0 us
// Timer3 Overflow Interrupt: Off
// Input Capture Interrupt: Off
// Compare A Match Interrupt: Off
// Compare B Match Interrupt: Off
// Compare C Match Interrupt: Off
TCCR3A=(1<<COM3A1) | (0<<COM3A0) | (1<<COM3B1) | (0<<COM3B0) | (1<<COM3C1) | (0<<COM3C0) | (0<<WGM31) | (1<<WGM30);
TCCR3B=(0<<ICNC3) | (0<<ICES3) | (0<<WGM33) | (0<<WGM32) | (1<<CS32) | (0<<CS31) | (0<<CS30);
TCNT3H=0x00;
TCNT3L=0x00;
ICR3H=0x00;
ICR3L=0x00;
OCR3AH=0x00;
OCR3AL=0x00;
OCR3BH=0x00;
OCR3BL=0x00;
OCR3CH=0x00;
OCR3CL=0x00;

// Timer/Counter 4 initialization
// Clock source: System Clock
// Clock value: 62.500 kHz
// Mode: Ph. correct PWM top=0x00FF
// OC4A output: Non-Inverted PWM
// OC4B output: Non-Inverted PWM
// OC4C output: Non-Inverted PWM
// Noise Canceler: Off
// Input Capture on Falling Edge
// Timer Period: 8.16 ms
// Output Pulse(s):
// OC4A Period: 8.16 ms Width: 0 us
// OC4B Period: 8.16 ms Width: 0 us
// OC4C Period: 8.16 ms Width: 0 us
// Timer4 Overflow Interrupt: Off
// Input Capture Interrupt: Off
// Compare A Match Interrupt: Off
// Compare B Match Interrupt: Off
// Compare C Match Interrupt: Off
TCCR4A=(1<<COM4A1) | (0<<COM4A0) | (1<<COM4B1) | (0<<COM4B0) | (1<<COM4C1) | (0<<COM4C0) | (0<<WGM41) | (1<<WGM40);
TCCR4B=(0<<ICNC4) | (0<<ICES4) | (0<<WGM43) | (0<<WGM42) | (1<<CS42) | (0<<CS41) | (0<<CS40);
TCNT4H=0x00;
TCNT4L=0x00;
ICR4H=0x00;
ICR4L=0x00;
OCR4AH=0x00;
OCR4AL=0x00;
OCR4BH=0x00;
OCR4BL=0x00;
OCR4CH=0x00;
OCR4CL=0x00;

// Timer/Counter 5 initialization
// Clock source: System Clock
// Clock value: Timer5 Stopped
// Mode: Normal top=0xFFFF
// OC5A output: Disconnected
// OC5B output: Disconnected
// OC5C output: Disconnected
// Noise Canceler: Off
// Input Capture on Falling Edge
// Timer5 Overflow Interrupt: Off
// Input Capture Interrupt: Off
// Compare A Match Interrupt: Off
// Compare B Match Interrupt: Off
// Compare C Match Interrupt: Off
TCCR5A=(0<<COM5A1) | (0<<COM5A0) | (0<<COM5B1) | (0<<COM5B0) | (0<<COM5C1) | (0<<COM5C0) | (0<<WGM51) | (0<<WGM50);
TCCR5B=(0<<ICNC5) | (0<<ICES5) | (0<<WGM53) | (0<<WGM52) | (0<<CS52) | (0<<CS51) | (0<<CS50);
TCNT5H=0x00;
TCNT5L=0x00;
ICR5H=0x00;
ICR5L=0x00;
OCR5AH=0x00;
OCR5AL=0x00;
OCR5BH=0x00;
OCR5BL=0x00;
OCR5CH=0x00;
OCR5CL=0x00;

// Timer/Counter 0 Interrupt(s) initialization
TIMSK0=(0<<OCIE0B) | (0<<OCIE0A) | (1<<TOIE0);

// Timer/Counter 1 Interrupt(s) initialization
TIMSK1=(0<<ICIE1) | (0<<OCIE1C) | (0<<OCIE1B) | (0<<OCIE1A) | (0<<TOIE1);

// Timer/Counter 2 Interrupt(s) initialization
TIMSK2=(0<<OCIE2B) | (0<<OCIE2A) | (0<<TOIE2);

// Timer/Counter 3 Interrupt(s) initialization
TIMSK3=(0<<ICIE3) | (0<<OCIE3C) | (0<<OCIE3B) | (0<<OCIE3A) | (0<<TOIE3);

// Timer/Counter 4 Interrupt(s) initialization
TIMSK4=(0<<ICIE4) | (0<<OCIE4C) | (0<<OCIE4B) | (0<<OCIE4A) | (0<<TOIE4);

// Timer/Counter 5 Interrupt(s) initialization
TIMSK5=(0<<ICIE5) | (0<<OCIE5C) | (0<<OCIE5B) | (0<<OCIE5A) | (0<<TOIE5);

// External Interrupt(s) initialization
// INT0: Off
// INT1: Off
// INT2: Off
// INT3: Off
// INT4: Off
// INT5: Off
// INT6: Off
// INT7: Off
EICRA=(0<<ISC31) | (0<<ISC30) | (0<<ISC21) | (0<<ISC20) | (0<<ISC11) | (0<<ISC10) | (0<<ISC01) | (0<<ISC00);
EICRB=(0<<ISC71) | (0<<ISC70) | (0<<ISC61) | (0<<ISC60) | (0<<ISC51) | (0<<ISC50) | (0<<ISC41) | (0<<ISC40);
EIMSK=(0<<INT7) | (0<<INT6) | (0<<INT5) | (0<<INT4) | (0<<INT3) | (0<<INT2) | (0<<INT1) | (0<<INT0);
// PCINT0 interrupt: Off
// PCINT1 interrupt: Off
// PCINT2 interrupt: Off
// PCINT3 interrupt: Off
// PCINT4 interrupt: Off
// PCINT5 interrupt: Off
// PCINT6 interrupt: Off
// PCINT7 interrupt: Off
// PCINT8 interrupt: Off
// PCINT9 interrupt: Off
// PCINT10 interrupt: Off
// PCINT11 interrupt: Off
// PCINT12 interrupt: Off
// PCINT13 interrupt: Off
// PCINT14 interrupt: Off
// PCINT15 interrupt: Off
// PCINT16 interrupt: Off
// PCINT17 interrupt: Off
// PCINT18 interrupt: Off
// PCINT19 interrupt: Off
// PCINT20 interrupt: Off
// PCINT21 interrupt: Off
// PCINT22 interrupt: Off
// PCINT23 interrupt: Off
PCMSK0=(0<<PCINT7) | (0<<PCINT6) | (0<<PCINT5) | (0<<PCINT4) | (0<<PCINT3) | (0<<PCINT2) | (0<<PCINT1) | (0<<PCINT0);
PCMSK1=(0<<PCINT15) | (0<<PCINT14) | (0<<PCINT13) | (0<<PCINT12) | (0<<PCINT11) | (0<<PCINT10) | (0<<PCINT9) | (0<<PCINT8);
PCMSK2=(0<<PCINT23) | (0<<PCINT22) | (0<<PCINT21) | (0<<PCINT20) | (0<<PCINT19) | (0<<PCINT18) | (0<<PCINT17) | (0<<PCINT16);
PCICR=(0<<PCIE2) | (0<<PCIE1) | (0<<PCIE0);

// USART0 initialization
// USART0 disabled
UCSR0B=(0<<RXCIE0) | (0<<TXCIE0) | (0<<UDRIE0) | (0<<RXEN0) | (0<<TXEN0) | (0<<UCSZ02) | (0<<RXB80) | (0<<TXB80);

// USART1 initialization
// Communication Parameters: 8 Data, 1 Stop, No Parity
// USART1 Receiver: On
// USART1 Transmitter: On
// USART1 Mode: Asynchronous
// USART1 Baud Rate: 9600
UCSR1A=(0<<RXC1) | (0<<TXC1) | (0<<UDRE1) | (0<<FE1) | (0<<DOR1) | (0<<UPE1) | (0<<U2X1) | (0<<MPCM1);
UCSR1B=(1<<RXCIE1) | (0<<TXCIE1) | (0<<UDRIE1) | (1<<RXEN1) | (1<<TXEN1) | (0<<UCSZ12) | (0<<RXB81) | (0<<TXB81);
UCSR1C=(0<<UMSEL11) | (0<<UMSEL10) | (0<<UPM11) | (0<<UPM10) | (0<<USBS1) | (1<<UCSZ11) | (1<<UCSZ10) | (0<<UCPOL1);
UBRR1H=0x00;
UBRR1L=0x67;

// USART2 initialization
// Communication Parameters: 8 Data, 1 Stop, No Parity
// USART2 Receiver: On
// USART2 Transmitter: On
// USART2 Mode: Asynchronous
// USART2 Baud Rate: 38400
UCSR2A=(0<<RXC2) | (0<<TXC2) | (0<<UDRE2) | (0<<FE2) | (0<<DOR2) | (0<<UPE2) | (0<<U2X2) | (0<<MPCM2);
UCSR2B=(1<<RXCIE2) | (0<<TXCIE2) | (0<<UDRIE2) | (1<<RXEN2) | (1<<TXEN2) | (0<<UCSZ22) | (0<<RXB82) | (0<<TXB82);
UCSR2C=(0<<UMSEL21) | (0<<UMSEL20) | (0<<UPM21) | (0<<UPM20) | (0<<USBS2) | (1<<UCSZ21) | (1<<UCSZ20) | (0<<UCPOL2);
UBRR2H=0x00;
UBRR2L=0x19;


// USART3 initialization
// USART3 disabled
UCSR3B=(0<<RXCIE3) | (0<<TXCIE3) | (0<<UDRIE3) | (0<<RXEN3) | (0<<TXEN3) | (0<<UCSZ32) | (0<<RXB83) | (0<<TXB83);

// Analog Comparator initialization
// Analog Comparator: Off
// The Analog Comparator's positive input is
// connected to the AIN0 pin
// The Analog Comparator's negative input is
// connected to the AIN1 pin
ACSR=(1<<ACD) | (0<<ACBG) | (0<<ACO) | (0<<ACI) | (0<<ACIE) | (0<<ACIC) | (0<<ACIS1) | (0<<ACIS0);
ADCSRB=(0<<ACME);
// Digital input buffer on AIN0: On
// Digital input buffer on AIN1: On
DIDR1=(0<<AIN0D) | (0<<AIN1D);

// ADC initialization
// ADC Clock frequency: 125.000 kHz
// ADC Voltage Reference: AVCC pin
// ADC Auto Trigger Source: ADC Stopped
// Digital input buffers on ADC0: On, ADC1: On, ADC2: On, ADC3: On
// ADC4: On, ADC5: On, ADC6: On, ADC7: On
DIDR0=(0<<ADC7D) | (0<<ADC6D) | (0<<ADC5D) | (0<<ADC4D) | (0<<ADC3D) | (0<<ADC2D) | (0<<ADC1D) | (0<<ADC0D);
// Digital input buffers on ADC8: On, ADC9: On, ADC10: On, ADC11: On
// ADC12: On, ADC13: On, ADC14: On, ADC15: On
DIDR2=(0<<ADC15D) | (0<<ADC14D) | (0<<ADC13D) | (0<<ADC12D) | (0<<ADC11D) | (0<<ADC10D) | (0<<ADC9D) | (0<<ADC8D);
ADMUX=ADC_VREF_TYPE;
ADCSRA=(1<<ADEN) | (0<<ADSC) | (0<<ADATE) | (0<<ADIF) | (0<<ADIE) | (1<<ADPS2) | (1<<ADPS1) | (1<<ADPS0);
ADCSRB=(0<<MUX5) | (0<<ADTS2) | (0<<ADTS1) | (0<<ADTS0);

// SPI initialization
// SPI disabled
SPCR=(0<<SPIE) | (0<<SPE) | (0<<DORD) | (0<<MSTR) | (0<<CPOL) | (0<<CPHA) | (0<<SPR1) | (0<<SPR0);

// TWI initialization
// TWI disabled
TWCR=(0<<TWEA) | (0<<TWSTA) | (0<<TWSTO) | (0<<TWEN) | (0<<TWIE);

// Graphic Display Controller initialization
// The ILI9341 connections are specified in the
// Project|Configure|C Compiler|Libraries|Graphic Display menu:
// DB8 - PORTJ Bit 0
// DB9 - PORTJ Bit 1
// DB10 - PORTJ Bit 2
// DB11 - PORTJ Bit 3
// DB12 - PORTJ Bit 4
// DB13 - PORTJ Bit 5
// DB14 - PORTJ Bit 6
// DB15 - PORTG Bit 2
// RS - PORTC Bit 3
// /CS - PORTC Bit 2
// /RD - PORTC Bit 5
// /WR - PORTC Bit 4
// /RST - PORTC Bit 6

// Specify the current font for displaying text
glcd_init_data.font=font5x7;
// No function is used for reading
// image data from external memory
glcd_init_data.readxmem=NULL;
// No function is used for writing
// image data to external memory
glcd_init_data.writexmem=NULL;
// Horizontal display not reversed
glcd_init_data.reverse_x=0;
// Vertical display not reversed
glcd_init_data.reverse_y=1;
// Color bit writing order: BGR
glcd_init_data.cl_bits_order=ILI9341_CL_BITS_BGR;
// Use default value for pump ratio
glcd_init_data.pump_ratio=ILI9341_DEFAULT_PUMP_RATIO;
// Use default value for DDVDH enhanced mode
glcd_init_data.ddvdh_enh_mode=ILI9341_DEFAULT_DDVDH_ENH_MODE;
// Use default value for CR timing
glcd_init_data.cr_timing=ILI9341_DEFAULT_CR_TIMING;
// Use default value for EQ timing
glcd_init_data.cr_timing=ILI9341_DEFAULT_EQ_TIMING;
// Use default value for precharge_timing
glcd_init_data.precharge_timing=ILI9341_DEFAULT_PRECHARGE_TIMING;
// Use default value for VRH
glcd_init_data.vrh=ILI9341_DEFAULT_VRH;
// Use default value for VCOML
glcd_init_data.vcoml=ILI9341_DEFAULT_VCOML;
// Use default value for VCOMH
glcd_init_data.vcomh=ILI9341_DEFAULT_VCOMH;
// Use default value for VCOM offset
glcd_init_data.vcom_offset=ILI9341_DEFAULT_VCOM_OFFSET;
// LCD type normally white
glcd_init_data.lcd_type=ILI9341_LCD_TYPE_WHITE;
// LCD scan mode: interlaced
glcd_init_data.scan_mode=ILI9341_SCAN_INTERLACED;
// Frame rate: 70Hz
glcd_init_data.frame_rate=ILI9341_FRAME_RATE_70;

glcd_init(&glcd_init_data);

// Global enable interrupts
#asm("sei")

rt_init(true);//enable touch
glcdsia_init(GLCD_CL_BLACK,GLCD_CL_WHITE,font5x7,true); 
glcdsia_setbackclr(GLCD_CL_WHITE);
glcdsia_clear();
   //tuen on the deviece    
//PORTF .5 = 0; 
//delay_ms (2000); 
//PORTF .5 = 1; 
//desiredvalue=5;
currentvalue = 2;  

initiateItems();
glcdsia_setforeclr(0x0210);
startAddress = (unsigned int)&hydrigelPump;// first Item defined in eeprom memory
selectedItemID=0;

glcdsia_PutJPG(35,70,logooo) ;
delay_ms(5000);
glcdsia_setbackclr(GLCD_CL_WHITE);
glcdsia_clear();

SelectItem();
initialDisplay();
updateDisplayItems();
Digitalvolume(150);
while (1)
      {   
          if(rt_touched)
          {
           rtouchxy(&touchedx,&touchedy);//read touch coordinates 
              if (touchedx < 40) // buttons to /decrease ID
              if ( touchedy < 70  &&  touchedy > 40)
               {
               selectedItemID--;
               if (selectedItemID < 0)
               selectedItemID=0;
               SelectItem(); 
               updateDisplayItems(); 
               }
           
               if ( touchedx > 280)  //increase   ID
               if ( touchedy < 70 && touchedy > 40)
               {
               if (selectedItemID < TOTAL_NUMBER_OF_ITEMS-1){
               selectedItemID++;
               SelectItem(); 
               updateDisplayItems(); 
               }
               }
                 // status touch
               if ( touchedx < 230  && touchedx > 170) {
               if ( touchedy < 210 && touchedy > 180){
               if (*itemStatus == 0){
               *itemStatus=1;
               //glcdsia_PutJPG(170,180,on1);
               glcdsia_setforeclr(GLCD_CL_WHITE);
               glcdsia_drawlineH2(175,120,18,5);  
               glcdsia_drawlineH2(200,120,18,5);  // second coordinate
               glcdsia_setforeclr(0x0210);
               }
               else {  
                *itemStatus = 0;
               //glcdsia_PutJPG(170,180,off);
               glcdsia_setforeclr(GLCD_CL_WHITE);
               glcdsia_drawlineH2(175,120,18,5);  
               glcdsia_drawlineH2(200,120,18,5);  // second coordinate
               glcdsia_setforeclr(0x0210);
               }
               }
               }
               
                 
               if (*itemID < 5 && *itemID > 0) {// sorang temp,bed , heating and co2
               Group = 2;   
               initialDisplay();  
               updateDisplayValues(); 
               // current value
               if ( touchedy < 120 && touchedy > 100)
               if ( touchedx < 150  && touchedx > 25){ 
               option = 1; 
               glcdsia_setforeclr(GLCD_CL_WHITE);
               glcdsia_drawlineH2(2,105,20,100); 
               glcdsia_PutJPG(5,105,select);
               glcdsia_setforeclr(0x0210);
               sprintf(lcdtext,"%3d ",*itemcurrentValue);
               glcdsia_outtextxy(270,130,lcdtext);
               } 
               // desired select
               if ( touchedy < 160 && touchedy > 140 )
               if (touchedx < 150  && touchedx > 25){ 
               option = 2;  
               glcdsia_setforeclr(GLCD_CL_WHITE);
               glcdsia_drawlineH2(2,105,20,100);
               glcdsia_PutJPG(5,145,select);
               glcdsia_setforeclr(0x0210);
               sprintf(lcdtext,"%3d  ",*itemDesiredValue);
               glcdsia_outtextxy(270,130,lcdtext);
               } 
               // increase items
               if ( touchedx < 305 && touchedx > 265 )
               if (touchedy < 135  && touchedy > 100){ 
               if (option == 1){// current value  
               *itemcurrentValue = *itemcurrentValue+1;
               sprintf(lcdtext,"%3d  ",*itemcurrentValue);
               glcdsia_outtextxy(270,130,lcdtext);
               }
               if (option == 2){// desired  
               *itemDesiredValue = *itemDesiredValue + 1;
               sprintf(lcdtext,"%3d  ",*itemDesiredValue);
               glcdsia_outtextxy(270,130,lcdtext);
               sprintf(lcdtext,"%3d  ",*itemDesiredValue);
               glcdsia_outtextxy(270,130,lcdtext); 
               }
               } 
               //decrease item 
               if ( touchedx < 300 && touchedx > 270 )
               if (touchedy < 180  && touchedy > 155){ 
               if (option == 1){// duration
               *itemcurrentValue = *itemcurrentValue-1;
               sprintf(lcdtext,"%3d    ",*itemcurrentValue);
               glcdsia_outtextxy(270,130,lcdtext);
               }
               if (option == 2){// desired  
               *itemDesiredValue = *itemDesiredValue -1;
               sprintf(lcdtext,"%3d  ",*itemDesiredValue);
               glcdsia_outtextxy(270,130,lcdtext);
               }
               } 
               }
                             
               else {// Duration select 
               Group = 1;  
               initialDisplay();
               if ( touchedy < 120 && touchedy > 100)
               if ( touchedx < 150  && touchedx > 25){ 
               option = 1; 
               glcdsia_setforeclr(GLCD_CL_WHITE);
               glcdsia_drawlineH2(2,105,20,100); 
               glcdsia_PutJPG(5,105,select);
               glcdsia_setforeclr(0x0210);
               sprintf(lcdtext,"%3d ",*itemDurationminute);
               glcdsia_outtextxy(270,130,lcdtext);
               //glcdsia_drawlineH2(175,120,18,5);  // minute coordinate
               //glcdsia_drawlineH2(200,120,18,5);  // second coordinate
               } 
               // intensity select
               if ( touchedy < 160 && touchedy > 140 )
               if (touchedx < 150  && touchedx > 25){ 
               option = 2;  
               glcdsia_setforeclr(GLCD_CL_WHITE);
               glcdsia_drawlineH2(2,105,20,100);
               glcdsia_PutJPG(5,145,select);
               glcdsia_setforeclr(0x0210);
               sprintf(lcdtext,"%3d ",*itemIntensity);
               glcdsia_outtextxy(270,130,lcdtext);
               } 
               if (option == 1){
               // minute touch
               if ( touchedx < 195 && touchedx > 165 )
               if (touchedy < 115  && touchedy > 95){
               glcdsia_setforeclr(GLCD_CL_WHITE);
               glcdsia_drawlineH2(200,120,18,5);
               glcdsia_setforeclr(0x0210);
               glcdsia_drawlineH2(175,120,18,5);  // minute coordinate
               sprintf(lcdtext,"%3d ",*itemDurationminute );
               glcdsia_outtextxy(270,130,lcdtext);
               MinuteDuration = ON;
               SecondDuration = OFF;
               }   
               // second touch 
               if ( touchedx < 225 && touchedx > 200 )
               if (touchedy < 115  && touchedy > 95){
               glcdsia_setforeclr(GLCD_CL_WHITE);
               glcdsia_drawlineH2(175,120,18,5);
               glcdsia_setforeclr(0x0210);
               glcdsia_drawlineH2(200,120,18,5);  // second coordinate
               sprintf(lcdtext,"%3d ",*itemDurationsecond);
               glcdsia_outtextxy(270,130,lcdtext);
               MinuteDuration = OFF;
               SecondDuration = ON;
               }
               }
               else{
               glcdsia_setforeclr(GLCD_CL_WHITE);
               glcdsia_drawlineH2(175,120,18,5);  
               glcdsia_drawlineH2(200,120,18,5);  // second coordinate
               glcdsia_setforeclr(0x0210);
               }
               // increase items
               if ( touchedx < 305 && touchedx > 265 )
               if (touchedy < 135  && touchedy > 100){ 
               if (option == 1){// duration  
               if (MinuteDuration == ON){ 
               *itemDurationminute = *itemDurationminute +1;
               sprintf(lcdtext,"%3d ",*itemDurationminute );
               glcdsia_outtextxy(270,130,lcdtext);
               } 
               if (SecondDuration == ON){ 
               *itemDurationsecond = *itemDurationsecond +1;
                if (*itemDurationsecond > 59 ) *itemDurationsecond = 59;
               sprintf(lcdtext,"%3d ",*itemDurationsecond);
               glcdsia_outtextxy(270,130,lcdtext);
               }
               }
               if (option == 2){ // intensity
               *itemIntensity= *itemIntensity + 1;
               if (*itemIntensity > 100)*itemIntensity = 100; 
               if(*itemID==11 && *itemIntensity > 6)//plasma head
               {  
                 *itemIntensity = 6;
               }
               sprintf(lcdtext,"%3d  ",*itemIntensity);
               glcdsia_outtextxy(270,130,lcdtext);
               }
               } 
               //decrease item 
               if ( touchedx < 300 && touchedx > 270 )
               if (touchedy < 180  && touchedy > 155){ 
               if (option == 1){// duration
               if (MinuteDuration == ON){ 
               if (*itemDurationminute == 0) *itemDurationminute = 0; 
               else *itemDurationminute = *itemDurationminute -1;
               sprintf(lcdtext,"%3d ",*itemDurationminute );
               glcdsia_outtextxy(270,130,lcdtext);
               } 
               if (SecondDuration == ON){ 
               if (*itemDurationsecond == 0) *itemDurationsecond = 0; 
               else *itemDurationsecond = *itemDurationsecond -1; 
               sprintf(lcdtext,"%3d ",*itemDurationsecond);
               glcdsia_outtextxy(270,130,lcdtext);
               }
               }
               if (option == 2){ // intensity
               *itemIntensity= *itemIntensity - 1;
               if (*itemIntensity < 0)*itemIntensity = 0; 
               sprintf(lcdtext,"%3d  ",*itemIntensity);
               glcdsia_outtextxy(270,130,lcdtext);
               }
               } 
               }
              if( *itemID == 6 || *itemID == 7 || *itemID == 8  || *itemID == 15)
               {
                *itemIntensity = 100;
               }
              if(*itemID == 2 )//special case for HOtbed
              {
                if ( touchedx < 110 && touchedx > 75 )
                if (touchedy > 180  && touchedy < 205){ 
                 bedFunction =! bedFunction;
                }
                
                if (bedFunction == 0)//HOT 
                 { 
                   PORTA.2=PORTA.1=1;
                   glcdsia_outtextxy(75,180,"HOT   "); 
                 }
                 if (bedFunction == 1)//COOL 
                 { 
                   PORTA.2=PORTA.1=0;
                   glcdsia_outtextxy(75,180,"COLD ");  
                 }
              }  
               if(*itemID == 13 )//special case for electroSpinPump
              {   
                glcdsia_setforeclr(0x0210); 
                glcdsia_setfont(arial_narrow_bold);
                if ( touchedx < 110 && touchedx > 75 )
                if (touchedy > 180  && touchedy < 205){ 
                 motorFunction =! motorFunction;
                }
                
                if (motorFunction == 0)//RIGHT
                 { 
                   PORTK = (PORTK & ~(1<<PORTK0));                                  
                   glcdsia_outtextxy(75,180,"RIGHT   "); 
                 }
                 if (motorFunction == 1)//LEFT 
                 { 
                    PORTK=(1<<PORTK0);
                   glcdsia_outtextxy(75,180,"LEFT ");  
                 }
              }
              // updateDisplayValues(); 
              // delay_ms(10);
          }////end touch
          if(usartReceived)
          {
           SelectItem(); 
           delay_ms(20);
           updateDisplayItems(); 
           if( *itemID == 6 || *itemID == 7 || *itemID == 8  || *itemID == 15)
               {
                *itemIntensity = 100;
               }
              if (*itemID < 5 && *itemID > 0) {// sorang temp,bed , heating and co2
                Group = 2;   
                initialDisplay();    
                if(*itemID == 2 )//special case for HOtbed
                {
                if (bedFunction == 0)//HOT 
                 { 
                   PORTA.2=PORTA.1=1;
                   glcdsia_outtextxy(75,180,"HOT   "); 
                 }
                 if (bedFunction == 1)//COOL 
                 { 
                   PORTA.2=PORTA.1=0;
                   glcdsia_outtextxy(75,180,"COLD ");  
                 }
                }
              } 
              else {
                Group = 1;
                 if(*itemID == 13 )//special case for electroSpinPump
              {    
                if (motorFunction == 0)//RIGHT
                 { 
                   PORTK = (PORTK & ~(1<<PORTK0));                                  
                   glcdsia_outtextxy(75,180,"RIGHT   "); 
                 }
                 if (motorFunction == 1)//LEFT 
                 { 
                    PORTK=(1<<PORTK0);
                   glcdsia_outtextxy(75,180,"LEFT ");  
                 }
              }  
                initialDisplay();
              }
           updateDisplayValues();
           sprintf(string ," %d ",*itemID);
           Puts1(string);
           usartReceived = false;   
          } 

      updateDisplayValues(); 
       /*

eeprom struct Item hydrigelPump;
eeprom struct Item HGExtruderTemp;
eeprom struct Item coolerHeater;
eeprom struct Item Heating;
eeprom struct Item CO2valve;
eeprom struct Item Magnet;
eeprom struct Item Faninput;
eeprom struct Item Fanoutput;
eeprom struct Item chamberUV;
eeprom struct Item PlantLED;
eeprom struct Item Crossuv;
eeprom struct Item Plasma;
eeprom struct Item Pizo;
eeprom struct Item electroSpinPump;
eeprom struct Item electroSpinPower;
eeprom struct Item Laser;       
       */   
        
       if(electroSpinPump.status== ON)    // p2 connector
       {
         OCR2A = OCR2B= (100-electroSpinPump.intensity)*255/100;
       }
       else {
         OCR2A = OCR2B = 0;
       }
      if(Plasma.status == ON)
      {  
      DDRF.6 = 0;
      //PORTF.5=1;
      delay_ms(20);
      if (PINF.6 == 0){ 
      DDRF.5 = 1;
      PORTF.5=0  ; 
      delay_ms(2000);
      PORTF.5=1  ; 
      delay_ms(200); 
      currentvalue = 2; 
      }   
      DDRF.5 = 1;
      DDRF.6=1;
      startflag=1; 
      powerstat=0;   //0 FOR ON AND 1 FOR OFF 
      desiredvalue = Plasma.intensity;
      if (currentvalue > desiredvalue)
      {
      diff = desiredvalue+6-currentvalue ;
      for (m = 0; m < diff; m++)
      {  
      PORTF .5 = 0;    
      delay_ms (450);    
      PORTF .5 = 1;   
      delay_ms (300);
      }// end for
      currentvalue = desiredvalue;
      PORTF .6=powerstat;        
      }// end if
      else if (currentvalue < desiredvalue)
      {
      diff = desiredvalue - currentvalue;
      for (m = 0; m < diff; m++)
      {
      PORTF.5 = 0;
      delay_ms (450);
      PORTF .5 = 1;
      delay_ms (300);
      }// end for
      currentvalue = desiredvalue;
      PORTF .6=powerstat;                
      }
      else{
       PORTF .6=powerstat;
      }// end if
      startflag=0;  
      DDRF.5=0;
      // end while startflag
      }
      else PORTF .6 = 1;
      if (Crossuv.status== ON)    // p2 connector
       {
         OCR3BH=0x00;
         OCR3BL=Crossuv.intensity*255/100;
       }
       else {
         OCR3BH=0x00;
         OCR3BL=0x00;
       }
       
        if (PlantLED.status== ON)    // p2 connector
       {
         OCR3CH=0x00;
         OCR3CL=PlantLED.intensity*255/100;
       }
       else {
         OCR3CH=0x00;
         OCR3CL=0x00;
       }
       
      if (hydrigelPump.status== ON)    // p6 connector
       {
         OCR4BH=0x00;
         OCR4BL=hydrigelPump.intensity*255/100;
       }
       else {
         OCR4BH=0x00;
         OCR4BL=0x00;
       }
       
       if (HGExtruderTemp.status== OFF)  //P7 CONNECTOR  
       { 
       OCR4AH = 0x00;
       OCR4AL = 0x00;    
         //OCR4AH = 0x00;
         //OCR4AL = (int)PIDcalHG( HGExtruderTemp.desiredValue,HGExtruderTemp.currentSensorValue); 
//         sprintf(lcdtext,"%3d %3d  %3d  ",OCR4AL,HGExtruderTemp.desiredValue,HGExtruderTemp.currentSensorValue);
//               glcdsia_outtextxy(10,210,lcdtext);   
       }
       
       if (coolerHeater.status == ON)
       {
//       sprintf(lcdtext,"%3d %3d  %3d  ",OCR1AL,coolerHeater.desiredValue,coolerHeater.currentSensorValue);
//       glcdsia_outtextxy(10,210,lcdtext); 
       }
       else{
        OCR1AH=0x00;
        OCR1AL=0x00;
       }
       
//       if(Heating.status==OFF)
//       {
//          PORTK = (PORTK & ~(1<<PORTK7));// & PORTK);
//
////        sprintf(lcdtext,"%3x %3d  %3d  ",PORTK,cooldown_heater,(PORTK & (1<<PORTK7)));
////          glcdsia_outtextxy(10,210,lcdtext);   
////           //PORTK = ((1<<PORTK7) | PORTK);                  
//       }  
       
       if(Faninput.status==ON)//r3
       {
         PORTA.0 = 1;
       }
       else {
        PORTA.0=0;
       } 
       
       if(chamberUV.status==ON)//r3
       {
         PORTC.0 = 0;
       }
       else {
        PORTC.0=1;
       } 
       
       if(Fanoutput.status==ON)  //r7!
       {
         PORTJ = ((1<<PORTJ7) | PORTJ);
       }
       else {
        PORTJ = (PORTJ & ~(1<<PORTJ7));
       }     
       
       if (Magnet.status==ON)   //p4 connector
       {
         OCR4CH=0x00;
         OCR4CL=Magnet.intensity*255/100;
       }
       else{
         OCR4CH=0x00;
         OCR4CL=0x00;
       }
       if (Pizo.status==ON)   //p4 connector
       {
        Digitalvolume(Pizo.intensity);
       }
       else{
         Digitalvolume(0);
       } 
       if (Laser.status == ON){ 
       PORTK=(1<<PORTK6);
       }
       else PORTK = (PORTK & ~(1<<PORTK6));
           
      }//end while(1)
}






void SelectItem()
{
 itemName = (eeprom char*)(startAddress + selectedItemID*sizeof(hydrigelPump)); 
// previusItemName =(eeprom char*)(selectedItemID*sizeof(itemStruct) + 33);
//nextItemName =(eeprom char*)(selectedItemID*sizeof(itemStruct) - 33);
itemID =  (eeprom char*) (itemName + sizeof(hydrigelPump.name));
itemDurationminute =   (eeprom char*)(itemID + sizeof(hydrigelPump.ID)); 
itemDurationsecond =   (eeprom char*)(itemDurationminute + sizeof(hydrigelPump.durationminute));
itemIntensity  =  (eeprom signed char*)(itemDurationsecond + sizeof(hydrigelPump.durationsecond) ) ;//  the bytewise location of intensity in struct Item 
itemDesiredValue =  (eeprom signed char*)(itemIntensity + sizeof(hydrigelPump.intensity));
itemcurrentValue = (eeprom signed char*)(itemDesiredValue + sizeof(hydrigelPump.desiredValue));
itemStatus = (eeprom signed char*)(itemcurrentValue + sizeof(hydrigelPump.currentSensorValue));
itemMaxIntensity = (eeprom unsigned char*)(itemStatus + sizeof(hydrigelPump.status));
itemMinIntensity = (eeprom unsigned char*)(itemMaxIntensity + sizeof(hydrigelPump.maxIntensity));

}
void rtouchxy(unsigned short *xc,unsigned short *yc)
{
/*
rt_init(true );
for(x=0;x<239;x++)
    { 
      //glcdsia_putpixel(x,0,GLCD_CL_ORANGE); //portraite
      glcdsia_putpixel(0,x);
      glcdsia_putpixel(20,x);
      glcdsia_putpixel(50,x);
      glcdsia_putpixel(100,x);
      glcdsia_putpixel(130,x);
      glcdsia_putpixel(159,x);
      glcdsia_putpixel(200,x);
      glcdsia_putpixel(319,x);
    }
     while(1){
 if (rt_touched)//globar variable will be set to 1 when lcd touched
 { 
 glcdsia_gotoxy(10,10); 
  //rt_readctrl(&touchedx,&touchedy);  
  rtouchxy(&touchedx,&touchedy);
  glcdsia_putpixel(touchedx,touchedy,GLCD_CL_WHITE);
  sprintf(lcdtext,"x:%5d    y:%5d",touchedx,touchedy);
  glcdsia_outtext(lcdtext);
  //rt_touched=0;            
 }
}   
In order to calibrate resistive touch you have to draw these lines 
on TFT LCD then try to tap on those lines then insert the values of x direction of rt_readctrl(&x,&y)
to the equations bellow
*/
 unsigned short x,y;
 rt_readctrl(&x,&y);
 if (x<=1074)///for landscape lcd
    *xc = (1-((float)(x-200)/(float)(1074-200) )) * 159+159;
 else if ( x>1074 && x<=1240 )
    *xc =159- (((float)(x-1074)/(float)(1240-1074) )) * 29 ; 
 else if ( x>1240 && x<=1405)
    *xc = 130-(((float)(x-1240)/(float)(1405-1240) )) * 30 ;
 else if ( x>1405 && x<=1685)
    *xc = 100-(((float)(x-1405)/(float)(1685-1405) )) * 50 ; 
 else if ( x>1685 && x<=1850)
    *xc = 50-(((float)(x-1685)/(float)(1850-1685) )) *30 ;
 else if ( x>1850)
    *xc = 20-(((float)(x-1850)/(float)(1952-1850) )) *20 ;

 *yc = (float)(y-130)/(float)(1950-130) * YSIZE;
 

/////-----for portraite lcd
//if (x<=1074)
//    *yc = (1-((float)(x-200)/(float)(1074-200) )) * 159+159;
// else if ( x>1074 && x<=1240 )
//    *yc =159- (((float)(x-1074)/(float)(1240-1074) )) * 29 ; 
// else if ( x>1240 && x<=1405)
//    *yc = 130-(((float)(x-1240)/(float)(1405-1240) )) * 30 ;
// else if ( x>1405 && x<=1685)
//    *yc = 100-(((float)(x-1405)/(float)(1685-1405) )) * 50 ; 
// else if ( x>1685 && x<=1850)
//    *yc = 50-(((float)(x-1685)/(float)(1850-1685) )) *30 ;
// else if ( x>1850)
//    *yc = 20-(((float)(x-1850)/(float)(1952-1850) )) *20 ;
// *yc = 320-*yc;
// *xc = (float)(y-112)/(float)(1940-112) * XSIZE;  
// // if(*xc
}

void initialDisplay()
{
if (Group != PreventGroup) {
glcdsia_PutJPG(10,40,left);
glcdsia_PutJPG(280,40,right);
glcdsia_setfont(arial_narrow_bold);
if (*itemID < 5 && *itemID > 0) {// sorang temp,bed , heating and co2 
glcdsia_setforeclr(GLCD_CL_WHITE);
glcdsia_drawlineH2(5,100,250,135);
glcdsia_setforeclr(0x0210); 
glcdsia_PutJPG(25,100,data);     
glcdsia_outtextxy(55,100,"Current Value: ");
glcdsia_PutJPG(25,140,targets);
glcdsia_outtextxy(55,140,"Desired Value: ");
glcdsia_outtextxy(25,180,"Status:"); 
} 
else {
glcdsia_setforeclr(GLCD_CL_WHITE);
glcdsia_drawlineH2(5,100,250,135);
glcdsia_setforeclr(0x0210);      
glcdsia_PutJPG(25,100,time);
glcdsia_outtextxy(55,100,"Duration: ");
glcdsia_PutJPG(25,140,intensity);
glcdsia_outtextxy(55,140,"Intensity: ");
glcdsia_outtextxy(25,180,"Status:");
glcdsia_PutJPG(270,105,up);
glcdsia_PutJPG(270,155,down);
}
PreventGroup = Group ;
}
}

void updateDisplayItems()
{
    glcdsia_setfont(arial_narrow_bold); 
    glcdsia_setbackclr(GLCD_CL_WHITE);
    glcdsia_setforeclr(GLCD_CL_WHITE);
    glcdsia_drawlineH2(75,40,200,30); 
    strcpyfromeep(itemNamestr,itemName);//sprintf function do not have access to eeprom memory strings therefore we must use this function
    glcdsia_setforeclr(0x0210);                          //glcdsia_setforeclr(0xDEBB);
    sprintf(lcdtext,"%3d. %s",*itemID+1,itemNamestr);  
    glcdsia_outtextxy(82,40,lcdtext);

    glcdsia_setforeclr(GLCD_CL_WHITE);
    glcdsia_drawlineH2(0,0,320,35);
    glcdsia_setfont(arial);
    glcdsia_setforeclr(0x7BEF); 
    if (*itemID < 15){ 
         previusItemName =(eeprom char*)(selectedItemID*sizeof(itemStruct) + 33);
     strcpyfromeep(itemNamestr,previusItemName);                   
        sprintf(lcdtext,"%3d. %s ",*itemID+2,itemNamestr);  
        glcdsia_outtextxy(200,10,lcdtext);
        
    }
    if (*itemID > 0){   
 
     nextItemName =(eeprom char*)(selectedItemID*sizeof(itemStruct) - 33);
     strcpyfromeep(itemNamestr,nextItemName); 
        sprintf(lcdtext,"%3d. %s ",*itemID,itemNamestr); 
        glcdsia_outtextxy(10,10,lcdtext);
     }
     if ( *itemStatus == OFF){
    glcdsia_PutJPG(170,180,off);
    }
    if ( *itemStatus == ON) { 
    glcdsia_PutJPG(170,180,on1);
    } 
    glcdsia_setfont(arial_narrow_bold);
    glcdsia_setforeclr(0x0210); 
    if (*itemID < 4 && *itemID > 0) {// sorang temp,bed , heating and co2 
    sprintf(lcdtext," %3d 'C    ",*itemcurrentValue);
    glcdsia_outtextxy(170,100,lcdtext);
    sprintf(lcdtext," %3d 'C    ",*itemDesiredValue);
    glcdsia_outtextxy(170,140,lcdtext); 
    }
    else if 
    (*itemID==12 || *itemID==14 ) {
     glcdsia_setforeclr(GLCD_CL_WHITE);
    glcdsia_drawlineH2(75,180,50,50);
    }
    else {
    sprintf(lcdtext,"%3d :%3d    ",*itemDurationminute , *itemDurationsecond);
    glcdsia_outtextxy(170,100,lcdtext);
    sprintf(lcdtext,"%3d%%  ",*itemIntensity);
    glcdsia_outtextxy(170,140,lcdtext);
    }
    
    glcdsia_setforeclr(GLCD_CL_WHITE);
    glcdsia_drawlineH2(75,180,45,35);
    glcdsia_setforeclr(0x7BEF);
}

void updateDisplayValues()
{

    glcdsia_setforeclr(0x0210); 
    glcdsia_setfont(arial_narrow_bold);
     if ( *itemStatus == OFF){
    glcdsia_PutJPG(170,180,off);
    }
    if ( *itemStatus == ON) { 
    glcdsia_PutJPG(170,180,on1);
    }  
    if (*itemID < 4 && *itemID > 0) {// sorang temp,bed , heating and co2 
    sprintf(lcdtext," %3d 'C     ",*itemcurrentValue);
    glcdsia_outtextxy(170,100,lcdtext);
    sprintf(lcdtext," %3d 'C     ",*itemDesiredValue);
    glcdsia_outtextxy(170,140,lcdtext); 
    }
    else if 
    (*itemID==4) {// sorang temp,bed , heating and co2 
    sprintf(lcdtext,"%4d ppm   ",*itemcurrentValue*10);
    glcdsia_outtextxy(170,100,lcdtext);
    sprintf(lcdtext,"%4d ppm   ",*itemDesiredValue*10);
    glcdsia_outtextxy(170,140,lcdtext); 
    }
    else {
    sprintf(lcdtext,"%3d :%3d    ",*itemDurationminute , *itemDurationsecond);
    glcdsia_outtextxy(170,100,lcdtext);
    sprintf(lcdtext,"%3d%%  ",*itemIntensity);
    glcdsia_outtextxy(170,140,lcdtext);
    }
}

float PIDcalHG(int setpoint,int actual_temp)
{
    static float pre_error = 0;
    static float integral = 0;
    float error;
    float derivative;
    float output; 
    signed int abserror; 
    int min=0;
    
    //Caculate P,I,D
    error = (setpoint- actual_temp ); 

    //In case of error too small then stop integration 
    abserror = error; 
    if(error<0)
     abserror=-1*error; 
     
    if(abserror > epsilon ) //if setpoint is above current temp,integration cumulitively 
    {                                         //decreases therefore it takes a long time to compansate integral term
       integral = integral + error*dt;
       if(integral > 200)
       {
        integral=200;
       }
       else if (integral<-100)
       {
          integral=-100;
       }
    }
    derivative = (error - pre_error)/dt;
    output = KpHG*error + KiHG*integral + KdHG*derivative;
    //Saturation Filter
    
   // output=error;  
    if ( setpoint <14){
       min = 18;
    }
    else if (setpoint <18){
       min = 10;
    }
    else {
        min = 0;
    }  
    
    if(output > 255)
    {
        output = 255;
    }
    else if(output < min)
    {
        output = min;
    }
    //Update error
    //output = pre_error  ;
        pre_error = error;
    return output;
}


float PIDcalBed(int setpoint,int actual_temp)
{
    static float pre_error_bed = 0;
    static float integral_bed = 0;
    float error;
    float derivative;
    float output; 
    int abserror; 
    //signed int min=-255;
    float kp=0,ki,kd;
    //Caculate P,I,D
    error = -(setpoint - actual_temp );
    //In case of error too small then stop integration 
    abserror = error; 
    if(error<0)
     abserror=-1*error; 
    
    if (  bedFunction == 0)//HOT
    {
     kp=2;
     ki=0.3;
     kd=1.2; 
     error = -error;
    }
    else{
     kp=2;
     ki=1.8;
     kd=1.2;
    }
     
    if(abserror > epsilon ) //if setpoint is above current temp,integration cumulitively 
    {                                         //decreases therefore it takes a long time to compansate integral term
       integral_bed = integral_bed + error*dt;  
       if(integral_bed >200)
          integral_bed = 200;
       if(integral_bed <0)
          integral_bed = 0;
    }
    derivative = (error - pre_error_bed)/dt;
    output = kp*error + ki*integral_bed + kd*derivative;
    //Saturation Filter
    
      
//    if ( setpoint <14){
//       min = -255;
//    }
//    else if (setpoint <18){
//       min = -255;
//    }
//    else {
//        min = -255;
//    }  
    
    if(output > 255)
    {
        output = 255;
    }
    else if(output < 0)
    {
        output = 0;
    }
    
    
    //Update error
    //output = pre_error  ;
        pre_error_bed = error;
    return output;
}

void Digitalvolume(char goal)
{
 static char prevPosition = 99;
 signed char inc ; //increment
 char i;
 DDRB.0=1;
 DDRE.7 = DDRE.6 = 1;
 PORTB.0=0;
 goal = 100-goal;
 goal = (goal==100)? 99: goal;   
 inc =  goal -  prevPosition;
  
 PORTE.6 = ( inc > 0 ) ? 1 : 0; //0:increase 1:decrease  
 if (inc<0)
  inc*=-1; 
 for (i=0;i<inc;i++) //
   {
      PORTE.7=1;
      delay_us(5);
      PORTE.7=0;
      delay_us(5);
   } 
   prevPosition = goal; 
}
/*

float PIDcalChamber(int setpoint,int actual_temp)
{
    static float pre_error = 0;
    static float integral = 0;
    float error;
    float derivative;
    float output; 
    int abserror; 
    int min=0;
    //Caculate P,I,D
    error = -(setpoint- actual_temp );
    //In case of error too small then stop integration 
    abserror = error; 
    if(error<0)
     abserror=-1*error; 
     
    if(abserror > epsilon && error > 0) //if setpoint is above current temp,integration cumulitively 
    {                                         //decreases therefore it takes a long time to compansate integral term
       integral = integral + error*dt;
    }
    derivative = (error - pre_error)/dt;
    //output = Kp*error + Ki*integral + Kd*derivative;
    //Saturation Filter
    
      
    if ( setpoint <14){
       min = 18;
    }
    else if (setpoint <18){
       min = 10;
    }
    else {
        min = 0;
    }  
    
    if(output > MAX)
    {
        output = MAX;
    }
    else if(output < min)
    {
        output = min;
    }
    //Update error
    //output = pre_error  ;
        pre_error = error;
    return output;
}


float PIDcalCO2(int setpoint,int actual_temp)
{
    static float pre_error = 0;
    static float integral = 0;
    float error;
    float derivative;
    float output; 
    int abserror; 
    int min=0;
    //Caculate P,I,D
    error = -(setpoint- actual_temp );
    //In case of error too small then stop integration 
    abserror = error; 
    if(error<0)
     abserror=-1*error; 
     
    if(abserror > epsilon && error > 0) //if setpoint is above current temp,integration cumulitively 
    {                                         //decreases therefore it takes a long time to compansate integral term
       integral = integral + error*dt;
    }
    derivative = (error - pre_error)/dt;
    //output = Kp*error + Ki*integral + Kd*derivative;
    //Saturation Filter
    
      
    if ( setpoint <14){
       min = 18;
    }
    else if (setpoint <18){
       min = 10;
    }
    else {
        min = 0;
    }  
    
    if(output > MAX)
    {
        output = MAX;
    }
    else if(output < min)
    {
        output = min;
    }
    //Update error
    //output = pre_error  ;
        pre_error = error;
    return output;
}
*/