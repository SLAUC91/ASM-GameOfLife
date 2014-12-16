/* The Game of Life w\ Mouse
*
* Authors: SL
*
* Version 2.00
*/
#include <stdio.h>
#include <stdlib.h>
#include <pc.h>
#include <dpmi.h>
#include <go32.h>
#include <dos.h>
#include <string.h>

#include <sys/farptr.h>

#define BYTE unsigned char
#define CHECK_BIT(var,pos) ((var) & (1<<(pos)))

// Function used to store data used in graphical mode. Most of this structure is taken from the file "VBETEST.C".
typedef struct {
  unsigned ModeAttributes;
  unsigned granularity,startseg,farfunc;
  short bscanline;
  short XResolution;
  short YResolution;
  short charpixels;
  unsigned bogus1,bogus2,bogus3,bogus4;
  unsigned PhysBasePtr;
  char bogus[228];
} ModeInfoBlock;

unsigned int ADDR;

int width,height;
int location[34][34] = {{0}};       // add a buffer of empty locations around the grid to reduce special cases in check function
int duplicate[34][34] = {{0}};      // temporary array to store changes
int count = 0;
char array[12];
int x = 800;
int y = 600;

// Function used to get data used in graphical mode. Most of this function is taken from the file "VBETEST.C".
ModeInfoBlock *get_mode_info(int mode)
{
        static ModeInfoBlock info;
        __dpmi_regs r;

        /* Use the transfer buffer to store the results of VBE call */
        r.x.ax = 0x4F01;
        r.x.cx = mode;
        r.x.es = __tb / 16;
        r.x.di = 0;
        __dpmi_int(0x10, &r);
        if(r.h.ah) return 0;
        dosmemget(__tb, sizeof(ModeInfoBlock), &info);
        return &info;
}

// Function used to initiate the graphics. Most of this function is taken from the file "VBETEST.C".
void init_graphics(void)
{
        __dpmi_meminfo info;
        __dpmi_regs reg;
        ModeInfoBlock *mb;

        mb=get_mode_info(0x105);        /*XGA 1024x768 mode*/

        width=mb->XResolution;
        height=mb->YResolution;
        info.size=width*height;
        info.address=mb->PhysBasePtr;
        if(__dpmi_physical_address_mapping(&info) == -1) {
          printf("Physical mapping of address 0x%x failed!\n",mb->PhysBasePtr);
          exit(2);
        }
        ADDR=info.address;              /* Updated by above call */

        reg.x.ax=0x4f02;
        reg.x.bx=0x4105;
        __dpmi_int(0x10,&reg);          /* set the mode */
}


void drawbox(int c1, int c2, int theight, int bheight, int gridsize, int gridcolor, int my_video_ds)
/*c1 = left corner (x), c2 = right corner(x)
*theight = top height, bheight = bottom height
*gridsize = pixel by pixel grid size *NOTE* if gridsize = 0 no grid
*gridcolor = color of lines in grid
*/
{
   int i;
   int h;
   int j;
   int po;
   int jcount;
   int jcount1;
   int cwidth;
   int cwidth2;

   /*conversion*/
   jcount = height - theight;
   jcount1 = height - bheight;
   cwidth = width - c1;
   cwidth2 = width - c2;


   /*draw outside lines*/
   /*draw top and bottom lines*/
   for(i=(width-cwidth);i<(width-cwidth2);i++) {
     _farpokeb(my_video_ds,ADDR+i+width*(height-jcount),gridcolor);
     _farpokeb(my_video_ds,ADDR+i+width*(height-jcount1),gridcolor);
   }
   /*draws vertical line left & right lines*/
   for (h = theight; h <= bheight; h++){
     _farpokeb(my_video_ds,ADDR + (c1+(width*h)),gridcolor);
     _farpokeb(my_video_ds,ADDR + (c2+(width*h)),gridcolor);
   }

   if (gridsize != 0) {
   /*draw all inside lines*/
   /*draws horizontal lines*/
       for (j = theight + gridsize; j < bheight; j = j + gridsize){
          jcount = height - j;
          for(i=(width-cwidth);i<(width-cwidth2);i++) {
              _farpokeb(my_video_ds,ADDR+i+width*(height-jcount),gridcolor);
          }
       }

   /*draws vertical lines*/
       for (po = c1 + gridsize; po < c2; po= po + gridsize){
           for (h = theight; h <= bheight; h++){
               _farpokeb(my_video_ds,ADDR + (po+(width*h)),gridcolor);
           }
       }
   }
}

