/**
 * PIC Tennis
 * 
 * This software was put up quickly for a demonstration in 2016.
 * It features logic to implement "Tennis for Two" using a simple PIC18F258
 *  and 2 R2R DACs for XY display onto an oscilloscope
 * 
 * Author: Javier Albinarrate
 * 
 */

#pragma config OSC = RCIO       // Oscillator Selection bits (RC oscillator w/ OSC2 configured as RA6)
#pragma config LVP = OFF        // Low-Voltage ICSP Enable bit (Low-Voltage ICSP disabled)
#pragma config WDT = OFF        // Watchdog Timer Enable bit (WDT enabled)



#include <p18cxxx.h>
#include <p18f258.h>
#include "typedefs.h" 
#include "sintable.h" 
#include <math.h>		//gives rand() function
#include <stdlib.h>		//gives rand() function

/* GAME CONSTANTS */

// Gravity
#define g 0.8

#define force 1.4

// TimeStep
#define ts 0.020

// Net X position and height
#define Net_X        127
#define Net_H        61
#define Net_Repeat   2

// Trail length
#define Ball_Trail   20
#define Ball_MaxHits 10
#define Ball_Repeat  5
#define Ball_Wait    1000
#define Ball_WaitShort 100
#define Ball_H       110
#define Ball_L       25
#define Ball_R       230

#define Angle_Delta  48
#define Angle_Max    127
#define Angle_Min    16

#define TIMER_Mode_Auto    65000
#define TIMER_Mode_Players 5000

#define L_AUTO_X     Net_X - 20
#define R_AUTO_X     Net_X + 20
#define L_AUTO_Y     50
#define R_AUTO_Y     55

/* I/O CONFIGS:
 * RA  AN  I/O A/D Func
 * RA0 AN0 O   D   Relay
 * RA1 AN1 I   A   ADC R
 * RA2 AN2 I   D   BTN L
 * RA3 AN3 I   A   ADC L
 * RA4 --- I   D   BTN R
 * RA5 AN4 I   D   MODE0
 * RA6 --- I   D   MODE1
 */

// Vertical DAC
#define V_Read      PORTB
#define V_Write     LATB
#define V_Dir       TRISB
// Horizontal DAC
#define H_Read      PORTC
#define H_Write     LATC
#define H_Dir       TRISC

// Player LEFT (0)
#define L_Btn      PORTAbits.RA2
#define L_Btn_Dir  TRISAbits.TRISA2
#define L_ADC      3 // RA3/AN3
#define L_ADC_Dir  TRISAbits.TRISA3

// Player RIGHT (1)
#define R_Btn      PORTAbits.RA4
#define R_Btn_Dir  TRISAbits.TRISA4
#define R_ADC      1 // RA1/AN1
#define R_ADC_Dir  TRISAbits.TRISA1

// Relay pin (used to turn on or off the oscilloscope)
#define RELAY_Pin  LATAbits.LATA0
#define RELAY_Dir  TRISAbits.TRISA0

// Mode pins (inputs)
#define MODE_Read1 PORTAbits.RA5
#define MODE_Dir1  TRISAbits.TRISA5
#define MODE_Read2 PORTAbits.RA6
#define MODE_Dir2  TRISAbits.TRISA6

// ADC
#define ADC_Busy    ADCON0bits.NOT_DONE
#define ADC_Result  ADRES
#define ADC_CfgIo_Reg ADCON1
#define ADC_CfgIo_Val 0b00000100 // This affects Port A Digital vs Analog settings

/* Misc constants */
#define IN         1
#define OUT        0
#define IN_ALL     0xff
#define OUT_ALL    0x00

# pragma udata 

void ADC_start(unsigned char nChannel);
float getCustomSin(unsigned char angle);
float getCustomCos(unsigned char angle);
void XY_drawLineDelta(unsigned char xs, unsigned char ys, signed char dx, signed char dy);
void XY_drawLine(unsigned char xs, unsigned char ys, unsigned char xe, unsigned char ye);
void XY_drawLineX(signed char delta);
void XY_drawLineY(signed char delta);
void XY_drawGround(void);
void DEBUG_line_H(unsigned char delta);
void DEBUG_line_V(unsigned char delta);
void DEBUG_drawChar(unsigned char xin, unsigned char yin, unsigned char digit);
void DEBUG_drawDigit(unsigned char xin, unsigned char yin, unsigned char digit);


