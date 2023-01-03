#include <Adafruit_SSD1306.h>
#include <splash.h>
#include "U8glib.h"
#include <math.h>
#include <Adafruit_BMP085.h>  //Also runs BMP180
#include <Adafruit_GFX.h>


// Version Barometer_OLED_Analog_gauge_37 (revised LEDs) 
// Displays In/Hg  ver 35 Release date 12/17/22, DEW
//Special thanks to Kris Kasprzak for his OLED graphics code which were used here
// Pressure history plots oldest data to the right, current pressure on the left, 10 hrs history
// SSD1306 address 3C is for the analog pressure gauge
//SSD1306 address 3D is for the X-Y histogram pressure plot 
// 4 LEDs are used to indicate:
//Green, pressure going up, pin 50
//Red, pressure going down, pin 52
//Yellow, pressure remaining constant, pin 48
// Blue, means a large pressure step change from previous reading by >0.06 in of mercury , pin 46

// Once the barometer is working you must correct the barometer for your altitude. Adjust trimpot for altitude offset, Get local sea level  barometer setting from local airport
//This version is actually using the BMP180 but BMP085 driver is the same
//This  sketch to handles the graph and instantaneous dial display 
//This sketch uses the Arduino AT mega 2560 (Elegoo)
//Set array to 10 elements so graph is 0 -9 hrs , fixed precision on x graph. timer now at 1200000 so should be hour updates . This version is working
 
#define READ_PIN            A0
#define OLED_RESET          A4
#define LOGO16_GLCD_HEIGHT  16
#define LOGO16_GLCD_WIDTH   16

//  variables declerations


int x =0; //x cartesian axis index
double y =0;//y axis cartesian index
double Timer =120001; //update time eventually 1 hr , set timer to 120001 so it draws the graph first time through 
float  History[11]={29.92,29.92,29.92,29.92,29.92,29.92,29.92,29.92,29.92,29.92}; //Declare history array, 29.92 is standard pressure at sea level
float RelHistory[11]= {0,0,0,0,0,0,0,0,0,0,}; //Initialize relative array to zero, relative array gets rid of the huge nominal offset for finer graph resolution 

// these are a required variables for the graphing functions
bool Redraw1 = true;
bool Redraw2 = true;
bool Redraw3 = true;
bool Redraw4 = true;
double ox , oy ; //ox and oy are plotting orgins


// create the display object for the histograph
Adafruit_SSD1306 display(OLED_RESET);


U8GLIB_SSD1306_128X64 u8g(U8G_I2C_OPT_NONE|U8G_I2C_OPT_DEV_0);
//Declare variables 
int xmax=128;                                  
int ymax=62;                                   
int xcenter=xmax/2;                            
int ycenter=ymax/2+10;                        
int arc=ymax/2;                             
int angle=0;
                                       
char* label[]={"In/Hg","Pascals","Bar", "VOLT"}; //This sets the label , choose only one label//
int labelXpos[] = {53, 45, 49, 53}; 
char prs [5]; //declares ascii pressure buffer  string, prs                
                            
double p;
int  w, mp,a=10;  //
int hx = 0; //hx is histogram counter pointer
double Offset = 0; //zero offset 
int psi;  //psi is the pressure converted to an integer 

double old=29.92 ; 
double newx=29.92;




u8g_uint_t xx=0;
Adafruit_BMP085 bmp;  //BMP decleration
//Draw the analog gauge

