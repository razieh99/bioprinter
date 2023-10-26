/*
 * lcd_touch.c
 *
 *  Created on: Oct 29, 2016
 *      Author: Danylo Ulianych
 *
 *
 *
 */

#include <stdlib.h>
#include "lcd_touch.h"
//#include "functions.h"
//#include "stm32f1xx_hal_conf.h"
//#include "tft.h"
#include "glcdsia.h"
#include "glcd.h"
#include "delay.h"
#include "stdio.h"
//#include "arial_narrow_bold.h"
/*#include "back.h"
#include "setting.h"
#include "sun.h"
#include "temp.h"
#include "hum.h"
#include "soil.h"
 */
#define TOUCH_ADC_X_MAX 965
#define TOUCH_ADC_X_MIN 225
#define TOUCH_ADC_Y_MIN 160
#define TOUCH_ADC_Y_MAX 940

/*static ADC_HandleTypeDef* hadcX ;
static ADC_HandleTypeDef* hadcY ;
static uint32_t ADC_ChannelX;
static uint32_t ADC_ChannelY;*/

// Voltage Reference: AVCC pin
#define ADC_VREF_TYPE ((0<<REFS1) | (1<<REFS0) | (0<<ADLAR))

void ADC_init(){

// ADC initialization
// ADC Clock frequency: 125.000 kHz
// ADC Voltage Reference: AVCC pin
ADMUX=ADC_VREF_TYPE;
ADCSRA=(1<<ADEN) | (0<<ADSC) | (0<<ADFR) | (0<<ADIF) | (0<<ADIE) | (1<<ADPS2) | (1<<ADPS1) | (0<<ADPS0);
SFIOR=(0<<ACME);
}
// Read the AD conversion result
unsigned int read_adc(unsigned char adc_input)
{
ADMUX=adc_input | ADC_VREF_TYPE;
// Delay needed for the stabilization of the ADC input voltage
delay_us(10);
// Start the AD conversion
ADCSRA|=(1<<ADSC);
// Wait for the AD conversion to complete
while ((ADCSRA & (1<<ADIF))==0);
ADCSRA|=(1<<ADIF);
return ADCW;
}
static uint32_t touchX() {
    /*
     * set the touch pins like this :
     *_yp ---------->  INPUT(ADC)   ------->  PA1
     *_ym ---------->  INPUT(LOW)   ------->  PB1
     *_xp ---------->  OUTPUT(HIGH) ------->  PB0
     *_xm ---------->  OUTPUT(LOW)  ------->  PA2
     */       
                     DDRA.4 = 0;   //set as input    D1
                     PORTA.4 = 0;  // SET AS LOW
                     DDRA.3 = 1;   // set as output   D0
                     PORTA.3 = 1 ; // set as high
//                     DDRA.6 = 1;   // set as output   RS
//                     PORTA.6 = 0 ; // set as low    
//                     DDRA.5 = 0;   //set as input
//                     PORTA.5 = 0;    // set as low      CS 
                     DDRF=(1<<DDF6) | (0<<DDF5) ;
                     PORTF= (0<<PORTF6) | (0<<PORTF5) ;
                                           

        return  read_adc(5);
}
static uint32_t touchY() {
      /*
       * set the touch pins like this :
       *_yp ---------> OUTPUT(HIGH) ------->  PA1
       *_ym --------->  OUTPUT(LOW) ------->  PB1
       *_xp ---------> INPUT(LOW)   ------->  PB0
       *_xm --------->  INPUT(ADC)  ------->  PA2
       */
       // coordinate y :
                     DDRA.3 = 0;   //set as input
                     PORTA.3 = 0;  // SET AS LOW
                     //DDRF.5 = 1;   // set as output
                     //PORTF.5 = 1 ; // set as high
                     DDRA.4 = 1;   // set as output
                     PORTA.4 = 0 ; // set as low 
                     //DDRF.6 = 0;   //set as input 
                     //PORTF.6= 0;    // set as low 
                     DDRF=(0<<DDF6) | (1<<DDF5) ;
                     PORTF= (0<<PORTF6) | (1<<PORTF5) ;
                                       

        return read_adc(6);

}


long map(long x, long in_min, long in_max, long out_min, long out_max) {
  return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

/*
 * Reads raw touch x- and y-positions and draw on the lcd
 */
 void ADC_sleep(){

// ADC initialization
// ADC disabled
ADCSRA=(0<<ADEN) | (0<<ADSC) | (0<<ADFR) | (0<<ADIF) | (0<<ADIE) | (0<<ADPS2) | (0<<ADPS1) | (0<<ADPS0);

}   
 
int LCD_Touch_Read() {
    uint32_t  x ,y;
    uint16_t x1,y1;  
    int i,back;
    char str1[20],str2[20];
    ADC_init();
    x = 0;  
    i = 0;
    for (i=0;i<=10;i++){
    x = x+touchX()/10; 
    delay_us(1);
    }     
     y = touchY();
    // scale ADC range to tft height
     x1 =240- map(x,TOUCH_ADC_X_MIN, TOUCH_ADC_X_MAX,240,0 );
    
    // scale ADC range to tft weight
     y1 = 320- map(y,TOUCH_ADC_Y_MIN,TOUCH_ADC_Y_MAX, 320,0);
    // set pins to draw 
                  DDRF=(1<<DDF6) | (1<<DDF5) ;
                   DDRA.3 = 1;
                   DDRA.4 = 1; 
      // setting touch
   if (200<x1 && x1<240) 
       if (10<y1 && y1<40) {
                             back = 1;
       }
                    // back touch
    if (5<x1 && x1<50) 
        if (270<y1 && y1<320) {
                                back = 2;
        } 
        if (back != 1 && back != 2) back =0; // no touch
return back;
}



void imag (int x1 , int y1 ,flash unsigned char data[])
{
int x ,i=0, y ,h,w, k = 4;
int color;
w = data[i+1];
w = w << 8;
w = w | data[i];
i = i+2;
h = data[i+1];
h = h << 8;
h = h | data[i];
x = x1 ;
y = y1;
while (x < x1+h)
{
	for (y = y1 ; y < y1+w ; y++){
		color = data[k+1];
		color = color << 8;
		color = color | data[k];
		glcdsia_putpixel(y, x,color);
		k=k+2;
	}
	x++;
	y = y1;
}
}

