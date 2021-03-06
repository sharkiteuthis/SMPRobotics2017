/* Tobi.cpp
Created by Cherag Bhagwagar 
10/21/2016
bit |= (1<<x) sets bit x high
bit &= ~(1<<x) sets bit x low
x starts from 0

read from PCF8574AN

/////////////////////////////////////
 $ Changes to code by Andrea Frank
	marked with $ before comment.
	7/18/17
/////////////////////////////////////

*/

/***************************************************************************************/
/***************************************************************************************/
/****************************** $ GLOBAL VARIABLES *************************************/
/***************************************************************************************/
/***************************************************************************************/


// Make sure you include wire.h in your code to run this library 

#include "Tobi.h"

void *__ptr1; 		// $ placeholder pointer 

#define __pcf2 57	// $ addresses for PCF IO expander
#define __pcf1 56

byte __byte1 = 0;	// $ placeholder bits
byte __byte2 = 0;

/***************************************************************************************/
/***************************************************************************************/
/************************************ $ METHODS ****************************************/
/***************************************************************************************/
/***************************************************************************************/

/////////////////////////// SETUP ////////////////////////////

/*  $         CONSTRUCTOR
	Initialize all variables on this instance.
	INPUTS: 	- None
	OUTPUTS: 	- None
	UPDATED: 	Tobi::__pwmPins, Tobi::__encoderPins
*/
Tobi::Tobi()
{
	digitalWrite(13,HIGH);		// $ turn on LED, I think?

	// for (int i = 0 ; i < 5; i ++ ){
	// 	Serial.println("Working");
	// 	Tobi::__encoderVal[i] = 0;
	// }

	Tobi::__pwmPins[0] = 9;
	Tobi::__pwmPins[1] = 5; 		// $ nevermind, digial 5 vs analog 5, different pins;    xxxx$ changed from 5 to 12, because Tobi::__encoderPins[5] = 5
	Tobi::__pwmPins[2] = 6;
	Tobi::__pwmPins[3] = 10;
	Tobi::__pwmPins[4] = 11;
	Tobi::__pwmPins[5] = 13;		// $ <--- THIS PWM PIN CAUSES A PROBLEM (it pulses with the LEDs during upload)
									// pretty sure its a problem with the PCB; we have it wired to something that 
									// pulses on startup -- maybe change it to D4 or D12?

	digitalWrite(13,LOW);		// $ turn off LED

	Tobi::__encoderPins[0] = 0;
	Tobi::__encoderPins[1] = 1;
	Tobi::__encoderPins[2] = 2;
	Tobi::__encoderPins[3] = 3;
	Tobi::__encoderPins[4] = 4;
	Tobi::__encoderPins[5] = 5;
	digitalWrite(13,HIGH); 		// $ turn on LED
}

/*  $         ENABLE
	Set up and enable hardware. Sets IO pins to proper direction, writes 0 to two pcf pins, sets default motor
		directions, turns on power for each axis, and writes pwm to 0 for each motor. It then cascades LEDs on
		and off and prints a complete statement to Serial (if enabled).
	NOTE: DOES NOT ENABLE AXIS 2 (M4-M5), AS THIS LINKS MOTOR 5 TO UPLOAD SEQUENCE (spins with flashing LEDS AND
		DOES NOT ACTIVATE MOTOR. Direction and pwm are still set for motors 4 and 5, but axis is not powered. If
		use of motor 4 and 5 is desired, it should be powered separately in code. This will make it spin during upload.
	INPUTS: 	- None
	OUTPUTS: 	- None
	UPDATED:	- maxEncoderVals initialized.
	EFFECTS:	- Writes 0 to pins __pcf1 and __pcf2, and 0 to all pins in Tobi::__pwmPins. Turns on all axis and sets motor
					directions to default. Cascades LEDs.
*/
void Tobi::enable(void){
	if(Serial) Serial.println("Enabling TOBI...");
	/* Sets the robot in Tobi Mode, enabling and disabling pins */

	// $ Set PWM pins to output 
	for (int i = 0 ; i < NUM_MOTORS; i ++){
		pinMode(Tobi::__pwmPins[i],OUTPUT);
	}
	// $ Set encoder pins to input
	for (int i = 0 ; i < NUM_MOTORS; i ++){
		pinMode(Tobi::__encoderPins[i],INPUT);
	}

	// $ initialize maxEncoderVals
	for (int i =0; i < NUM_MOTORS; i++){
		maxEncoderVals[i] = 1023;		// default max encoder val
	}

	// $ write 0 to IO expanders
	Wire.begin();
	Tobi::__write8(__pcf1,0);
	Tobi::__write8(__pcf2,0);

	for(int i = 0 ; i < NUM_MOTORS; i ++) {		
		analogWrite(Tobi::__pwmPins[i], 0);	
	}
		
	// $ turn all motor axes off originally to avoid issues with setting direction and pwm
	for (int i = 0; i < 3; i++){
		Tobi::powerAxis(i,0);
	}
	// $ set up motors initially in forward direction, pwm = 0, then turn axis power on
	for (int i = 0; i < NUM_MOTORS; i++){
		Tobi::setMotor(i,1);	// set motors in default direction
		Tobi::setPwm(i,0);			// set duty cycle to 0
	}
	for (int i = 0; i < NUM_AXES; i++){
		Tobi::powerAxis(i,1);	// axis power on
	}

	// $ cascade LEDS on and then off		
  	for (int i = 0 ; i < 6 ; i ++){
  		Tobi::led(i,1);
  		delay(100);
  	}
	for(int i = 6 ; i >=0 ; i --) {
		Tobi::led(i,0);
  		delay(100);	
	}

	if(Serial) Serial.println("TOBI enabled.\n");
}


