#include "glcdsia.h"

struct FontSpecs{
flash unsigned char *font; // default font after initialization
GLCDCOL_t backclr;
GLCDCOL_t foreclr;
}Font;



POINT glcdsia_cursor;

void glcdsia_init(int backcolor,int forecolor,flash unsigned char *defaultfont,char isLandscape)
{
if ( isLandscape)
 {
   ili9341_wrcmd(0x36); // display in landscape mode
   ili9341_wrdata(0x28);      
 } 
////else if (isLandscape==0)
// { 
// //do nothing
//   //ili9341_wrcmd(0x36); // display in landscape mode
//   //ili9341_wrdata(0x28);      //comment if  portraite
// } 

glcdsia_cursor.x = glcdsia_cursor.y =0;

Font.backclr = backcolor;
Font.foreclr = forecolor;
Font.font = defaultfont;
}

void SetAddrWindow(int x0, int y0, int x1, int y1)
{
ili9341_wrcmd(ILI9341_CMD_SET_COLUMN_ADDR); // set column range (x0,x1)
ili9341_wrdata(x0 >> 8);  //Start column hi byte
ili9341_wrdata(x0 & 0xff); //start col lo byte
ili9341_wrdata(x1 >> 8 );     //End col hi byte
ili9341_wrdata(x1 & 0xff);// End col lo byte

ili9341_wrcmd(ILI9341_CMD_SET_PAGE_ADDR); // set row range (y0,y1)
ili9341_wrdata(y0>>8 );     //Start row hi byte...
ili9341_wrdata(y0 & 0xff);
ili9341_wrdata(y1>>8);
ili9341_wrdata(y1 & 0xff);
}


void PutCh2 (int ch, int x, int y,char *fontH,char *fontW)
{ // write ch to display X,Y coordinates using  font
    //POINT fontHeightWidth;
    int pixel,startIndexOfChar=0, mask = 0x01;
    unsigned char maxBytePerRow=0, maxcol,numberOfchars=0,i=0,fontHeight,colcnt;
    char row, col, bit_, data,firstChar;
    numberOfchars = Font.font[3];
    firstChar = Font.font[2];
    fontHeight = Font.font[1];
    //index = ch-32;
    if(Font.font[0] == 0 ) // Proportional font
    {        //ch-32 yani indexe ch dar miane hame 0ta96 character
      maxcol = Font.font[ch-firstChar+4];/* standard fonts of codevisionAvr have 4 elements of describing font :
      in case of Fixed font size : 0x07,  Fixed font width 
                                    0x0E, Font height 
                                    0x20,  First character 
                                    0x60,  Number of characters in font 
      and in variable font width: 0x00,  Proportional font 
                                  0x0E,  Font height 
                                  0x20, First character 
                                  0x60,  Number of characters in font                              
      Therefore maximum column for each character may be different.
      */ 
      startIndexOfChar = 4+numberOfchars;
        for ( i=0; i<(ch-firstChar);i++)
        { 
          colcnt = Font.font[i+4];
          maxBytePerRow = ( (int)(colcnt/8)+( (colcnt%8>0) ? 1:0 ) ); //inke har satri chand byte mikhad
          startIndexOfChar += fontHeight* maxBytePerRow; 
        } 
    }
    else {
       maxcol = Font.font[0];/* Fixed font width */    
       startIndexOfChar = 4 + ((ch-firstChar))*fontHeight; 
    }   
    
    SetAddrWindow(x,y,x+maxcol-1,y+fontHeight-1); 
    ili9341_wrcmd(ILI9341_CMD_WR_MEM_START ); // 
   
    maxBytePerRow = ( (int)(maxcol/8)+( (maxcol%8>0) ? 1:0 ) );
    
    for (row=0;row<fontHeight;row++)//
    {
        for (col=0; col<maxcol;col++)
        {
           data = Font.font[startIndexOfChar+(int)(col/8)+maxBytePerRow*row]; 
           bit_ = data & mask;
           if (bit_==0) pixel=Font.backclr;
           else pixel = Font.foreclr;
           ili9341_wrdram(pixel);
           mask <<=1;
           if (mask == 0x100)   //2 bytes per pixel 0x01 and 0x10 then next lvl shift is not neccerery
            mask = 0x01;
        }
        mask = 0x01;
    }
     *fontH = maxcol;
     *fontW = fontHeight;
    
}



