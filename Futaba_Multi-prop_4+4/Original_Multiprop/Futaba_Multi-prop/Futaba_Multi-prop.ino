#include <VarSpeedServo.h>
//#include <EnableInterrupt.h>
#include "ThreeStateDoubleSwitch.h"

#define NUM_OUTPUTS         8
#define NUM_SERVOS          4
#define NUM_SWITCHES        4

//#define AUX_IN_PIN 8 //input pin for the correspondent multiprop channel (in my case, CH6 -> A4);

byte last_channel_1;
unsigned long timer_1, current_time, receiver_input;


volatile uint8_t current_output = 0;
volatile uint16_t unAuxInShared;
volatile uint32_t ulAuxStart;
volatile uint8_t output_ready;
volatile int multi_output[NUM_OUTPUTS]; // Array that holds effective RC reading from multiprop;
int nonISR_output[NUM_OUTPUTS] = {0,}; // Array that holds a copy multi_output values, outside ISR 

int outputA_pins[NUM_OUTPUTS];
int outputB_pins[NUM_OUTPUTS];

//Servo servos[NUM_SERVOS]; // only 4 servo, 1 per each prop, only one servo used in my experiment

VarSpeedServo servos[NUM_SERVOS];

ThreeStateDoubleSwitch switches[NUM_SWITCHES];

void calcAux();

void setup()
{
 //   pinMode(AUX_IN_PIN, INPUT_PULLUP); 

 //   Serial.begin(115200); 

PCICR |= (1 << PCIE0);                                                    //Set PCIE0 to enable PCMSK0 scan.
PCMSK0 |= (1 << PCINT0);                                                  //Set PCINT0 (digital input 8) to trigger an interrupt on state change.



    // Init array of output pins
    // 1-4 Props
    // 5-8 - (3 Position Switches)

    // Primary function array, includes both switched and props

    outputA_pins[0] = 6;    // Prop 1
    outputA_pins[1] = 0;    // Prop 2
    outputA_pins[2] = 0;    // Prop 3
    outputA_pins[3] = 0;    // Prop 4
    outputA_pins[4] = 7;    // Switch 1
    outputA_pins[5] = 0;    // Switch 2
    outputA_pins[6] = 0;    // Switch 3
    outputA_pins[7] = 0;    // Switch 4

    // Secondary function array, includes only switches

    outputB_pins[0] = 0;    // Prop 1 - not used, reserved for 2 switching function
    outputB_pins[1] = 0;    // Prop 2 - not used, reserved for 2 switching function
    outputB_pins[2] = 0;    // Prop 3 - not used, reserved for 2 switching function
    outputB_pins[3] = 0;    // Prop 4 - not used, reserved for 2 switching function
    outputB_pins[4] = 9;    // Switch 1
    outputB_pins[5] = 0;    // Switch 2
    outputB_pins[6] = 0;    // Switch 3
    outputB_pins[7] = 0;    // Switch 4

    // assign output pins

    for( int outputnum=0; outputnum<NUM_OUTPUTS; outputnum++ )
    {
        if( outputA_pins[outputnum] != 0 )
        {
            pinMode( outputA_pins[outputnum], OUTPUT );
            pinMode( outputB_pins[outputnum], OUTPUT );

        }//if

    }//for

    //set up the array of switch and servo objects, and init switch to LOW and servo to a "centered" position

    for( int outputnum=0; outputnum<NUM_OUTPUTS; outputnum++ )
    {
        if( outputnum > 3 )
        {
            //non-servo outputs
            if( outputA_pins[outputnum] != 0 )
                digitalWrite( outputA_pins[outputnum], LOW );
            if( outputB_pins[outputnum] != 0 )
                digitalWrite( outputB_pins[outputnum], LOW );

        }//if
        else
        {
            //servo outputs
            multi_output[outputnum] = 1500;
            if( outputA_pins[outputnum] != 0 )
            {
                servos[outputnum].attach( outputA_pins[outputnum], 1020,1980 );
                servos[outputnum].writeMicroseconds( multi_output[outputnum] );

            }//if

        }//else

    }//for

    unAuxInShared = 0;
    output_ready = 0;

    //attach an interrupt to the RX input pin of the multiprop 4 + 4

 //   enableInterrupt( AUX_IN_PIN, calcAux, CHANGE );

}//setup

void loop()
{
    int outputnum = 0;
/*
if (output_ready == 1) {
  
for (outputnum = 0; outputnum < 8; outputnum++) {

  Serial.print(outputnum);
  Serial.print("/");
  Serial.print(multi_output[outputnum]);
  Serial.print(" ");
  }
Serial.println();
delay(250);
} 
*/
    //if the interrupt routine flag is set, we have a set of servos to move
    if( output_ready == 1 )
    {
      
       noInterrupts();
        // copy ISR_accessed array to working, non-ISR array in protected section
        for( uint8_t idx=0; idx<8; idx++ ){
       
          nonISR_output[idx] = multi_output[idx];
        }
      interrupts();

        for (outputnum = 0; outputnum < 8; outputnum++)
        {

         
           switch( outputnum )
            {
                case    0:
                //case    1:
                //case    2:
                //case    3:
                
                  
                    // I only use one Prop to drive a servo, position 1
                    servos[outputnum].write( map(nonISR_output[outputnum],1020,1980,5,175), 28);
                    //servos[outputnum].writeMicroseconds(nonISR_output[outputnum]);
                   

                    
                    break;

                case    4:
                //case    5: 
                //case    6:
                    // I only use one switch to light two distinct led circuits, position 5
                    switches[outputnum-4].computeNewState(nonISR_output[outputnum]);
                    //check the on/off state and act
                    digitalWrite(outputA_pins[outputnum], switches[outputnum-4].isUpperSwitchOn() ? HIGH:LOW );
                    digitalWrite(outputB_pins[outputnum], switches[outputnum-4].isLowerSwitchOn() ? HIGH:LOW );
                    break;

                case    7:
                    if( (nonISR_output[outputnum] >= 1020) && (nonISR_output[outputnum] <= 1070) )
                    {
                        digitalWrite( outputA_pins[outputnum], HIGH );

                    }//if

                    if( (nonISR_output[outputnum] >= 1940) && (nonISR_output[outputnum] <= 1980) )
                    {
                        digitalWrite( outputB_pins[outputnum], HIGH );

                    }//if
                    if( (nonISR_output[outputnum] >= 1480) && (nonISR_output[outputnum] <= 1540) )
                    {
                        digitalWrite( outputA_pins[outputnum], LOW );
                        digitalWrite( outputB_pins[outputnum], LOW );

                    }//if
                    break;
            }//switch

        }//for

        output_ready = 0;

    }//if

}//loop


ISR(PCINT0_vect){
  current_time = micros();
  //Channel 6=========================================
  if(PINB & B00000001){                                                     //Is input 8 high?
    if(last_channel_1 == 0){                                                //Input 8 changed from 0 to 1.
      last_channel_1 = 1;                                                   //Remember current input state.
      timer_1 = current_time;                                               //Set timer_1 to current_time.
    }
  }
  else if(last_channel_1 == 1){                                             //Input 8 is not high and changed from 1 to 0.
    last_channel_1 = 0;                                                     //Remember current input state.
    receiver_input = current_time - timer_1;                                //Channel 6 is current_time - timer_1.

      if ((receiver_input > 850)&&(receiver_input <950)){
          current_output = 0;
      
         }
          else {

                if (current_output <8){
          
                  multi_output[current_output]= receiver_input;
                  current_output++;
                  output_ready = 1;
                 }
    
          }
  }
}