/*  $         DISABLE
	Disable TOBI. Set all legs to speed 0, unpower all axes, and clear __byte1 and __byte2.
	INPUTS: 	- None.
	OUTPUTS: 	- None.
	UPDATED:	- __byte1, __byte2.
*/
void Tobi::disable(){
	for(int i = 0; i < 3; i++){
		Tobi::powerAxis(i,0);
	}
	for(int i  = 0 ; i < NUM_MOTORS ; i ++){
		analogWrite(Tobi::__pwmPins[i], 0); 	//all leg speed to 0
	}
	__byte2 = 0 ;
	__byte1 = 0 ;
	if(Serial) Serial.println("TOBI disabled.\n");
}


/////////////////////////// MOTION ////////////////////////////

/*  $         SETMOTOR
	Set motor direction. Uses PCF io expander and bit shifts.
	INPUTS: 	- (int) motor, (int) direction.
	OUTPUTS: 	- None
	UPDATED:	- __byte1.
*/
void Tobi::setMotor(int motor , int direction){
	// $ check for valid motor number input
    if ( (motor < 0) && (motor > 5) ){
    	Serial.println("Wrong Motor");
    	return;
    }
	
	// 
    if (direction == 1)			__byte1 |= (1<<motor);        // $ assign "motor"th bit to 1    
    else if (direction == -1)	__byte1 &= ~(1<<(motor)); 	 // $ assign "motor"th bit to 0
    else Serial.println ("Invalid direction. Must choose 1 or -1.") ;
    Tobi::__write8(__pcf1,__byte1);	// $ write __byte1 to __pcf1 through PCF io expander.
}

/*  $         POWERAXIS
	Turn motor axis on or off. Can only control power to motors in pairs (axes),
		M0-M1 (axis 0), M2-M3 (axis 1), and M4-M5 (axis 2).
	INPUTS: 	- (int) axis, (int) state.
	OUTPUTS: 	- None
	UPDATED:	- __byte1, __byte2.
*/
void Tobi::powerAxis (int axis, int state){
	//TODO Check pins of new eagle file 
	/* turn motor axis on/off (0 -> M0-M1 ,1 -> M2-M3, 2-> M4-M5 anything else-> error
        works on enable pins for motor driver
    */
    switch (axis){
    	case 0 : 
	    	if (state == 1)			__byte1 |= (1<<6);
	    	else if (state == 0)	__byte1 &= ~(1<<6);
	    	Tobi::__write8(__pcf1,__byte1);
	    	break;
    	case 1 :
	    	if (state == 1)			__byte1 |= (1<<7);
	    	else if (state == 0)	__byte1 &= ~(1<<7);
	    	Tobi::__write8(__pcf1,__byte1);
	    	break;
    	case 2 :
	    	if (state == 1)			__byte2 |= (1<<0);
	    	else if (state == 0)	__byte2 &= ~(1<<0);
	    	Tobi::__write8(__pcf2,__byte2);
	    	break;
   	 	default:
   	 		Serial.println ("Wrong command") ;
   	}
}

/*  $         SETPWM
	Set PWM of motor from 0 - 255.
	INPUTS: 	- (int) motor, (int) pwm.
	OUTPUTS: 	- None.
	UPDATED:	- None.
*/
void Tobi::setPwm(int motor, int pwm){
	Tobi::__pwmVal[motor] = pwm;
	analogWrite(Tobi::__pwmPins[motor], pwm);	
}

/*  $         GETPWM
	Returns PWM of desired motor from 0 - 255.
	INPUTS: 	- (int) motor
	OUTPUTS: 	- None.
	UPDATED:	- __pwmVal[motor]
*/
int Tobi::getPwm(int motor){
	return Tobi::__pwmVal[motor];
}


///////////////////////// READ ENCODERS /////////////////////////