#pragma udata


// Position
float xOld, yOld, xNew, yNew;  // Theoretical ball positions
unsigned char xp = 0;          // Actual ball position
unsigned char yp = 0;          //
unsigned char x  = 0;          // Oscilloscope beam position
unsigned char y  = 0;          //

// Velocity
float VxOld, VyOld, VxNew, VyNew;

// Trail
unsigned char x_Trail[Ball_Trail];
unsigned char y_Trail[Ball_Trail];

// Player control
unsigned char L_used  = 0;
unsigned char L_angle = 0;
unsigned char R_used  = 0;
unsigned char R_angle = 0;

// Game control
unsigned char nDebug  = 0;
unsigned char nMode   = 255;
unsigned char nMode_Auto_L    = 0;
unsigned char nMode_Auto_R    = 0;
unsigned char nRule_SingleHit = 0;
unsigned char nRule_DeadBall  = 0;
unsigned char nSide      = 0;
unsigned char nDeadBall  = 0;
unsigned char nBallCount = 0;
unsigned char nBallHits  = Ball_MaxHits + 1;

// Timers, delays and big counters
unsigned int iDelayNewBall = Ball_Wait;
unsigned int iTimerIdle    = TIMER_Mode_Auto;

// Dummy variables:
unsigned char k = 0;
unsigned char m = 0;
unsigned char j = 0;
unsigned int  iVal = 0;

// ADC related
unsigned char ADC_CurrentPlayer = 0;

#pragma code