void gauge(uint8_t angle) {
  // draw border of the gauge
  u8g.drawCircle(xcenter,ycenter,arc+6, U8G_DRAW_UPPER_RIGHT);
  u8g.drawCircle(xcenter,ycenter,arc+4, U8G_DRAW_UPPER_RIGHT);
  u8g.drawCircle(xcenter,ycenter,arc+6, U8G_DRAW_UPPER_LEFT);
  u8g.drawCircle(xcenter,ycenter,arc+4, U8G_DRAW_UPPER_LEFT);
  // draw the needle
  float x1=sin(2*angle*2*3.14/360);           
  float y1=cos(2*angle*2*3.14/360); 
  u8g.drawLine(xcenter, ycenter, xcenter+arc*x1, ycenter-arc*y1);
  u8g.drawDisc(xcenter, ycenter, 5, U8G_DRAW_UPPER_LEFT);
  u8g.drawDisc(xcenter, ycenter, 5, U8G_DRAW_UPPER_RIGHT);
  u8g.setFont(u8g_font_chikita); 
  // show pressure scale labels , in inches of mercury
  u8g.drawStr( 12, 42, "28");                   
  u8g.drawStr( 25, 18, "29");
  u8g.drawStr( 55, 14, "29.5");
  u8g.drawStr( 95, 18, "30");
  u8g.drawStr( 105, 42, "31"); 
  // show gauge label
  u8g.setPrintPos(labelXpos[0],32);            
  u8g.print(label[0]); //  this prints the gauge label from the array ,0= In/Hg, 
  // show digital value and align its position
  u8g.setFont(u8g_font_profont22);             
  u8g.setPrintPos(35,60);
 
 
  u8g.print(prs);  // This is the Baro pressure & altitude offset, its a ascii char array, printing to baro gauge readout

}
void setup(void) {
  u8g.setFont(u8g_font_chikita);
  u8g.setColorIndex(1);                        
  // assign default color value 
  if ( u8g.getMode() == U8G_MODE_R3G3B2 ) {
    u8g.setColorIndex(255);                    
  }
  else if ( u8g.getMode() == U8G_MODE_GRAY2BIT ) {
    u8g.setColorIndex(3);                      
  }
  else if ( u8g.getMode() == U8G_MODE_BW ) {
    u8g.setColorIndex(1);                      
  }
  else if ( u8g.getMode() == U8G_MODE_HICOLOR ) {
    u8g.setHiColorByRGB(255,255,255);
  }



  // initialize the 3D I2C display
  // note you may have to change the address if you change the display type 
  // the most common are 0X3C and 0X3D (don't forget to solder correct address strap on display)

  display.begin(SSD1306_SWITCHCAPVCC, 0x3D);  // Address for the histograph
  // if you distribute your code, the adafruit license requires you show their splash screen
  // otherwise clear the video buffer memory then display
  display.display(); // This does the splash display
  //delay(2000);
  display.clearDisplay();
  display.display();// This clears the adafruit splashscreen

  
}  //end of setup

