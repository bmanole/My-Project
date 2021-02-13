#include <Servo.h>
#include <EnableInterrupt.h>

volatile long lastChange = 0;

int lastDebugTime = 0;
int debugInterval = 200;

#define AUX_IN_PIN A4 //input pin for the correspondent multiprop channel (in my case, CH6 -> A4);

volatile uint8_t current_output = 0;
volatile uint16_t unAuxInShared;
volatile uint32_t ulAuxStart;
volatile uint8_t output_ready;
volatile int multi_output[8]; // Array that holds effective RC reading from multiprop;

int outputA_pins[8];
int outputB_pins[8];

Servo servos[4]; // only 4 servo, 1 per each prop, only one servo used in my experiment

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

  outputA_pins[0] = 5; //Prop 1
  outputA_pins[1] = 0; //Prop 2
  outputA_pins[2] = 0; //Prop 3
  outputA_pins[3] = 0; //Prop 4
  outputA_pins[4] = 7; // Switch 1
  outputA_pins[5] = 0; // Switch 2
  outputA_pins[6] = 0; // Switch 3
  outputA_pins[7] = 0; // Switch 4

  // Secondary function array, includes only switches

  outputB_pins[0] = 0; //Prop 1 - not used, reserved for 2 switching function
  outputB_pins[1] = 0; //Prop 2 - not used, reserved for 2 switching function
  outputB_pins[2] = 0; //Prop 3 - not used, reserved for 2 switching function
  outputB_pins[3] = 0; //Prop 4 - not used, reserved for 2 switching function
  outputB_pins[4] = 8; //Switch 1
  outputB_pins[5] = 0; //Switch 2
  outputB_pins[6] = 0; //Switch 3
  outputB_pins[7] = 0; //Switch 4

  // assign output pins

  for (int outputnum = 0; outputnum < 6; outputnum++)
  {
    pinMode(outputA_pins[outputnum], OUTPUT);
    pinMode(outputB_pins[outputnum], OUTPUT);
  }

  //set up the array of switch and servo objects, and init switch to LOW and servo to a "centered" position

  for (int outputnum = 0; outputnum < 6; outputnum++)
  {

    if (outputnum > 3)
    {
      digitalWrite(outputA_pins[outputnum], LOW);
      digitalWrite(outputB_pins[outputnum], LOW);
    }
    else
    {

      multi_output[outputnum] = 1500;
      servos[outputnum].attach(outputA_pins[outputnum]);
      servos[outputnum].write(multi_output[outputnum]);
    }
  }

  unAuxInShared = 0;
  output_ready = 0;

  //attach an interrupt to the RX input pin of the multiprop 4 + 4

  enableInterrupt(AUX_IN_PIN, calcAux, CHANGE);
  //PCintPort::attachInterrupt(AUX_IN_PIN, calcAux, CHANGE);
}

void loop()
{

  int outputnum = 0;

  int crtTime = millis();

  //if the interrupt routine flag is set, print multiswitch values;

  if (output_ready == 1)
  {
    if(crtTime > lastDebugTime + debugInterval) {
      for (outputnum = 0; outputnum < 8; outputnum++)
      {

        Serial.print(outputnum + 1);
        Serial.print("/");
        Serial.print(multi_output[outputnum]);
        Serial.print(" ");
      }
      Serial.println();
      lastDebugTime = crtTime;
    }
  }

  //if the interrupt routine flag is set, we have a set of servos to move

  if (output_ready == 1)
  {
    for (outputnum = 0; outputnum < 8; outputnum++)
    {

      if (outputnum == 4)
      {

        // I only use one switch to light two distinct led circuits, position 5

        if ((multi_output[outputnum] >= 1020) && (multi_output[outputnum] <= 1070))
        {

          // switch upper position  - crnt_state_A = 2

          crnt_state_A = 2;
          if (crnt_state_A != prev_state_A)
          {
            if (upper_state_A == 1)
            {
              upper_state_A = 0;
            }
            else
            {
              upper_state_A = 1;
            }
            prev_state_A = crnt_state_A;
          }
        }

        if ((multi_output[outputnum] >= 1940) && (multi_output[outputnum] <= 1980))
        {

          // switch lower position  - crnt_state_A = 0

          crnt_state_A = 0;
          if (crnt_state_A != prev_state_A)
          {
            if (lower_state_A == 1)
            {
              lower_state_A = 0;
            }
            else
            {
              lower_state_A = 1;
            }
            prev_state_A = crnt_state_A;
          }
        }

        if ((multi_output[outputnum] >= 1480) && (multi_output[outputnum] <= 1540))
        {

          // switch middle(neutral) position  - crnt_state_A = 1

          crnt_state_A = 1;
          prev_state_A = 1;
        }

        //check the on/off state and act

        if (upper_state_A == 1)
        {

          digitalWrite(outputA_pins[outputnum], HIGH);
        }
        else
        {
          digitalWrite(outputA_pins[outputnum], LOW);
        }

        if (lower_state_A == 1)
        {

          digitalWrite(outputB_pins[outputnum], HIGH);
        }
        else
        {
          digitalWrite(outputB_pins[outputnum], LOW);
        }
      }

      if (outputnum > 4)
      {
        if ((multi_output[outputnum] >= 1020) && (multi_output[outputnum] <= 1070))
        {
          digitalWrite(outputA_pins[outputnum], HIGH);
        }
        if ((multi_output[outputnum] >= 1940) && (multi_output[outputnum] <= 1980))
        {
          digitalWrite(outputB_pins[outputnum], HIGH);
        }
        if ((multi_output[outputnum] >= 1480) && (multi_output[outputnum] <= 1540))
        {
          digitalWrite(outputA_pins[outputnum], LOW);
          digitalWrite(outputB_pins[outputnum], LOW);
        }
      }

      if (outputnum == 0)
      {

        servos[outputnum].write(multi_output[outputnum]);

        // I only use one Prop to drive a servo, position 1
      }
      if (outputnum == 1)
      {
        //servos[outputnum].writeMicroseconds(multi_output[outputnum]);
      }
      if (outputnum == 2)
      {
        //servos[outputnum].writeMicroseconds(multi_output[outputnum]);
      }
      if (outputnum == 3)
      {
        //servos[outputnum].writeMicroseconds(multi_output[outputnum]);
      }
    }
    output_ready = 0;
  }
}

void calcAux()
{
  ulAuxStart = micros();                     //Save current time
  unAuxInShared = (ulAuxStart - lastChange); //Calculate time since the last flank
  if ((unAuxInShared > 700) && (unAuxInShared < 2200))
  { //Filter HIGH pulse If time between 700 and 2200 is a HIGH pulse
    if ((unAuxInShared > 850) && (unAuxInShared < 950))
    {                     //Filter the start pulse of 915
      current_output = 0; //Set index current_output to 0 so that next pulses are written from value [0].
      output_ready = 1;
    }
    else
    {
      multi_output[current_output] = unAuxInShared; //Write time on array
      current_output++;                             //Increase index by 1 for the next pulse
    }
  }

  lastChange = ulAuxStart; // Save current time for the next interr  upt
}
