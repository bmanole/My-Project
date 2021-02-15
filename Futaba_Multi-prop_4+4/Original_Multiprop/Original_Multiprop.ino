#include <VarSpeedServo.h>
//#include <Servo.h>
#include <EnableInterrupt.h>

#define NUM_OUTPUTS         8
#define NUM_SERVOS          4

volatile long lastChange = 0;

int lastDebugTime = 0;
int debugInterval = 200;

#define AUX_IN_PIN A4 //input pin for the correspondent multiprop channel (in my case, CH6 -> A4);

volatile uint8_t current_output = 0;
volatile uint16_t unAuxInShared;
volatile uint32_t ulAuxStart;
volatile uint8_t output_ready;
volatile int multi_output[NUM_OUTPUTS]; // Array that holds effective RC reading from multiprop;
int nonISR_output[NUM_OUTPUTS];
int outputA_pins[NUM_OUTPUTS];
int outputB_pins[NUM_OUTPUTS];

//Servo servos[NUM_SERVOS]; // only 4 servo, 1 per each prop, only one servo used in my experiment
VarSpeedServo servos[NUM_SERVOS];

int crnt_state_A = 1; // 0  1  2  for three state switch
int prev_state_A = -1;

int upper_state_A = 0; // 0 or 1, on or off
int lower_state_A = 0;

void setup()
{
    pinMode(AUX_IN_PIN, INPUT_PULLUP);

    //set current default values for servo settings
    crnt_state_A = 1; //0, 1, or 2  vals for 3 position switch
    prev_state_A = 1;
    upper_state_A = 0; // 0 or 1, on or off
    lower_state_A = 0;

    Serial.begin(115200);

    // Init array of output pins
    // 1-4 Props
    // 5-8 - (3 Position Switches)

    // Primary function array, includes both switched and props

    outputA_pins[0] = 5;    // Prop 1
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
    outputB_pins[4] = 8;    // Switch 1
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
                servos[outputnum].attach( outputA_pins[outputnum] );
                servos[outputnum].writeMicroseconds( multi_output[outputnum] );

            }//if

        }//else

    }//for

    unAuxInShared = 0;
    output_ready = 0;

    //attach an interrupt to the RX input pin of the multiprop 4 + 4

    enableInterrupt( AUX_IN_PIN, calcAux, CHANGE );

}//setup

void loop()
{
    int outputnum = 0;
    //uint32_t crtTime = millis();

    //if the interrupt routine flag is set, we have a set of servos to move
    if( output_ready == 1 )
    {
        noInterrupts();
        // copy ISR_accessed array to working, non-ISR array in protected section
        for( uint8_t idx=0; idx<8; idx++ )
            nonISR_output[idx] = multi_output[idx];
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
                    servos[outputnum].write( map(nonISR_output[outputnum], 1000,2000, 0, 180), 51);

                break;

                case    4:
                    // I only use one switch to light two distinct led circuits, position 5
                    if( (nonISR_output[outputnum] >= 1020) && (nonISR_output[outputnum] <= 1070) )
                    {
                        // switch upper position  - crnt_state_A = 2
                        crnt_state_A = 2;
                        if( crnt_state_A != prev_state_A )
                        {
                            upper_state_A ^= 1;
                            prev_state_A = crnt_state_A;

                        }//if

                    }//if

                    if( (nonISR_output[outputnum] >= 1940) && (nonISR_output[outputnum] <= 1980) )
                    {
                        // switch lower position  - crnt_state_A = 0
                        crnt_state_A = 0;
                        if( crnt_state_A != prev_state_A )
                        {
                            lower_state_A ^= 1;
                            prev_state_A = crnt_state_A;

                        }//if

                    }//if

                    if ((nonISR_output[outputnum] >= 1480) && (nonISR_output[outputnum] <= 1540))
                    {
                        // switch middle(neutral) position  - crnt_state_A = 1
                        crnt_state_A = 1;
                        prev_state_A = 1;

                    }//if

                    //check the on/off state and act
                    digitalWrite( outputA_pins[outputnum], (upper_state_A == 1) ? HIGH:LOW );
                    digitalWrite( outputB_pins[outputnum], (lower_state_A == 1) ? HIGH:LOW );

                break;

                case    5:
                case    6:
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

void calcAux()
{
   static bool
        bNeedSync = true;

    uint8_t pinState = digitalRead( AUX_IN_PIN );

    ulAuxStart = micros();                     //Save current time

    if( pinState == HIGH )
        lastChange = ulAuxStart;
    else
    {
        //Calculate time of high-going pulse
        unAuxInShared = ulAuxStart - lastChange;
        if( bNeedSync )
        {
            if( (unAuxInShared > 850) && (unAuxInShared < 950) )
            {
                //Filter the start pulse of 915
                current_output = 0; //Set index current_output to 0 so that next pulses are written from value [0].
                bNeedSync = false;

            }//if

        }//if
        else
        {
            multi_output[current_output++] = unAuxInShared; //Write time on array
            if( current_output == 8 )
            {
                bNeedSync = true;
                output_ready = 1;

            }//if

        }//else

    }//else

}//calcAux