/*used to draw a small filled box. 18x18 pixels of indicated color*/
void fillbox(int h, int v, int color, int my_video_ds){
       int j;
       int i;
       int jcount;
       
       for (j = v; j <= v+18; j++){
          jcount = height - j;
          for(i = h; i <= h + 18; i++) {
              _farpokeb(my_video_ds,ADDR+i+width*(height-jcount),color);
          }
       }
}

// Function used to exit back to text mode
void close_graphics(void)
{
        __dpmi_regs reg;
        reg.x.ax = 0x0003;              // Text mode
        __dpmi_int(0x10,&reg);
}


// Function used to count the organisms around the input organism and make adjustments accordingly
void check(int h, int v, int my_video_ds)
{

   int num = 0;

   // Count the number of neighbors around input. Had to do this the long way.
   if (location[h-1][v-1] == 1) {
       num++;
   }
   if (location[h-1][v] == 1) {
       num++;
   }
   if (location[h-1][v+1] == 1) {
       num++;
   }
   if (location[h][v-1] == 1) {
       num++;
   }
   // skip [h][v]
   if (location[h][v+1] == 1) {
       num++;
   }
   if (location[h+1][v-1] == 1) {
       num++;
   }
   if (location[h+1][v] == 1) {
       num++;
   }
   if (location[h+1][v+1] == 1) {
       num++;
   }

   // Check for birth of new organism
   if (location[h][v] == 0) {
      if (num == 3) {
         fillbox(41 + 20*h, 41 + 20*v, 15, my_video_ds); // with white
	 duplicate[h][v] = 1;
      } 
   }	
   // Check for overpopulation and underpopulation of organisms
   else if (num < 2 || num > 3) {
      fillbox(41 + 20*h, 41 + 20*v , 0, my_video_ds);    // with black
      duplicate[h][v] = 0;
   }
   else {
      duplicate[h][v] = location[h][v];
   }
}

void write(int h, int v, char A, int mytext, int my_video_ds)
/*Used to draw a single character (x, y, character, text and video segments*/
{
   char char_row;
   int i;
   int j;
   for (i = 0; i < 8 ; i++){
       char_row = _farpeekb(mytext, (A * 8)+i);

        for ( j = 0; j < 8; j++){
                if (char_row & 0x80)
                        _farpokeb(my_video_ds, ((i+v)*width) + j + h, 15);
                char_row <<= 1;
        }
   }
}

void writestring(int h, int v, char *buffer, int mytext, int my_video_ds)
/*Used to print a string to screen (x, y, buffer memory, text and video segments*/
{
      int len;
      int i;
      len = strlen(buffer);
      for (i = 0; i < len; i++){
           write (h + (i*8), v , buffer[i], mytext, my_video_ds);
      }
}
//Majority of the interrupt handler below is for ps2mouse
_go32_dpmi_seginfo old_handler,new_mousehandler; 
/*These structures hold the selector/offset pairs for the far pointers of the old
and new handlers. We need the old_handler for clean-up when we exit the program. */

/*******************************************************************************/
void NewInt74(void)  
/*This is the actual ISR called asynchronously whenever we
move or click the mouse. ISRs should be as small as 
possible. This one is pretty tiny, except that DPMI adds
a lot of invisible stuff (about 3000 clock cycles,
evidently! Also the wrappers.) A bare-bones asm ISR would
probably run in 100 clocks, much of this due to wait 
states in the mouse controller hardware access.*/
{
        if (count > 11) { count = 0;}
	array[count] = inportb(0x60); /*PS/2 data port address */
        count++;

	outportb(0xA0, 0x20);   /*This resets the S PIC.*/
    outportb(0x20, 0x20);   /*Reset M PIC */
}