void main (void)
{
    // Inputs
    MODE_Dir1  = IN;
    MODE_Dir2  = IN;
    L_Btn_Dir  = IN;
    L_ADC_Dir  = IN;
    R_Btn_Dir  = IN;
    R_ADC_Dir  = IN;

    // Outputs
    RELAY_Dir  = OUT;
    RELAY_Pin  = 1;

	// DAC Outputs
	V_Dir      = OUT_ALL;
	H_Dir      = OUT_ALL;
	V_Write    = 0;
	H_Write    = 0;
    
    // ADC I/O
    ADC_CfgIo_Reg = ADC_CfgIo_Val;
    /*
    ADCON1bits.PCFG0 = 0;
    ADCON1bits.PCFG1 = 0;
    ADCON1bits.PCFG2 = 1;
    ADCON1bits.PCFG3 = 0;
    */
    
    // Initialize RAND seeding it with L ADC
    ADC_start(L_ADC);
	// If ADC conversion has finished
    while (ADC_Busy) {
        // We need to wait for the conversion
    }
    // Seed rand, used for autoplayers
    srand(ADC_Result);

    // Game init
	nSide = 0;
	xOld  = 0;
	yOld  = 0;
    VxOld = 0;
	VyOld = 0;
	xNew  = 0;
	yNew  = 0;
    VxNew = 0;
	VyNew = 0;

    // main loop
	for (;;) {
        // Handle mode
        // Note: I have them inverted in the switch
        m = (unsigned char) (MODE_Read1 * 2) + MODE_Read2;
        if (nMode != m){
            nMode = m;
            nBallHits = Ball_MaxHits + 1;

            // Mode: Auto
            if (nMode == 0){
                nRule_SingleHit = 0;
                nRule_DeadBall  = 0;
                nMode_Auto_L    = 1;
                nMode_Auto_R    = 1;
                iTimerIdle      = TIMER_Mode_Auto;
            }
            // Mode: 2P Original (No rules)
            else if (nMode == 1){
                nRule_SingleHit = 0;
                nRule_DeadBall  = 0;
                nMode_Auto_L    = 0;
                nMode_Auto_R    = 0;
                iTimerIdle      = TIMER_Mode_Players;
            }
            // Mode: 2P with Rules
            else if (nMode == 2){
                nRule_SingleHit = 1;
                nRule_DeadBall  = 1;
                nMode_Auto_L    = 0;
                nMode_Auto_R    = 0;
                iTimerIdle      = TIMER_Mode_Players;
            }
            // Mode: 1P with Rules
            else if (nMode == 3){
                nRule_SingleHit = 1;
                nRule_DeadBall  = 1;
                nMode_Auto_L    = 0;
                nMode_Auto_R    = 1;
                iTimerIdle      = TIMER_Mode_Players;
            }            
        }
        
        // Turn on/off Debug mode
        if (nMode == 1 
            && iDelayNewBall > 0 
            && nSide   == 0 
            && L_angle == 0 
            && R_angle == 0 
            && L_Btn   == 0 
            && R_Btn   == 0){
            nDebug = nDebug ? 0 : 1;
            iDelayNewBall = 0;
        }
        
        // Handle timers
        if (L_Btn == 0 || R_Btn == 0){
            RELAY_Pin = 1;
            // Reset idle timer according to mode
            if (nMode == 0){
                iTimerIdle = TIMER_Mode_Auto;
            }
            else{
                iTimerIdle = TIMER_Mode_Players;
            }
        }
        else if (iTimerIdle > 0){
            iTimerIdle--;
        }
        else{
            if (nMode == 0){
                // Mode: Auto -> Turn off oscope
               RELAY_Pin = 0;
            }
            else{
               // Mode: Players  -> switch to auto
               nMode = 0;
               iTimerIdle = TIMER_Mode_Auto;
            }
        }
        
        // Changing nSide
		if (nSide != (xOld >= Net_X)) {
			nSide = (xOld >= Net_X);

			if (nSide){
                R_used = 0;
			}
            else{
                L_used = 0;
            }
		}

		// IF ball has run out of energy, make a new ball!
		if ( nBallHits > Ball_MaxHits ) {
            nBallCount++;
			nBallHits = 0;
			nDeadBall = 0;
			R_used    = 0;
			L_used    = 0;
            VxOld     = 0;
            VyOld     = 0;

			iDelayNewBall  = Ball_Wait;

            yOld = (float) Ball_H;
			if (nSide == 0) {
                nSide  = 1;
				xOld   = (float) Ball_R;
				L_used = 1;
                if (nMode_Auto_R){
                    // We don't want to wait too much
                    iDelayNewBall  = Ball_WaitShort;
                }
			}
			else {
                nSide   = 0;
				xOld   = (float) Ball_L;
				R_used = 1;
                if (nMode_Auto_L){
                    // We don't want to wait too much
                    iDelayNewBall  = Ball_WaitShort;
                }
			}

            // Fill in history
			m = 0;
			while (m < Ball_Trail) {
				x_Trail[m] = xOld;
				y_Trail[m] = yOld;
				m++;
			}
		}

		// If ADC conversion has finished
        if (ADC_Busy == 0) {
            // Read ADC value (10 bits right aligned 1111 1111 1100 0000)
            // We only care about the 8 most significant bits
			iVal = ADC_Result >> 8;
            // 128 values allowed, hence 7 bits are actually used
            iVal = iVal >> 1;

			// We are using *ONE* ADC, but sequentially multiplexing it to sample
			// the two different input lines.	
			if (ADC_CurrentPlayer == 0) {
                
				L_angle =  iVal;
                // Start next conversion
                ADC_CurrentPlayer = 1;
                ADC_start(R_ADC);
            }
			else {
				R_angle =  iVal;
                // Start next conversion
                ADC_CurrentPlayer = 0;
                ADC_start(L_ADC);
            }
		}
        
        /* DEBUG!!!!! */
        //L_angle = ((nBallCount & 0x07) << 2) + 31;
        //R_angle = ((nBallCount & 0x07) << 2) + 31;
        //R_angle = L_angle;

		if (iDelayNewBall > 0) {
			iDelayNewBall--;

   			if ( (nSide == 0 && L_Btn == 0) || (nSide == 1 && R_Btn == 0)){
				iDelayNewBall = 0;
            }

			m = 0;
			while (m < Ball_Repeat) {
				V_Write = yp;
				H_Write = xp;
				m++;
			}

			VxNew = VxOld;
			VyNew = VyOld;
			xNew  = xOld;
			yNew  = yOld;
		}
		else {
            // Physics calculations
            // x' = x + v*t + at*t/2
            // v' = v + a*t
            //
            // Horizontal (X) axis: No acceleration; a = 0.
            // Vertical   (Y) axis: a = -g
			xNew  = xOld + VxOld;
			yNew  = yOld + VyOld - 0.5 * g * ts * ts;

			VyNew = VyOld - g * ts;
			VxNew = VxOld;

			/* Bounce at walls */
            // Left Wall
			if (xNew < 0) {
				VxNew   *= -0.25;
				VyNew   *= 0.75;
				xNew     = (float) 0;
				nDeadBall = nRule_DeadBall;
				//nBallHits  = 100;
			}
            // Right Wall
			if (xNew > 255) {
				VxNew   *= -0.25;
                VyNew   *= 0.75;
				xNew     = (float) 255;
				nDeadBall = nRule_DeadBall;
				//nBallHits  = 100;
                
			}
            // Floor
			if (yNew <= 0) {
				yNew = (float) 0;
				if (VyNew * VyNew < 10) {
					nBallHits++;
				}
				if (VyNew < 0) {
					VyNew *= -0.75;
				}
			}
            // Ceiling
			if (yNew >= 255) {
				yNew = (float) 255;
				VyNew *= -0.75;
			}

            /* Check net */
			if (nSide) {
                // RIGHT SIDE
				if (xNew < Net_X) {
					if (yNew <= Net_H) {
						// Bounce off of net
						VxNew   *= -0.5;
						VyNew   *= 0.5;
						xNew     = (float) Net_X + 1;
						nDeadBall = nRule_DeadBall;
					}
				}
			}
            else {
                // LEFT SIDE
				if (xNew > Net_X) {
					if (yNew <= Net_H) {
						// Bounce off of net
						VxNew   *= -0.5;
						VyNew   *= 0.5;
						xNew     = (float) Net_X - 1;
						nDeadBall = nRule_DeadBall;
					}
				}
			}

            /* Button presses */
            // LEFT
			if (nSide == 0 && xOld < Net_X - 7) {
				if (L_used == 0 && nDeadBall == 0) {
                    if (nMode > 0 && L_Btn == 0) {
						VxNew   =     force * getCustomCos(L_angle);
						VyNew   = g + force * getCustomSin(L_angle);
						L_used  = nRule_SingleHit;
						nBallHits = 0;
                    }
                    else if(nMode_Auto_L == 1){
                        if (xOld < 20 || (yOld < L_AUTO_Y && xOld < L_AUTO_X)){
                            iVal = rand();
                            j = (unsigned char) iVal >> 8;
                            
                            if (j < 10){
                                // we have 4% chances that the automata will fuck it up totally
                                L_used = 1;
                                nDeadBall = 1;
                            }
                            else if (j > 50){
                                // We have 30% (50 / 255) chance that automata will not reply in this iteration
                                // but we are not making it deadball, it should let the ball continue
                                // and hit it the next time possibly
                                
                                j = ((unsigned char) (iVal & 31) + Angle_Delta + Angle_Min);

                                VxNew   =     force * getCustomCos(j);
                                VyNew   = g + force * getCustomSin(j);
                                L_used  = nRule_SingleHit;
                                nBallHits = 0;
                            }
                        }
                    }
                }
			}
			// RIGHT
			else if (nSide == 1 && xOld > Net_X + 7) {
                if (R_used == 0 && nDeadBall == 0) {
                    if (nMode > 0 && R_Btn == 0) {
						VxNew   =   - force * getCustomCos(R_angle);
						VyNew   = g + force * getCustomSin(R_angle);
						R_used  = nRule_SingleHit;
						nBallHits = 0;
                    }
                    else if (nMode_Auto_R == 1){
                        if (xOld > 235 || (yOld < R_AUTO_Y && xOld > R_AUTO_X)){
                            iVal = rand();
                            j = (unsigned char) iVal >> 8;
                            
                            if (j < 10){
                                // we have 4% chances that the automata will fuck it up totally
                                R_used = 1;
                                nDeadBall = 1;
                            }
                            else if (j > 50){
                                // We have 30% (50 / 255) chance that automata will not reply in this iteration
                                // but we are not making it deadball, it should let the ball continue
                                // and hit it the next time possibly

                                j = ((unsigned char) (iVal & 31) + Angle_Delta + Angle_Min);

                                VxNew   =   - force * getCustomCos(j);
                                VyNew   = g + force * getCustomSin(j);
                                R_used  = nRule_SingleHit;
                                nBallHits = 0;
                            }
                        }
                    }
                }
			}
		}

		//Figure out which point we're going to draw.
		xp =  (int) floor(xNew);
		yp =  (int) floor(yNew);

        // Draw ball trail
		m = 0;
		while (m < Ball_Trail) {
			k = 0;
            y = y_Trail[m];
            x = x_Trail[m];
			//while (k < (4 * m * m)) {
            while (k < 10) {
				V_Write = y;
				H_Write = x;
				k++;
			}
			m++;
		}
        
        // Draw the ball
		m = 0;
		while (m < Ball_Repeat){
            V_Write = yp;
            H_Write = xp;
			m++;
		}

        
        // Shift the values in the stack
		m = 0;
		while (m < (Ball_Trail - 1)){
			x_Trail[m] = x_Trail[m+1];
			y_Trail[m] = y_Trail[m+1];
			m++;
		}
        // Push the current point to the stack
		x_Trail[(Ball_Trail - 1)] = xp;
		y_Trail[(Ball_Trail - 1)] = yp;

        
		//Draw Ground and Net
		for (k = Net_Repeat; k > 0; k--) {
           XY_drawGround();
		}
        
        //DEBUG LINES
        if (nDebug){
            x = 0;
            y = 235;

            DEBUG_drawDigit(x, y, nMode);
            x += 6;
            DEBUG_drawDigit(x, y, nMode_Auto_L);
            x += 6;
            DEBUG_drawDigit(x, y, nMode_Auto_R);
            x += 8;
            DEBUG_drawDigit(x, y, nRule_SingleHit);
            x += 6;
            DEBUG_drawDigit(x, y, nRule_DeadBall);
            x += 6;
            DEBUG_drawChar(x, y, nBallCount);
            x += 6;
            DEBUG_drawDigit(x, y, nSide);
            x += 6;
            DEBUG_drawDigit(x, y, nDeadBall);
            x += 6;
            DEBUG_drawChar(x, y, nBallHits);
            x += 10;
            DEBUG_drawChar(x, y, xNew);
            x += 10;
            DEBUG_drawChar(x, y, yNew);


            x  = 0;
            y -= 25;
            DEBUG_drawChar(x, y, (unsigned char) (iTimerIdle >> 8));
            DEBUG_drawChar(x, y, (unsigned char) (iTimerIdle & 0x0f));
            x += 20;
            DEBUG_drawChar(x, y, (unsigned char) (iDelayNewBall >> 8));
            DEBUG_drawChar(x, y, (unsigned char) (iDelayNewBall & 0x0f));


            x  = 0;
            y -= 25;
            DEBUG_drawDigit(x, y, L_used);
            x += 10;
            DEBUG_drawDigit(x, y, L_Btn);
            x += 10;
            DEBUG_drawChar(x, y, L_angle);

            x = 127;
            DEBUG_drawDigit(x, y, R_used);
            x += 10;
            DEBUG_drawDigit(x, y, R_Btn);
            x += 10;
            DEBUG_drawChar(x, y, R_angle);
        }
        
        x = 0;
        y = Net_X;
        V_Write = 0;
        H_Write = Net_X;
        
        /*
        y = 190;
        x = 0;
        V_Write = y;
        H_Write = x;
        DEBUG_line_V(64);
        V_Write = (nSide > 0) ? y + 50 : y;
        DEBUG_line_H(20);
        DEBUG_line_V(64);
        V_Write = (nDeadBall > 0) ? y + 50 : y;
        DEBUG_line_H(20);
        DEBUG_line_V(64);
        V_Write = y + (unsigned char) nBallHits / 25;
        DEBUG_line_H(20);
        DEBUG_line_V(64);
        V_Write = y + (unsigned char) (iDelayNewBall >> 16) / 25;
        DEBUG_line_H(30);
        DEBUG_line_V(64);
        V_Write = y + (unsigned char) (iDelayNewBall % 255) / 25;
        DEBUG_line_H(30);
        DEBUG_line_V(64);
        
        DEBUG_drawPlayerL(0,   120);
        DEBUG_drawPlayerR(127, 120);
        
        */

		// Get ready for the next iteration
        // New values become old values
		VxOld = VxNew;
		VyOld = VyNew;
		xOld  = xNew;
		yOld  = yNew;

	}

}