// This is the main program loop. Changing instructions will change the update rate.
void loop(void) { 

  Serial.begin(9600); 
 //Declare LED pins 
int Gled = 50 ; //green led on pin 50
int Rled = 52; //Red led on pin 52 
int Yled = 48; //yellow led on 48
int Bled = 46; //blue led on 46

pinMode(Gled,OUTPUT);//Pressure going up
pinMode(Rled,OUTPUT); //pressure going down 
pinMode(Yled,OUTPUT);//pressure staying the same
pinMode(Bled,OUTPUT);// big change

   //  show a cartesian style graph to plot values over time 
  //plotter will do 10 counts and stop incrementing 
  //X++ needs to be the history array 10 elements 
  //colors are hard coded
  //Call drawCGraph from barometer sketch but need the above setup also embedded
  
  delay(10); //sets sample delay 1000 counts =1 sec


 if (!bmp.begin()) {   //this starts the BMP85 or BMP180
  Serial.println("Could not find a valid BMP085 sensor, check wiring!");
  }
//Read the current pressure and add in altitude offset which is controlled by the trim pot 
 p = analogRead(A0); //Potientiometer  input voltage for altitude offset
 Offset = ((p/1024)*3.3)-1; //map pot to in of Hg offset     offset is in volts , guessing .43 volts is the ~ altitude offset (400ft) for my location        
 float Q=(bmp.readPressure()); //read the pressure from the barometer, Q is pascals 
  float  PS = Q*0.0002953;  //convert pressue to in"
 PS = PS + Offset;  //Correct the pressure reading for your altitude
//Serial.print("Pressure=");Serial.print(PS);Serial.print("offset=");Serial.println(Offset); // for trouble shooting pressure with serial monitor
 
  
dtostrf(PS,5,2,prs); //convert float to byte string, PS= pressure, output of the sketch is 5 characters long, prs is the ascii string 
//2 characters after the decimal point
 
              
 //Convert the pressure to an integer to scale it to the analog meter needle 
 psi = PS*100;
 mp = map(psi,2800,3100,0,100);  // mp is the needle position it needs to be an integer for scaling so multiply PS by 100                   
  // show needle and dial
  xx = mp;   // needed to control needle drawing                                  
  if (xx<45){                                
    xx=xx+135;
  }
  else {
    xx=xx-45;
  } 
  // Baro gauge picture loop
      delay(10);
  {
    u8g.firstPage(); 
    do {             
      gauge(xx);
    }
   while( u8g.nextPage() );  // writes the analog baro gauge to the 0x3C display
 Timer=Timer+1 ;
// Below serial print is for troubleshooting the loop timer elapsed time 
 //Serial.begin(9600); 
// Serial.print("Timer="); //check timer 
//  Serial.println(Timer);
//  delay(100);
//  Serial.end();
  //if time = 200 do print loop this gives about a minute 
 if (Timer>12000) {  // this should be an hour  for updates (120000 calculated to = 1 hr)
     Timer=0; // Reset the timer after 12000 counts/Hr then update the history display and LEDs
   
 // Below code is to control  LEDS based on recent pressure changes 
 
  old=newx; // Shift the pressure in the array
 
  newx=PS; //write the pressure to the new value  (#1 in the array) 
  //Trouble shooting serial monitor 
 // shift the array  
memmove(History+1,History,(sizeof(History)-1));//Slide data left to right  1 element and add the new pressure 
 History[0]=PS; //this adds the latest pressure 
 
// memmove(History,&History[1],sizeof(History));//Slide data down 1 element and add the new pressure - This is working now 1-5-22
 //History[9]=PS; //this addes the latest pressure 
 
 //calculate the sum of the array 
 double Historysum = 0;
  for (int Hi = 0; Hi < (sizeof(History) / sizeof(History[0])-1); ++Hi)
   
   
        Historysum += History[Hi];
        double Historyavg = Historysum/10 ; //compute the avg barometric reading ,this seems to be working now 
        

  



// Update the relative pressure array  array of the relative pressure changes 
for (int RHi =0 ;RHi <10 ;RHi ++) { RelHistory[RHi] = History[RHi]- Historyavg;
// check for out of bounds change to prevent graph error
if (RelHistory[RHi] > .2 ) {RelHistory[RHi] =.2;}
else
if (RelHistory[RHi] < -0.2 ) {RelHistory[RHi] =-0.2;}
else
 { }
}


//output  the relative history array to the serial monitor

 
//  draw graph function here 

 //This  loop calls the drawgraph function 
 display.fillRect(31,15,75,35,SSD1306_BLACK); //Fill rectangle with black  to clear graph 
 display.display(); //Plots the graph and the data 
 int z=0; // z is the index for printing the array
while  ( z < 10 ){DrawCGraph(display, z, RelHistory[z], 30, 50, 75, 30, 0,9, 1, -.2, .2, .1, 1, "In/Hg vs Hours ", Redraw4); 

  z++;
delay(10); 
}  

 
  //LED control section 

 
  double  Temp1; 
  //Pressure is constant or small turn on yellow LED 
   Temp1=abs(History[0])-abs(History[1]);
      if (abs(Temp1) <= 0.01) {   // Change is small so turn on yellow LED
    
  digitalWrite (Yled,HIGH);
 digitalWrite (Bled,LOW);
    }  //end of old==new if
  
  //Change is between .01 and .06 , turn off yellow and blue LEDs
 if (Temp1 > 0.01) { 
  digitalWrite (Yled,LOW);
 digitalWrite (Bled,LOW);
      //
 } 
  //pressure is rising or falling faster than 0.06 in of mercury
 float delta= (History[1]-History[0]); //subtract new reading from old reading 
 if (abs( delta) >= .06) {    
 digitalWrite (Bled, HIGH);
 digitalWrite (Yled,LOW);
 }  // end of change > .06 InHg 
   
    
   // Pressure is rising
          if (History[0] >= History[1] ){
          digitalWrite (Gled ,HIGH);
          digitalWrite  (Rled, LOW);
                  //end of old< new if
                }
      //Pressure is falling               
            if (History[0] <= History[1] ) {
            digitalWrite (Gled ,LOW);
             digitalWrite  (Rled, HIGH);
                         } //end of old> new if 
               
   
  }  //end of timer loop 
  }  // end of picture loop
} //end of main loop 
 