void LOCKint74() { } 
/*This is a dummy function declaration. Its purpose is
to give us a cheap and nasty way to obtain the length
of the ISR function so we can lock it-see below. This
depends on gcc not rearranging the order of functions
in the compilation. This is not adequate for quality
code-we really need assembler to do this properly. 
This might break if certain optimizations are turned
on. In asm this is a trivial problem.*/

/***************************************************************************/
void mouse_init(void)     
/*Note that we are doing NO device initialization. 
We are assuming that this was done suitably when the system 
was booted and the standard mouse driver was installed. THis is very poor
programming practice!*/
{

 /*  _go32_dpmi_lock_data(mousequeue,200);   
 
	//Not USED

   _go32_dpmi_lock_data(&mousequeue,sizeof(mousequeue));*/

   _go32_dpmi_lock_code(NewInt74,(unsigned long)(LOCKint74-NewInt74)); 
   /*Here, we ensure that the handler doesn't get paged out from 
	under our noses and cause a page fault. */

   _go32_dpmi_get_protected_mode_interrupt_vector(0x74, &old_handler);
   /*This line reads the selector and offset of the previous handler for
	INT 74h (the PS/2 mouse interrupt) into the structure old_handler for use later.  */

   /*The next two lines give us the far pointer for our own ISR*/
   new_mousehandler.pm_selector=_go32_my_cs();  
   /* Note that _go32_my_cs() returns the selector for the program's code.*/

   new_mousehandler.pm_offset=(unsigned long)NewInt74;

   _go32_dpmi_allocate_iret_wrapper(&new_mousehandler); 
   /*See comment in the ISR.
	This "wrapper" is basically a series of PUSH instructions
	to save the machine state when the ISR is called-it is
	a part of the ISR called first before the ISR shown above
	is executed. This is rather inefficient because we are
	almost certainly saving lots of stuff that we wouldn't
	clobber anyway. 
	The wrapper also includes code to terminate the ISR:
	POP the original machine state and return from interrupt
	(that is, the asm IRET instruction)*/ 

   _go32_dpmi_set_protected_mode_interrupt_vector(0x74, &new_mousehandler); 
   /*This puts our ISR far pointer into the interrupt table vector #12. This
	is a protected mode interrupt table maintained by CWSDPMI or whatever DPMI host is in use.*/

}

/***************************************************************************/
void mouse_delete(void)
/*Once we have finished with it, we can 
return the vector to its previous state by passing
the old handler's address we saved before. */
{
   _go32_dpmi_set_protected_mode_interrupt_vector(0x74,&old_handler);
   _go32_dpmi_free_iret_wrapper(&new_mousehandler);   
}

/***************************************************************************/

void drawEverything(int my_video_ds, int mytext) {
   int n = 0;           // counter

   __dpmi_regs reg;
   reg.x.ax=0x4f02;
   reg.x.bx=0x4105;
   __dpmi_int(0x10,&reg);          /* set the mode */

    // Print all the text out to the screen
   char string[] = "GAME OF LIFE SIMULATION";
   writestring(60, 30, string, mytext, my_video_ds);
   char string1[] = "Created by:  Michael Benediktson, Slaven Lauc";
   writestring(70, 40, string1, mytext, my_video_ds);
   char string2[] = "COMMANDS";
   writestring(800, 60, string2, mytext, my_video_ds);
   char string3[] = "START";
   writestring(800, 82, string3, mytext, my_video_ds);
   char string4[] = "EXIT";
   writestring(800, 116, string4, mytext, my_video_ds);
   char string5[] = "To fill the grid, simpily move the cursor over the desired area and left click to populate that square";
   writestring(60, 730, string5, mytext, my_video_ds);

   drawbox(60, 700, 60, 700, 20, 15, my_video_ds);                      // setup the grid

   // Loop for drawing the border
   while (n < 20) {
        drawbox(20-n, 1004+n, 20-n, 748+n, 0, 5, my_video_ds);          // in magenta
        n++;
   }

   drawbox(797, 865, 57, 69, 0, 15, my_video_ds);                       // draw a white box around "COMMANDS"
   drawbox(797, 865, 79, 103, 0, 15, my_video_ds);		        // draw a white box around "Start"
   drawbox(797, 865, 113, 137, 0, 15, my_video_ds);                     // draw a white box aroung "Stop"

   int i = 0;
   int j = 0;
   for (i = 1; i < 33; i++) {
        for (j = 1; j < 33; j++) {
            if (location[i][j]) {
                fillbox(41+i*20,41+j*20,15,my_video_ds);
            }
        }
   }   
}