float getCustomSin(unsigned char angle){
    if (angle > Angle_Max){
        angle = Angle_Max;
    }
    if (angle < Angle_Min){
        angle = Angle_Min;
    }
    
    if (angle >= Angle_Delta){
        return simplesin(angle - Angle_Delta);
    }
    
    return simplesin(256 - Angle_Delta + angle);
}

float getCustomCos(unsigned char angle){
    if (angle > Angle_Max){
        angle = Angle_Max;
    }
    if (angle < Angle_Min){
        angle = Angle_Min;
    }

    if (angle >= Angle_Delta){
        return simplecos(angle - Angle_Delta);
    }
    
    return simplecos(256 - Angle_Delta + angle);
}

void ADC_start(unsigned char nChannel) {
    // Enable ADC
    ADCON0bits.ADON = 1;
    // Select channel
    ADCON0bits.CHS0 = ( nChannel & 0b00000001);
    ADCON0bits.CHS1 = ((nChannel & 0b00000010) >> 1);
    ADCON0bits.CHS2 = ((nChannel & 0b00000100) >> 2);
    // Start AD conversion
    ADCON0bits.GO   = 1;
} // end ADC_start


void DEBUG_line_H(unsigned char delta){
    for (j = 0; j < 5; j++){
        for (m = 0; m < delta; m++) {
            H_Write = (j % 2 == 0) ? x++ : x--;
        }
    }
}