void glcdsia_PutJPG(int left,int top,flash unsigned char* img)
{
  unsigned int i;
  unsigned int data;
  int width = img[0] | (img[1] << 8) ;
  int height = img[2] | (img[3] << 8) ;   
    SetAddrWindow(left,top,left+width-1,top+height-1); 
    ili9341_wrcmd(ILI9341_CMD_WR_MEM_START );    
     for (i=0;i<width*height*2;i+=2)//
     {  
           data = img[4+i]|(img[4+i+1] << 8); 
           ili9341_wrdram(data);
     }
}

void glcdsia_outtext(char *text)
{
 
 char fontw,fonth;//number of pixels each character leaves behind
 unsigned char tlength = strlen(text); 
 unsigned int i=0; 
 //displacement = PutCh2(text[0],glcdsia_cursor.x,glcdsia_cursor.y);//quering the displacement of the font
 for (i=0;i<tlength;i++)
 {   
    if ( text[i] != '\n' && text[i] != '\r'){ 
    // displacement = PutCh2(text[i],glcdsia_cursor.x,glcdsia_cursor.y); 
       PutCh2(text[i],glcdsia_cursor.x,glcdsia_cursor.y,&fontw,&fonth);
     if (Font.font == font5x7)
        glcdsia_cursor.x += fontw+1;
     else  
        glcdsia_cursor.x += fontw+0; 
     if ( glcdsia_cursor.x > XSIZE-fontw+1)
     {
       glcdsia_cursor.x =0;
       glcdsia_cursor.y += fonth+1;
     }   
    }
    else if (text[i] == '\n'){  
      glcdsia_cursor.x =0;
      glcdsia_cursor.y += (fonth+1);
    }  
    else if (text[i] == '\r'){
      glcdsia_cursor.x =0;
    }   
    else {
        //glcdsia_outtext("*");//prints * in case of undefined char
    }
    
    if ( glcdsia_cursor.y > YSIZE-fonth+1)
    {
       glcdsia_cursor.y = 0;
    }
    
   
 }
 
}

void glcdsia_gotoxy(int xpos,int ypos)
{
  glcdsia_cursor.x = xpos;
  glcdsia_cursor.y = ypos;
}

void glcdsia_clear()
{
    unsigned int i,j;
    glcd_display(0);
    SetAddrWindow(0,0,XSIZE-1,YSIZE-1);
    ili9341_wrcmd(ILI9341_CMD_WR_MEM_START );
    for (i=0;i<XSIZE-1;i++)//
     {   
       for (j=0;j<YSIZE-1;j++)
           ili9341_wrdram(Font.backclr);
     }
     glcdsia_cursor.x=glcdsia_cursor.y=0;
     glcd_display(1);    
}

void glcdsia_setbackclr(int color)
{
    Font.backclr = color;
}

void glcdsia_setforeclr(int color)
{
 Font.foreclr = color;
}

void glcdsia_drawlineH(int xstart,int ystart,int xend ,unsigned char thickness)
{
    unsigned int i,j;
    
    SetAddrWindow(xstart,ystart,xend,thickness+ystart-1);
    ili9341_wrcmd(ILI9341_CMD_WR_MEM_START );
    for (i=0;i<xend-xstart+1;i++)//
     {   
       for (j=0;j<thickness;j++)
           ili9341_wrdram(Font.foreclr);
     }
   //  glcdsia_cursor.x+=xend-xstart+1;
    // glcdsia_cursor.y+=thickness;
        
}

void glcdsia_drawlineH2(int xstart,int ystart,int lenght ,unsigned char thickness)//horizontal line
{
    unsigned int i,j;
    
    SetAddrWindow(xstart,ystart,xstart+lenght-1,thickness+ystart-1);
    ili9341_wrcmd(ILI9341_CMD_WR_MEM_START );
    for (i=0;i<lenght;i++)//
     {   
       for (j=0;j<thickness;j++)
           ili9341_wrdram(Font.foreclr);
     }
   //  glcdsia_cursor.x+=xend-xstart+1;
    // glcdsia_cursor.y+=thickness;
        
}
void glcdsia_putpixel(int x , int y,int color)
{
   SetAddrWindow(x,y,x,y);
   ili9341_wrcmd(ILI9341_CMD_WR_MEM_START );
   ili9341_wrdram(color);
}