void drawmouse(int h, int v, int color, int my_video_ds){
        //draws you an arrow
        int i = 0;
        for(i=0; i<7; i++){
                drawbox(h-i, h+(i+1), v+i, v+(i+1), 0, color, my_video_ds);
        }
}


int main(void)
{   
   int go = 0;
   int go2 = 0;
   char marker = 0;
   int h = 1;           // horizontal offset
   int v = 1;           // vertical offset

   mouse_init();        // NEW
   
   // initial setup of resolution and picture
   int my_video_ds;
   int mytext;
   init_graphics();
   my_video_ds = __dpmi_allocate_ldt_descriptors(1);
   __dpmi_set_segment_base_address(my_video_ds,ADDR);
   ADDR = 0;       /* basis now zero from selector */
   __dpmi_set_segment_limit(my_video_ds,(width*height)|0xfff);
   mytext = __dpmi_allocate_ldt_descriptors(1);
   __dpmi_set_segment_base_address(mytext, 0xffa6e);
   __dpmi_set_segment_limit(mytext, 8*128);

   drawEverything(my_video_ds, mytext);

   drawmouse(x, y, 6, my_video_ds);
   // get input from user
   int k = 0;
   register int pak1;
   while ( 1 ) {
   	// change the mouse memory location
   	k += 3;
	if (k >= 12) {
		k = 0;
	}
	// move the mouse in the x direction
	x += array[k+1];
        if (x < 7){
            x = 7;
        }
        else if (x > 1016){
            x = 1016;
        }
	// move the mouse in the y direction
	y -= array[k+2];
        if (y < 0) {
                y = 0;}
        else if (y > 760) {
                y =760;
	}
	drawmouse(x,y,6, my_video_ds);

        pak1 = array[k];
        array[k] = 0;
        array[k+1] = 0;
        array[k+2] = 0;
        // if ENTER fill/empty the box and mark the array
        if (CHECK_BIT(pak1,0)) {
              int xtemp = 0;
	      int ytemp = 0;
	   //quit
	   if (x >= 797 && x <= 865 && y >= 113 && y <= 137) {
                break;
	   }
           //set marker for stop/start
           else if (x >= 797 && x <= 865 && y >= 79 && y<=103) { 
                if (marker == 0) {
                        marker = 1;
                        go = 0;
                }
                else {
                        marker = 0;
                }
           }
	   //fill box if inside grid
	   else if (x >= 61 && x <= 699 &&  y >= 61 && y<= 699) {
		xtemp = x -21;
		ytemp = y -21;
		xtemp = (xtemp -1 - (xtemp % 20))/20;  // this will give you x = 1 or 2 or 3 (the horizontal location on the grid)
		ytemp = (ytemp -1 - (ytemp % 20))/20;
		if (location[xtemp][ytemp] == 0) {
			fillbox(41+xtemp*20,41+ytemp*20,15, my_video_ds);
			location[xtemp][ytemp] = 1;
		}
		else {
			fillbox(41+xtemp*20,41+ytemp*20,0, my_video_ds);
			location[xtemp][ytemp] = 0;
		}
	   }

        }
	//if marker set perform operation
        if (marker == 1) {
                if (go == 30) {
                   int i = 0;
                   int j = 0;
                   for (i = 1; i < 33; i++) {
                        for (j = 1; j < 33; j++) {
                             check(i,j, my_video_ds);
                        }
                   }
                   // copy the duplicate array into the real array
                   // duplicate array is needed to prevent "realtime" changes
                   for (i = 1; i < 33; i++) {
                        for (j = 1; j < 33; j++) {
                             location[i][j] = duplicate[i][j];
                        }
                   }
                   go = 0;  
                }
                if (go2 == 500) {
                        go2 = 0;
                        drawEverything(my_video_ds, mytext);
                }
                delay(10);                       // pause to let the user see any changes
                go++;
                go2++;
        }
   }
   mouse_delete();
   close_graphics();   // exit back to text mode
   return 0;           // exit the program
}