//Draw Cartesian Histogram Graph function call
 void DrawCGraph(Adafruit_SSD1306 &d, double x, double y, double gx, double gy, double w, double h, double xlo, double xhi, double xinc, double ylo, double yhi, double yinc, double dig, String title, boolean &Redraw) {
// dig = precision for the X and y axis legend ,
  double i;
  double temp;
  //int rot, newrot; //Seems not needed

  if (Redraw == true) {
    Redraw = false;
    d.fillRect(0, 0,  127 , 16, SSD1306_BLACK); //Fill rectangle with black 
    d.setTextColor(SSD1306_WHITE, SSD1306_BLACK); //White letters on black background
    d.setTextSize(1);
    d.setCursor(2, 4);
    d.println(title);
    ox = (x - xlo) * ( w) / (xhi - xlo) + gx;
    oy = (y - ylo) * (gy - h - gy) / (yhi - ylo) + gy;
    // draw y scale
    d.setTextSize(1);
    d.setTextColor(SSD1306_WHITE, SSD1306_BLACK);
    for ( i = ylo; i <= yhi; i += yinc) {
      // compute the transform
      // note my transform funcition is the same as the map function, except the map uses long and we need doubles
      temp =  (i - ylo) * (gy - h - gy) / (yhi - ylo) + gy;
      if (i == ylo) { /* test to draw horizontal axis */
       d.drawFastHLine(gx - 3, temp, w + 3, SSD1306_WHITE);//this  draws the x axis line 
      }
      else {
        d.drawFastHLine(gx - 3, temp, 3, SSD1306_WHITE); // this draws the Y tic marks  
      }
      d.setCursor(gx - 27, temp - 3);
      d.println(i, dig); //this prints the digits for the Y axis 
   }
    // draw x scale
    for (i = xlo; i <= xhi; i += xinc) {
      // compute the transform
      d.setTextSize(1);
      d.setTextColor(SSD1306_WHITE, SSD1306_BLACK);
      temp =  (i - xlo) * ( w) / (xhi - xlo) + gx;
      if (i == 0) {
        d.drawFastVLine(temp, gy - h, h + 3, SSD1306_WHITE);  // this draws the verticle axis line 
      }
      else {
        d.drawFastVLine(temp, gy, 3, SSD1306_WHITE);  // this draws the x axis tic marks 
      }
      d.setCursor(temp, gy + 6);
      d.println(i, 0); //Print the Horizontal  axis digits , note the 0 changes the X axis digits precision to 0 
    }
  } //end of redraw true loop

  // graph drawn now plot the data
  // the entire plotting code are these next few lines...

// below if statement added to reset the ox,oy orgin , its  working but needs tweeking still
if (x == 0) {
  ox = (x - xlo) * ( w) / (xhi - xlo) + gx;
    oy = (y - ylo) * (gy - h - gy) / (yhi - ylo) + gy;
}
else {
  x =  (x - xlo) * ( w) / (xhi - xlo) + gx;
  y =  (y - ylo) * (gy - h - gy) / (yhi - ylo) + gy;
  d.drawLine(ox, oy, x, y, SSD1306_WHITE); // this is drawing the graph line 
  //d.drawLine(ox, oy - 1, x, y - 1, SSD1306_WHITE);
  ox = x;
  oy = y;
 // Serial.print("ox");
 // Serial.println(ox);
 // Serial.print("oy");
 // Serial.println(oy);
  // up until now print sends data to a video buffer NOT the screen
  // this call sends the data to the screen
  
 
  d.display(); //Plots the graph and the data 
}
}

//End of program 
 
 