int glcdsia_getforeclr()
{
 return Font.foreclr;
}

int glcdsia_getbackclr()
{
 return Font.backclr;
}

void glcdsia_outtextxy(int x,int y,char *text)
{
 glcdsia_cursor.x = x;
  glcdsia_cursor.y = y;
 glcdsia_outtext(text);
 
}

void glcdsia_putImgSdCard(int xpos,int ypos,char* path,char *result,char turnLCDoff)
{
unsigned int bytesread=0;
char bytebuffer;
unsigned char msb=0,lsb=0;
unsigned int color2byte=0;
bool msb_or_lsb = LSB;
unsigned char linecnt=0;
unsigned int xdimension=0,ydimension=0;
char line[51];
bool bothbytesread= 0;
int i=0,j=0;


FRESULT res;
FATFS drive;
FIL file;

glcdsia_cursor.x= xpos;
glcdsia_cursor.y= ypos;

if(turnLCDoff)
  glcd_display(0);
  
  if ((res=f_mount(0,&drive))!=FR_OK) {
   sprintf( result,"Logical drive 0: mounted error :%d\n",res);
   }
  else {
    if ((res=f_open(&file,path,FA_READ|FA_WRITE))!=FR_OK)
       { 
       sprintf(result,"error openning file:%C code:%d\n",path, res);
       }
            else { 
             while (linecnt!=2 && (res=f_read(&file, &bytebuffer, 1, &bytesread) == 0)  ) //reading first two lines containig dimensions of the image file
                                                                                          //dimensions of the image 
                { 
                 line[i]=bytebuffer;
                 i++; 
                 if (line[i-1] == '\n')// 
                 { 
                    if (linecnt==0)//first line containing 2 bytes of image width size
                    {
                      j=strpos(line,'x');// low byte of width
                       if(line[j+1] >= 'A')
                         lsb |= (line[j+1]-'A'+10)<<4;
                       else if (line[j+1] >= '0')
                         lsb |= (line[j+1]-'0')<<4;
                                       
                       if(line[j+2] >= 'A')
                         lsb |= (line[j+2]-'A'+10);
                       else if (line[j+2] >= '0')
                         lsb |= (line[j+2]-'0'); 
                         
                     j=strrpos(line,'x');// high byte of width
                       if(line[j+1] >= 'A')
                         msb |= (line[j+1]-'A'+10)<<4;
                       else if (line[j+1] >= '0')
                         msb |= (line[j+1]-'0')<<4;
                                       
                       if(line[j+2] >= 'A')
                         msb |= (line[j+2]-'A'+10);
                       else if (line[j+2] >= '0')
                         msb |= (line[j+2]-'0');    
                      xdimension |= (lsb | msb<<8);   
                    } 
                    else   //second  line containing 2 bytes of image lengrthhfh size e.g. 0x3E, 0x01,
                    {  
                    lsb=msb=0;
                     j=strpos(line,'x');// low byte of 
                       if(line[j+1] >= 'A')
                         lsb |= (line[j+1]-'A'+10)<<4;
                       else if (line[j+1] >= '0')
                         lsb |= (line[j+1]-'0')<<4;
                                       
                       if(line[j+2] >= 'A')
                         lsb |= (line[j+2]-'A'+10);
                       else if (line[j+2] >= '0')
                         lsb |= (line[j+2]-'0'); 
                         
                     j=strrpos(line,'x');// high byte of 
                       if(line[j+1] >= 'A')
                         msb |= (line[j+1]-'A'+10)<<4;
                       else if (line[j+1] >= '0')
                         msb |= (line[j+1]-'0')<<4;
                                       
                       if(line[j+2] >= 'A')
                         msb |= (line[j+2]-'A'+10);
                       else if (line[j+2] >= '0')
                         msb |= (line[j+2]-'0');    
                      ydimension |= (lsb | msb<<8);  
                    }
                    linecnt++;
                    i=0;
                  }  
                 
                }
             
             linecnt = 0;
             lsb=msb=0  ; 
             SetAddrWindow(xpos,ypos,xpos+xdimension-1,ypos+ydimension-1);
             ili9341_wrcmd(ILI9341_CMD_WR_MEM_START );
             while ( (res=f_read(&file, line, 50, &bytesread) == 0) && bytesread==50  )
             {     
                    j=1;
                    if(line[j+1] >= 'A')
                     lsb |= (line[j+1]-'A'+10)<<4;
                    else //if (line[j+1] >= '0')
                     lsb |= (line[j+1]-'0')<<4;                  
                    if(line[j+2] >= 'A')
                      lsb |= (line[j+2]-'A'+10);
                    else //if (line[j+2] >= '0')
                      lsb |= (line[j+2]-'0'); 
                    j+=6;  
                    if(line[j+1] >= 'A')
                     msb |= (line[j+1]-'A'+10)<<4;
                    else //if (line[j+1] >= '0')
                      msb |= (line[j+1]-'0')<<4;                   
                    if(line[j+2] >= 'A')
                       msb |= (line[j+2]-'A'+10);
                     else //if (line[j+2] >= '0')
                       msb |= (line[j+2]-'0'); 
                    color2byte |= (lsb | msb<<8);  
                    ili9341_wrdram(color2byte);
                    lsb=msb=color2byte=0;   
                    
                    j+=6;
                    if(line[j+1] >= 'A')
                     lsb |= (line[j+1]-'A'+10)<<4;
                    else //if (line[j+1] >= '0')
                     lsb |= (line[j+1]-'0')<<4;                  
                    if(line[j+2] >= 'A')
                      lsb |= (line[j+2]-'A'+10);
                    else //if (line[j+2] >= '0')
                      lsb |= (line[j+2]-'0'); 
                    j+=6;  
                    if(line[j+1] >= 'A')
                     msb |= (line[j+1]-'A'+10)<<4;
                    else //if (line[j+1] >= '0')
                      msb |= (line[j+1]-'0')<<4;                   
                    if(line[j+2] >= 'A')
                       msb |= (line[j+2]-'A'+10);
                     else //if (line[j+2] >= '0')
                       msb |= (line[j+2]-'0'); 
                    color2byte |= (lsb | msb<<8);  
                    ili9341_wrdram(color2byte);
                    lsb=msb=color2byte=0;
                    
                    j+=6;
                    if(line[j+1] >= 'A')
                     lsb |= (line[j+1]-'A'+10)<<4;
                    else //if (line[j+1] >= '0')
                     lsb |= (line[j+1]-'0')<<4;                  
                    if(line[j+2] >= 'A')
                      lsb |= (line[j+2]-'A'+10);
                    else //if (line[j+2] >= '0')
                      lsb |= (line[j+2]-'0'); 
                    j+=6;  
                    if(line[j+1] >= 'A')
                     msb |= (line[j+1]-'A'+10)<<4;
                    else //if (line[j+1] >= '0')
                      msb |= (line[j+1]-'0')<<4;                   
                    if(line[j+2] >= 'A')
                       msb |= (line[j+2]-'A'+10);
                     else //if (line[j+2] >= '0')
                       msb |= (line[j+2]-'0'); 
                    color2byte |= (lsb | msb<<8);  
                    ili9341_wrdram(color2byte);
                    lsb=msb=color2byte=0;
                    
                    j+=6;
                    if(line[j+1] >= 'A')
                     lsb |= (line[j+1]-'A'+10)<<4;
                    else //if (line[j+1] >= '0')
                     lsb |= (line[j+1]-'0')<<4;                  
                    if(line[j+2] >= 'A')
                      lsb |= (line[j+2]-'A'+10);
                    else //if (line[j+2] >= '0')
                      lsb |= (line[j+2]-'0'); 
                    j+=6;  
                    if(line[j+1] >= 'A')
                     msb |= (line[j+1]-'A'+10)<<4;
                    else //if (line[j+1] >= '0')
                      msb |= (line[j+1]-'0')<<4;                   
                    if(line[j+2] >= 'A')
                       msb |= (line[j+2]-'A'+10);
                     else //if (line[j+2] >= '0')
                       msb |= (line[j+2]-'0'); 
                    color2byte |= (lsb | msb<<8);  
                    ili9341_wrdram(color2byte);
                    lsb=msb=color2byte=0;
              /*      
//                    j+=6+2;  //second line \r\n is also added
//                    if(line[j+1] >= 'A')
//                     lsb |= (line[j+1]-'A'+10)<<4;
//                    else if (line[j+1] >= '0')
//                     lsb |= (line[j+1]-'0')<<4;                  
//                    if(line[j+2] >= 'A')
//                      lsb |= (line[j+2]-'A'+10);
//                    else if (line[j+2] >= '0')
//                      lsb |= (line[j+2]-'0'); 
//                    j+=6;  
//                    if(line[j+1] >= 'A')
//                     msb |= (line[j+1]-'A'+10)<<4;
//                    else if (line[j+1] >= '0')
//                      msb |= (line[j+1]-'0')<<4;                   
//                    if(line[j+2] >= 'A')
//                       msb |= (line[j+2]-'A'+10);
//                     else if (line[j+2] >= '0')
//                       msb |= (line[j+2]-'0'); 
//                    color2byte |= (lsb | msb<<8);  
//                    ili9341_wrdram(color2byte);
//                    lsb=msb=color2byte=0; 
//                    
//                    j+=6;
//                    if(line[j+1] >= 'A')
//                     lsb |= (line[j+1]-'A'+10)<<4;
//                    else if (line[j+1] >= '0')
//                     lsb |= (line[j+1]-'0')<<4;                  
//                    if(line[j+2] >= 'A')
//                      lsb |= (line[j+2]-'A'+10);
//                    else if (line[j+2] >= '0')
//                      lsb |= (line[j+2]-'0'); 
//                    j+=6;  
//                    if(line[j+1] >= 'A')
//                     msb |= (line[j+1]-'A'+10)<<4;
//                    else if (line[j+1] >= '0')
//                      msb |= (line[j+1]-'0')<<4;                   
//                    if(line[j+2] >= 'A')
//                       msb |= (line[j+2]-'A'+10);
//                     else if (line[j+2] >= '0')
//                       msb |= (line[j+2]-'0'); 
//                    color2byte |= (lsb | msb<<8);  
//                    ili9341_wrdram(color2byte);
//                    lsb=msb=color2byte=0;
//                    
//                    j+=6;
//                    if(line[j+1] >= 'A')
//                     lsb |= (line[j+1]-'A'+10)<<4;
//                    else if (line[j+1] >= '0')
//                     lsb |= (line[j+1]-'0')<<4;                  
//                    if(line[j+2] >= 'A')
//                      lsb |= (line[j+2]-'A'+10);
//                    else if (line[j+2] >= '0')
//                      lsb |= (line[j+2]-'0'); 
//                    j+=6;  
//                    if(line[j+1] >= 'A')
//                     msb |= (line[j+1]-'A'+10)<<4;
//                    else if (line[j+1] >= '0')
//                      msb |= (line[j+1]-'0')<<4;                   
//                    if(line[j+2] >= 'A')
//                       msb |= (line[j+2]-'A'+10);
//                     else if (line[j+2] >= '0')
//                       msb |= (line[j+2]-'0'); 
//                    color2byte |= (lsb | msb<<8);  
//                    ili9341_wrdram(color2byte);
//                    lsb=msb=color2byte=0;
//                    
//                    j+=6;
//                    if(line[j+1] >= 'A')
//                     lsb |= (line[j+1]-'A'+10)<<4;
//                    else if (line[j+1] >= '0')
//                     lsb |= (line[j+1]-'0')<<4;                  
//                    if(line[j+2] >= 'A')
//                      lsb |= (line[j+2]-'A'+10);
//                    else if (line[j+2] >= '0')
//                      lsb |= (line[j+2]-'0'); 
//                    j+=6;  
//                    if(line[j+1] >= 'A')
//                     msb |= (line[j+1]-'A'+10)<<4;
//                    else if (line[j+1] >= '0')
//                      msb |= (line[j+1]-'0')<<4;                   
//                    if(line[j+2] >= 'A')
//                       msb |= (line[j+2]-'A'+10);
//                     else if (line[j+2] >= '0')
//                       msb |= (line[j+2]-'0'); 
//                    color2byte |= (lsb | msb<<8);  
//                    ili9341_wrdram(color2byte);
//                    lsb=msb=color2byte=0;
             */      
             }
             
             sprintf(result,"%s",(line)); 
             glcdsia_gotoxy(xpos+xdimension-1,ypos+ydimension-1);
            }
   }
   glcd_display(1);
}

void glcdsia_setfont(flash unsigned char *font)
{
  Font.font = font;
}
