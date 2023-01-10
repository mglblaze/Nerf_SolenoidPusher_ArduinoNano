//Pin reference constants
const int REV_IN = 3;
const int TRIGGER_IN = 2;
const int FIRE_SEMI_IN = 4;
const int FIRE_BURST_IN = 5;
const int FIRE_AUTO_IN = 6;
const int REV_OUT = 7;
const int PUSHER_OUT = 8;

//Pusher activation time constants, in milliseconds
const unsigned long PUSHER_UP_TIME = 20;
const unsigned long PUSHER_DOWN_TIME = 100;

//Fire mode constants, to improve future readability and reduce the use of magic numbers
const int FIRE_SEMI = 0;
const int FIRE_3_BURST = 1;
const int FIRE_AUTO = 2;

//Burst shot length constant
const int BURST_THREE = 3;

//Trigger button state variables
int triggerState = HIGH;
int previousTriggerState = HIGH;
int revState = HIGH;

//Solenoid sdequence control flag
bool pusherActive = false;

//Pusher control timers
unsigned long triggerTime = 0;
unsigned long pusherTime = 0;

//Fire control variable. 0 = semi-automatic, 1 = burst, 2 = automatic
int fireSelect = 0;

//Burst count variable. Set equal to BURST_THREE at initialization, only set to 1 upon first starting a burst sequence
int burstCount = BURST_THREE;
//Burst target variable. For future extensibility if I or someone else wants to make multiple burst modes that shoot different numbers of darts
int burstTarget = BURST_THREE;

void setup() 
{
  //Set arduino pin Input modes.
  //All are set to Pullup mode; will need to be pulled low by surrounding circuit
  pinMode(REV_IN, INPUT_PULLUP);
  pinMode(TRIGGER_IN, INPUT_PULLUP);
  pinMode(FIRE_SEMI_IN, INPUT_PULLUP);
  pinMode(FIRE_BURST_IN, INPUT_PULLUP);
  pinMode(FIRE_AUTO_IN, INPUT_PULLUP);

  //Set arduino pin Output modes
  pinMode(REV_OUT, OUTPUT);
  pinMode(PUSHER_OUT, OUTPUT);
}

void loop() 
{
  if (pusherActive) //If the pusherActive flag is true, simply call the Pusher method immediately to continue an in-progress shot without checking the trigger input
  {
    revState = LOW; //Always set revState to LOW to prevent the pusher sending a dart in to inactive flywheels
    Pusher(); //Call Pusher method to continue current shot sequence
  }
  else //Otherwise perform the usual checks
  {
    //Read fire select button state. If no fire select buttons are pulled low, keep previous state.
    //This is only done while a shot is not currently being executed to prevent odd behaviour from changing fire modes mid-shot.
    if (digitalRead(FIRE_SEMI_IN) == LOW) fireSelect = FIRE_SEMI; //Semi
    if (digitalRead(FIRE_BURST_IN) == LOW) fireSelect = FIRE_3_BURST; //Burst
    if (digitalRead(FIRE_AUTO_IN) == LOW) fireSelect = FIRE_AUTO; //Auto

    //Read the trigger and rev button state.
    triggerState = digitalRead(TRIGGER_IN);
    revState = digitalRead(REV_IN);

    if (!revState) //If the flywheels are not set to spin, continue the main loop without calling any fire control methods to prevent a dart getting jammed in to inactive flywheels
      //Switch-case statement for fire select. Allows for more patterns to be added in the future if desired
      switch (fireSelect)
      {
        case FIRE_SEMI: //For semi auto shots, check if the trigger has been released since the last shot before starting a new shot sequence
          if (triggerState == LOW && previousTriggerState == HIGH) Shoot();
          break;
        case FIRE_3_BURST: //For burst shots, perform the same check
          if (triggerState == LOW && previousTriggerState == HIGH) 
          {
            burstTarget = BURST_THREE; //Set burstTarget
            burstCount = 1; //Set burstCount variable to 1 to start a new burst
            Shoot(); //Then start a new shot sequence
          }
          break;
        case FIRE_AUTO: //For full auto shots, simply check if the trigger is being held down
          if (triggerState == LOW) Shoot();
          break;
        default: break;
      }
  }
  
  previousTriggerState = triggerState;

  //Send intended flywheel motor state: HIGH if revState was pulled LOW, and vice versa
  digitalWrite(REV_OUT, !revState);

  //Delay for stability
  delay(1);
}

//Method to start a shot cycle
void Shoot()
{
  //Set the current triggerTime, set the pusherActive flag, and then call the Pusher method
  triggerTime = millis();
  pusherActive = true;
  Pusher();
}

//Pusher handler method
void Pusher()
{
  //Get the intended state of the pusher pin, then send to PUSHER_OUT via digitalWrite
  digitalWrite(PUSHER_OUT, PusherSequence());
}

//Pusher sequence method
bool PusherSequence()
{
  //Subtract the time the trigger was first pressed from the current time to figure out how much time has elapsed since the trigger press
  pusherTime = millis() - triggerTime;
  
  if (pusherTime > PUSHER_UP_TIME + PUSHER_DOWN_TIME) //If the pusher time has exceeded the total of PUSHER_UP_TIME + PUSHER_DOWN_TIME...
  {
    if (burstCount < burstTarget) // And the current burstCount is less than the intended burst length (burstCount only set otherwise during burst fire mode)
    {
      burstCount++; //Increment burst count
      triggerTime = millis(); //Reset trigger time to current system clock time for the next cycle
    }
    else //Otherwise, if burstCount => burstLength, as would be the case for when a burst has finished, or at all times for all non-burst fire modes,
      pusherActive = false; //Set the pusherActive flag to false to signal the end of the current shot sequence
  }
  else if (pusherTime < PUSHER_UP_TIME)
  {
    //If pusherTime is less than PUSHER_UP_TIME, return HIGH
    return HIGH;
  }

  //When either pusherTime is greater than PUSHER_UP_TIME but less than the sum of PUSHER_UP_TIME + PUSHER_DOWN_TIME or as a failsafe, return LOW
  return LOW;
}