void DEBUG_line_V(unsigned char delta){
    for (m = 0; m < delta; m++) {
        V_Write = y++;
    }
    for (m = 0; m < delta; m++) {
        V_Write = y--;
    }
}

void XY_drawLineY(signed char delta){
    H_Write = x;
    V_Write = y;
    if (delta > 0){
        while (delta > 0){
            V_Write = y++;
            delta--;
        }
    }
    else{
        while (delta < 0){
            V_Write = y--;
            delta++;
        }
    }
}

void XY_drawLineX(signed char delta){
    H_Write = x;
    V_Write = y;
    if (delta > 0){
        while (delta > 0){
            H_Write = x++;
            delta--;
        }
    }
    else{
        while (delta < 0){
            H_Write = x--;
            delta++;
        }
    }
}



void DEBUG_drawChar(unsigned char xin, unsigned char yin, unsigned char digit){
    DEBUG_drawDigit(xin, yin, (digit & 0xf0) >> 4);
    DEBUG_drawDigit(x, y, digit & 0x0f);
}


void DEBUG_drawDigit(unsigned char xin, unsigned char yin, unsigned char digit){
    signed char wh   = 4;
    signed char w    = 8;
    signed char w2   = 16;
    unsigned char s  = 4;

    if (digit == 0){
        x = xin;
        y = yin;
        XY_drawLineY(w2);
        XY_drawLineX(w);
        XY_drawLineY(-w2);
        XY_drawLineX(-w);
    }
    else if (digit == 1){
        x = xin + w;
        y = yin;
        XY_drawLineY(w2);
        XY_drawLineY(-w2);
    }
    else if (digit == 2){
        x = xin;
        y = yin + w2;
        XY_drawLineX(w);
        XY_drawLineY(-w);
        XY_drawLineX(-w);
        XY_drawLineY(-w);
        XY_drawLineX(w);
    }
    else if (digit == 3){
        x = xin;
        y = yin + w2;
        XY_drawLineX(w);
        XY_drawLineY(-w);
        XY_drawLineX(-w);
        XY_drawLineX(w);
        XY_drawLineY(-w);
        XY_drawLineX(-w);
    }
    else if (digit == 4){
        x = xin;
        y = yin + w2;
        XY_drawLineY(-w);
        XY_drawLineX(w);
        XY_drawLineY(w);
        XY_drawLineY(-w2);
    }
    else if (digit == 5){
        x = xin + w;
        y = yin + w2;
        XY_drawLineX(-w);
        XY_drawLineY(-w);
        XY_drawLineX(w);
        XY_drawLineY(-w);
        XY_drawLineX(-w);
    }
    else if (digit == 6){
        x = xin + w;
        y = yin + w2;
        XY_drawLineX(-w);
        XY_drawLineY(-w2);
        XY_drawLineX(w);
        XY_drawLineY(w);
        XY_drawLineX(-w);
    }
    else if (digit == 7){
        x = xin;
        y = yin + w2;
        XY_drawLineX(w);
        XY_drawLineY(-w2);
    }
    else if (digit == 8){
        x = xin;
        y = yin + w;
        XY_drawLineY(w);
        XY_drawLineX(w);
        XY_drawLineY(-w);
        XY_drawLineX(-w);
        XY_drawLineY(-w);
        XY_drawLineX(w);
        XY_drawLineY(w);
    }
    else if (digit == 9){
        x = xin + w;
        y = yin + w;
        XY_drawLineX(-w);
        XY_drawLineY(w);
        XY_drawLineX(w);
        XY_drawLineY(-w2);
    }
    else if (digit == 10){ // A
        x = xin;
        y = yin + w + wh;
        XY_drawLineX(w);
        XY_drawLineY(-wh);
        XY_drawLineY(-w);
        XY_drawLineX(-w);
        XY_drawLineY(w);
        XY_drawLineX(w);
    }
    else if (digit == 11){ // b
        x = xin;
        y = yin + w2;
        XY_drawLineY(-w2);
        XY_drawLineX(w);
        XY_drawLineY(w);
        XY_drawLineX(-w);
    }
    else if (digit == 12){ // c
        x = xin + w;
        y = yin + w;
        XY_drawLineX(-w);
        XY_drawLineY(-w);
        XY_drawLineX(w);
    }
    else if (digit == 13){ // d
        x = xin + w;
        y = yin + w2;
        XY_drawLineY(-w2);
        XY_drawLineX(-w);
        XY_drawLineY(w);
        XY_drawLineX(w);
    }
    else if (digit == 14){ // e
        x = xin + w;
        y = yin;
        XY_drawLineX(-w);
        XY_drawLineY(w);
        XY_drawLineY(wh);
        XY_drawLineX(w);
        XY_drawLineY(-w);
        XY_drawLineX(-w);
    }
    else if (digit == 15){ // f
        x = xin;
        y = yin;
        XY_drawLineY(w);
        XY_drawLineX(wh);
        XY_drawLineX(-wh);
        XY_drawLineY(w);
        XY_drawLineX(wh);
    }
   
    x = xin + w + s;
    y = yin;
}