/*  $         CALIBRATEENCODERS
	Finds max values of each encoder and saves them to an array passed
	in by reference. maxEncoderVals MUST be an array of size NUM_MOTORS.
	If encoders are not numbered 0-NUM_MOTORS, must pass in a third
	argument that is a pointer to an array that indexes encoder numbers.
	INPUTS: 	- int* maxEncoderVals, int NUM_MOTORS (optional int* encoderIndices)
	OUTPUTS: 	- None.
	UPDATED:	- maxEncoderVals
*/
void Tobi::calibrateEncoders(){
	if(Serial) Serial.println("Calibrating encoders...");
  	for(int i = 0; i < NUM_MOTORS; i++){
	    // set motor to max speed
	    Tobi::setPwm(i,255);
	    // record 100 encoder values
	    int maxVal = 0;
	    for (int j = 0; j < 100; j++){
	      // read encoder value and save if max
	      int v = Tobi::readEncoder(i);
	      if(v > maxVal) maxVal = v;
	      delay(20);
	    }
	    // save in maxEncoderVals[i]
	    maxEncoderVals[i] = maxVal;
	    // turn motor off
    	Tobi::setPwm(i,0);
 	}
 	if(Serial) Serial.println("Calibration complete.\n");
}
void Tobi::calibrateEncoders(int* encoderIndices){
	if(Serial) Serial.println("Calibrating encoders...");
	for(int i = 0; i < NUM_MOTORS; i++){
	    // set motor to max speed
	    Tobi::setPwm(encoderIndices[i],255);
	    // record 100 encoder values
	    int maxVal = 0;
	    for (int j = 0; j < 100; j++){
	      // read encoder value and save if max
	      int v = Tobi::readEncoder(encoderIndices[i]);
	      if(v > maxVal) maxVal = v;
	      delay(20);
	    }
	    // save in maxEncoderVals[i]
	    maxEncoderVals[i] = maxVal;
	    // turn motor off
	    Tobi::setPwm(encoderIndices[i],0);
	  }
	if(Serial) Serial.println("Calibration complete.\n");
}

/*  $         READENCODER
	Call to read the value reported by the encoder for a specific leg. Method is necessary
		because __encoderVal is private. Calls _analogUpdate() before polling values. Encoder
		values usually range from 0-1023. However, it is good practice to use calibrateEncoders()
		to determine the true max value for each encoder, as occasionally an encoder will roll
		back to 0 at ~600 instead of 1023.
	INPUTS: 	- (int) leg 	-> 	number 0 to 5 to specify which leg to query.
	OUTPUTS: 	- (int) value 	-> 	number 0 to 1023 from encoder, corresponds to motor angle.
	UPDATED:	- encoderVal
*/
int Tobi::readEncoder(int motor){
	Tobi::_analogUpdate();	// update _encoderVal
	return (Tobi::__encoderVal[motor]);
}



/////////////////////////// LEDS ////////////////////////////

/*  $         LED
	Turn LED (0,1,2,3,4,5) on (1) or off (0). Note: for state, any nonzero number will be treated as on.
	INPUTS: 	- (int) led, (int) state.
	OUTPUTS: 	- None
	UPDATED:	- __byte2. 
*/
void Tobi::led(int led, int state){
	if (state == 1){	__byte2 |= (1<<led+1);
	}
	else {
		__byte2 &= ~(1<<led+1);
	}
	Tobi::__write8(__pcf2,__byte2);
}

/*  $         NOSELED
	Turn nose LED off (state = 0) or on (state = 1).
	INPUTS: 	- (int) state.
	OUTPUTS: 	- None.
	UPDATED:	- __byte2.
*/
void Tobi::noseLed(int state){
	if (state == 1)		__byte2 |= (1<<7); 
	else 				__byte2 &= ~(1<<7) ;
}



/////////////////////////// HELPER METHODS ////////////////////////////

/*  $         _ANALOGUPDATE
	Read encoder values into Tobi::__encoderVal array for easy access. Should be called before 
		acting upon Tobi::__encoderVal values.
	INPUTS: 	- None
	OUTPUTS: 	- None
	UPDATED:	- Tobi::__encoderVal
*/
void Tobi::_analogUpdate(){
	//call in loop everytime
	Tobi::__encoderVal[0] = analogRead(0);
	Tobi::__encoderVal[1] = analogRead(1);
	Tobi::__encoderVal[2] = analogRead(2);
	Tobi::__encoderVal[3] = analogRead(3);
	Tobi::__encoderVal[4] = analogRead(4);
	Tobi::__encoderVal[5] = analogRead(5);
}

/*  $         WRITE8
	Write one byte to chosen address. Used to write to PCF expander.
	INPUTS: 	- (int) address, (byte) value.
	OUTPUTS: 	- None.
	UPDATED:	- None.
*/
void Tobi::__write8(int address, byte value){
  Wire.beginTransmission(address);
  Wire.write(value);
  Wire.endTransmission();
}

/***************************************************************************************/
/***************************************************************************************/
/***************************** $ UNFINISHED METHODS ************************************/
/***************************************************************************************/
/***************************************************************************************/

// $ methods left unfinished by Cherag

void Tobi::print_raw(){
	//TODO
	Serial.print("BIT 1:\t");
	Serial.println(__byte1,BIN);
	Serial.print("BIT 2:\t");
	Serial.println(__byte2,BIN);
	Serial.println("---------------------------");

	}

void Tobi::print(){
	//prints out all angle in format 
	// angle leg <leg #> <angle value>
	for (int i = 0 ; i < NUM_MOTORS; i ++){
		Serial.print("angle leg "); Serial.print(i); Serial.print("  "); Serial.println(Tobi::__encoderVal[i]);
	}

	//TODO
}