void XY_drawLineDelta(unsigned char xs, unsigned char ys, signed char dx, signed char dy){
    XY_drawLine(xs, ys, xs + dx, ys + dy);
}

void XY_drawLine(unsigned char xs, unsigned char ys, unsigned char xe, unsigned char ye){
    unsigned char x = 0;
    unsigned char y = 0;
    signed char   dx = 1;
    signed char   dy = 1;
    
    dx = xe >= xs ? 1 : -1;
    dy = ye >= ys ? 1 : -1;
    
    x = xs;
    y = ys;
    while (x != xe && y != ye){
        if (x != xe){
            x += dx;
        }
        if (y != ye){
            y += dy;
        }
        H_Write = x;
        V_Write = y;
    }
}

void XY_drawGround(void){
    // AT LEFT
    y = 0;
    x = 0;
    V_Write = y;
    H_Write = x;

    // To right
    while (x < Net_X) {
        H_Write = x;
        x++;
    }

    // Net base
    V_Write = y;
    H_Write = x; // X-position of NET

    // Up
    while (y < Net_H) {
        V_Write = y;
        y++;
    }
    // Down
    while (y > 1) {
        V_Write = y;
        y--;
    }
    // Net base
    V_Write = y;
    H_Write = x;
    // To right up to the end
    while (x < 255) {
        H_Write = x;
        x++;
    }
    // AT RIGHT
    V_Write = y;
    H_Write = x;
    // To left
    while (x > Net_X) {
        H_Write = x;
        x--;
    }
    // Net base
    V_Write = y;
    H_Write = x;
    // Up
    while (y < Net_H) {
        V_Write = y;
        y++;
    }
    // Down
    while (y > 1) {
        V_Write = y;
        y--;
    }
    // Net base
    V_Write = y;
    H_Write = x;
    // To left up to start
    while (x > 1) {
        H_Write = x;
        x--;
    }
    // Starting point
    V_Write = y;
    H_Write = x;
}