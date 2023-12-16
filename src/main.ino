/////////////// HEADER SECTION /////////////////
// Program Name: EngProj2 W13 ArduinoLEDDisplay
// Author(s): Noah Grant, Brandon Hauck
// Date: April 4, 2022
// Version: 1
// Class: Engineering Project 2
// Comments: this program is the culmination of our work this semester
//////////////////////////////////////////////////////////////

///////////// DIRECTIVE SECTION /////////////
#include <string.h>

#define SERIAL_CLOCK 12 //shifts data into shift registers when pulsed (SRCLK)
#define LATCH_CLOCK  11 //releases shift register data to LED matrix when pulsed (RCLK)
#define SERIAL_DATA  10 //data line for the shift registers 

#define DEMUX_A 7
#define DEMUX_B 8
#define DEMUX_C 9

#define ROW_CNT 6 //number of rows in LEDdisplay[][] array
#define COL_CNT 24 //number of columns of LED array
#define BUF_LEN 26 //number of columns in buffer that holds input string
#define SENSOR_UPDATE_DELAY 5000 //miliseconds before checking temp
#define SCROLL_DELAY 300 //miliseconds before scroling the display frame
////////////////////////////////////////////////////////////

//PROTOTYPE FUNCTIONS
int initialize(); //initalizes values at the start of the program
String input(String &userCMD, int tC, int aC, int tF, int c1, int c2); //gets input from the user
int sensorInput(int &tC, int &aC, int &tF, int &c1, int &c2); //checks the voltage value from the temp sensors and sets the text to be displayed
int brightness(String userCMD, int &refresh, int &delayOff, int c1, int c2); //update brightness level of display
char assembleDisplay(String stringText, String userCMD); //takes input string and converts it to LED pattern
int displayPattern(char arr[ROW_CNT][BUF_LEN], int refresh, int delayOff, int scrollPointer); //display row pattern for current row
String updateSensors(String userCMD, int tC, int aC, int tF, int c1, int c2, int oC, int oaC, int oF, int oc1, int oc2); //updates temperature after certain amount of time

/////////////// CODE SECTION ////////////////
void setup() 
{
  initialize();
}

//MAIN FUNCTION
void loop() 
{
  //LOCAL VARIABLE DELCARATION
  int onTime; //microseconds between changing row, ON time for PWM. This needs to be changed in order to change the brightness level of display
  int offTime; //OFF time for PWM
  char LEDdisplay[ROW_CNT][BUF_LEN]; //display array
  String displayText; //set text to display. Make the string "" to display nothing on the LED display
  String userInput; //collects the input for the temp the user wants to display
  int previousTimer; //time when last shift occured
  int currentTimer; //time since program began
  int avgC; //average temp in C to display
  int tempF; //temp in C to display
  int tempC; //temp in C to display
  int config1; //light sensor 1
  int config2; //light sensor 2
  int windowPointer; //tells program how to display the frame window
  
  //EXECUTION
  if(Serial.available() > 0) //change what is displaying if new text is entered in serial monitor
  {
    sensorInput(tempC, avgC, tempF, config1, config2);
    displayText = input(userInput, tempC, avgC, tempF, config1, config2);
    brightness(userInput, onTime, offTime, config1, config2);
    LEDdisplay[ROW_CNT][BUF_LEN] = assembleDisplay(displayText, userInput);
    windowPointer = 0; //reset pointer so that text starts scrolling from the beginning
  }
  
  displayPattern(LEDdisplay, onTime, offTime, windowPointer);

  //update display
  currentTimer = millis();
  
  if(userInput == "6") //increase window pointer if the user is scrolling text
  {
    if(currentTimer - previousTimer >  SCROLL_DELAY)
    {
      windowPointer = (windowPointer + 1) % BUF_LEN; //moves the window pointer to display next frame of animation
      previousTimer = currentTimer;
    }
  }

  else //only update sensors if the user is not scrolling text on the display
  {
    if(currentTimer - previousTimer > SENSOR_UPDATE_DELAY)
    { 
      //save old temps
      int oldC = tempC; //save old temps
      int oldF = tempF;
      int oldAvgC = avgC;
      int oldConfig1 = config1;
      int oldConfig2 = config2;
  
      sensorInput(tempC, avgC, tempF, config1, config2); //check current temp    
      
      displayText = updateSensors(userInput, tempC, avgC, tempF, config1, config2, oldC, oldF, oldAvgC, oldConfig1, oldConfig2); //update display if temp is different than previous temp
      brightness(userInput, onTime, offTime, config1, config2);
      assembleDisplay(displayText, userInput); 
      
      previousTimer = currentTimer;
    }
  }
}

///////////////////////// FUNCTIONS /////////////////////////
/*
    -Function Name:         initialize 
    -Purpose:               set baud rate, and pin outputs/inputs before main program loop. Print the menu on the console screen
    -Input(s):          Variable Name   Variable Type   Purpose
                            NA
    -Output(s):         Variable Name   Variable Type   Purpose
                            NA
    Algorithm:              NA
*/
int initialize()
{ 
  //set baud rate
  Serial.begin(9600);
  
  //initalizes pin outputs
  pinMode(SERIAL_CLOCK, OUTPUT);
  pinMode(LATCH_CLOCK, OUTPUT);
  pinMode(SERIAL_DATA, OUTPUT);

  pinMode(DEMUX_A, OUTPUT);
  pinMode(DEMUX_B, OUTPUT);
  pinMode(DEMUX_C, OUTPUT); 

  pinMode(A0, INPUT); //averaging Celsius temp
  pinMode(A1, INPUT); //Fahrenheit temp
  pinMode(A3, INPUT); //single Celsius temp
  
  pinMode(A4, INPUT); //phototarnsistor config1
  pinMode(A5, INPUT); //phototransistor config2

  //print out menu
  Serial.println("================= MENU =================");
  Serial.println("1. Temperature in Celcius");
  Serial.println("2. Average Temperature in Celsius");
  Serial.println("3. Temperature in Fahrenheit");
  Serial.println("4. Light sensor config1");
  Serial.println("5. Light sensor config2");
  Serial.println("6. Display text on display");

  while(Serial.available() == 0) //wait for user to input option from menu before going to main program
  {
  }

  return 0;
}

/*
    -Function Name:       checkTemp
    -Purpose:             to check the temperature and display it on the console
    -Input(s):       Variable Name   Variable Type   Purpose
                          tC            int             get the celcius temp from the main loop
                          aC            int             get the average celsius temp from the main loop
                          tF            int             get the fahrenheit temp from the main loop
                          c1            int             get the light level from config1
                          c2            int             get light level from config2
    -Output(s):      Variable Name   Variable Type   Purpose
                          NA
    Algorithm:            read voltage values from sensors, convert ADC values from temp sensors to temp
*/
int sensorInput(int &tC, int &aC, int &tF, int &c1, int &c2)
{
  //EXECUTION
  //read ADC values from temp sensors
  aC = analogRead(A0);
  tF = analogRead(A1);
  tC = analogRead(A3);

  //read ADC values from light sensors
  c1 = analogRead(A4);
  c2 = analogRead(A5);

  //convert ADC values to temperature
  aC = aC / 10; 
  tF = (tF * 4.88) / 20;
  tC = (tC * 4.88) / 10;

  return 0;
}

/*
    -Function Name:     input  
    -Purpose:           collects input from user and returns string to display
    -Input(s):       Variable Name   Variable Type   Purpose
                        userCMD       String          read user input from main loop
                        tC            int             get the celcius temp from the main loop
                        aC            int             get the average celsius temp from the main loop
                        tF            int             get the fahrenheit temp from the main loop
                        c1            int             get the light level from config1
                        c2            int             get light level from config2
    -Output(s):      Variable Name   Variable Type   Purpose
                        stringText    String          return string to display back to main loop
                        userCMD       String          returns the user input to the main function
    Algorithm:          collects the command that the user wants to carry out, and creates the string to display
*/
String input(String &userCMD, int tC, int aC, int tF, int c1, int c2)
{ 
  //LOCAL VARIABLE DECLARATION AND INITIALIZATION
  String stringText = "";

  //EXECUTION
  userCMD = Serial.readString();

  userCMD.remove(1,1); //remove \n char
  
  Serial.println("==========================================");

  if(userCMD == "1")
  {
    Serial.println("Currently displaying the temperature in Celsius");
    Serial.println("The temperature in Celsius is " + String(tC));

    stringText = "C:" + String(tC);
  }

  else if(userCMD == "2")
  {
    Serial.println("Currently displaying the average temperature in Celsius");
    Serial.println("The average temperature in Celsius is " + String(aC));

    stringText = "Cav:" + String(aC);
  }

  else if(userCMD == "3")
  {
    Serial.println("Currently displaying the temperature in Fahrenheit");
    Serial.println("The temperature in Fahrenheit is " + String(tF));

    stringText = "F:" + String(tF);
  }

  else if(userCMD == "4")
  {
    Serial.println("Currently displaying light level of config1");
    Serial.println("The light level is " + String(c1));

    stringText = "lvl:" + String(c1); 
  }

  else if(userCMD == "5")
  {
    Serial.println("Currently displaying light level of config2");
    Serial.println("The light level is " + String(c2));

    stringText = "lv2:" + String(c2);    
  }

  else if (userCMD == "6") //display scrolling text
  {
    Serial.println("What would you like to display? (up to " + String(BUF_LEN / 7) + " characters)");
    while(Serial.available() == 0) {} //wait for input

    stringText = Serial.readString();

    if(stringText.length() > BUF_LEN / 7 + 1) //tell user if the string they entered was too long
    {
      Serial.println("Your text was too long to display correctly.");
    }
    Serial.println("Currently displaying: " + stringText.substring(0, BUF_LEN / 7));
    stringText = stringText.substring(0, BUF_LEN / 7);
  }

  else
  {
    Serial.println("You did not enter a valid option");
  }

  return stringText;
}

/*
    -Function Name:     brightness  
    -Purpose:           change brightness level of LED display
    -Input(s):       Variable Name   Variable Type   Purpose
                        userCMD        String         read user input from main loop
                        refresh        int            update the amount of time the LEDs are on
                        delayOff       int            update the amount of time the LEDs are off
                        c1             int            get the light level from config1
                        c2             int            get light level from config2
    -Output(s):      Variable Name   Variable Type   Purpose
                        refresh        int            update the amount of time the LEDs are on
                        delayOff       int            update the amount of time the LEDs are off
    Algorithm:          sets brightness level depending on what the light sensor reads
*/
int brightness(String userCMD, int &refresh, int &delayOff, int c1, int c2)
{ 
  if(userCMD == "4")
  {
    //convert ADC light input to brightness level
    if(c1 <= 2)
    {
      refresh = 10;
      Serial.println("Brightness level is " + String(refresh) + "%");
    }
  
    else if(c1 >= 3 && c1 <= 5)
    {
      refresh = 55;
      Serial.println("Brightness level is " + String(refresh) + "%");
    }
  
    else if(c1 >= 6 && c1 <= 50)
    {
      refresh = 85;
      Serial.println("Brightness level is " + String(refresh) + "%");
    }
  
    else
    {
      refresh = 100;
      Serial.println("Brightness level is " + String(refresh) + "%");
    }       
  }

  else if(userCMD == "5")
  {
    if(c2 <= 30)
    {
      refresh = 10;
      Serial.println("Brightness level is " + String(refresh) + "%");
    }
  
    else if(c2 >= 31 && c2 <= 150)
    {
      refresh = 55;
      Serial.println("Brightness level is " + String(refresh) + "%");
    }
  
    else if(c2 >= 151 && c2 <= 800)
    {
      refresh = 85;
      Serial.println("Brightness level is " + String(refresh) + "%");
    }
  
    else
    {
      refresh = 100;
      Serial.println("Brightness level is " + String(refresh) + "%");
    } 
  }

  else //adaptive brightness for temp sensors
  {
    if(c2 <= 30)
    {
      refresh = 20;
    }
  
    else if(c2 >= 31 && c2 <= 150)
    {
      refresh = 50;
    }
  
    else if(c2 >= 151 && c2 <= 800)
    {
      refresh = 99;
    }
  
    else
    {
      refresh = 100;
    }      
  }    

  delayOff = 100 - refresh;

  return 0;
}

/*
    -Function Name:       assembleDisplay   
    -Purpose:             turns input string into LED pattern
    -Input(s):      Variable Name     Variable Type   Purpose
                          stringText    String          read the text to display from the main loop
                          userCMD       String          read the user input command from the main loop
    -Output(s):     Variable Name     Variable Type   Purpose
                          stringText    String          return the LED pattern to display
    Algorithm:            gets input from user and then goes character by character and loads each character pattern into the array to be returned to the main function
*/
char assembleDisplay(String stringText, String userCMD)
{ 
  //LOCAL VARIABLE DECLARATION AND INITIALIZATION
  char arr[ROW_CNT][BUF_LEN] = {"","","","","",""};

  //EXECUTION
  //load character patterns into LEDdisplay
  for(int i = 0; i < stringText.length(); i++)
  {
    switch(stringText[i])
    {
      case 'a':
      case 'A':
      {
        if(userCMD == "6")
        {
          strcat(arr [0], "0111100");
          strcat(arr [1], "1000010");
          strcat(arr [2], "1000010");
          strcat(arr [3], "1111110");
          strcat(arr [4], "1000010");
          strcat(arr [5], "1000010");
        }
        
        else
        {
          strcat(arr [0], "0000");
          strcat(arr [1], "0000");
          strcat(arr [2], "1110");
          strcat(arr [3], "1010");
          strcat(arr [4], "1110");
          strcat(arr [5], "1010");
        }
  
        break;
      }
  
      case 'b':
      case 'B':
      {
        strcat(arr [0], "1111100");
        strcat(arr [1], "1000010");
        strcat(arr [2], "1111100");
        strcat(arr [3], "1000010");
        strcat(arr [4], "1000010");
        strcat(arr [5], "1111100");
  
        break;
      }
  
      case 'c':
      case 'C':
      {
        if(userCMD == "6")
        {
          strcat(arr [0], "1111110");
          strcat(arr [1], "1000000");
          strcat(arr [2], "1000000");
          strcat(arr [3], "1000000");
          strcat(arr [4], "1000000");
          strcat(arr [5], "1111110");
        }

        else
        {
          strcat(arr [0], "1110");
          strcat(arr [1], "1000");
          strcat(arr [2], "1000");
          strcat(arr [3], "1000");
          strcat(arr [4], "1000");
          strcat(arr [5], "1110");
        }
  
        break;
      }
  
      case 'd':
      case 'D':
      {
        strcat(arr [0], "1111100");
        strcat(arr [1], "1000010");
        strcat(arr [2], "1000010");
        strcat(arr [3], "1000010");
        strcat(arr [4], "1000010");
        strcat(arr [5], "1111100");
  
        break;
      }
  
      case 'e':
      case 'E':
      {
        strcat(arr [0], "1111110");
        strcat(arr [1], "1000000");
        strcat(arr [2], "1111110");
        strcat(arr [3], "1000000");
        strcat(arr [4], "1000000");
        strcat(arr [5], "1111110");
  
        break;
      }
  
      case 'f':
      case 'F':
      {
        if(userCMD == "6")
        {
          strcat(arr [0], "1111110");
          strcat(arr [1], "1000000");
          strcat(arr [2], "1111000");
          strcat(arr [3], "1000000");
          strcat(arr [4], "1000000");
          strcat(arr [5], "1000000");
        }

        else
        {
          strcat(arr [0], "1110");
          strcat(arr [1], "1000");
          strcat(arr [2], "1110");
          strcat(arr [3], "1000");
          strcat(arr [4], "1000");
          strcat(arr [5], "1000");
        }
  
        break;
      }
  
      case 'g':
      case 'G':
      {
        strcat(arr [0], "1111110");
        strcat(arr [1], "1000000");
        strcat(arr [2], "1000000");
        strcat(arr [3], "1001110");
        strcat(arr[4], "1000010");
        strcat(arr [5], "1111110");
  
        break;
      }
  
      case 'h':
      case 'H':
      {
        strcat(arr [0], "1000010");
        strcat(arr [1], "1000010");
        strcat(arr [2], "1111110");
        strcat(arr [3], "1000010");
        strcat(arr [4], "1000010");
        strcat(arr [5], "1000010");
  
        break;
      }
  
      case 'i':
      case 'I':
      {
        strcat(arr [0], "1111110");
        strcat(arr [1], "0011000");
        strcat(arr [2], "0011000");
        strcat(arr [3], "0011000");
        strcat(arr [4], "0011000");
        strcat(arr [5], "1111110");
  
        break;
      }
  
      case 'j':
      case 'J':
      {
        strcat(arr [0], "1111110");
        strcat(arr [1], "0001000");
        strcat(arr [2], "0001000");
        strcat(arr [3], "0001000");
        strcat(arr [4], "1001000");
        strcat(arr [5], "0110000");
  
        break;
      }
  
      case 'k':
      case 'K':
      {
        strcat(arr [0], "1000010");
        strcat(arr [1], "1000100");
        strcat(arr [2], "1001000");
        strcat(arr [3], "1111000");
        strcat(arr [4], "1000100");
        strcat(arr [5], "1000010");
  
        break;
      }
  
      case 'l':
      case 'L':
      {
        if(userCMD == "6")
        {
          strcat(arr [0], "1000000");
          strcat(arr [1], "1000000");
          strcat(arr [2], "1000000");
          strcat(arr [3], "1000000");
          strcat(arr [4], "1000000");
          strcat(arr [5], "1111110");
        }
        
        else
        {
          strcat(arr [0], "1000");
          strcat(arr [1], "1000");
          strcat(arr [2], "1000");
          strcat(arr [3], "1000");
          strcat(arr [4], "1000");
          strcat(arr [5], "1110");
        }
  
        break;
      }
  
      case 'm':
      case 'M':
      {
        strcat(arr [0], "1000010");
        strcat(arr [1], "1100110");
        strcat(arr [2], "1011010");
        strcat(arr [3], "1000010");
        strcat(arr [4], "1000010");
        strcat(arr [5], "1000010");
  
        break;
      }
  
      case 'n':
      case 'N':
      {
        strcat(arr [0], "1000010");
        strcat(arr [1], "1100010");
        strcat(arr [2], "1010010");
        strcat(arr [3], "1001010");
        strcat(arr [4], "1000110");
        strcat(arr [5], "1000010");
  
        break;
      }
  
      case 'o':
      case 'O':
      {
        strcat(arr [0], "1111110");
        strcat(arr [1], "1000010");
        strcat(arr [2], "1000010");
        strcat(arr [3], "1000010");
        strcat(arr [4], "1000010");
        strcat(arr [5], "1111110");
  
        break;
      }
  
      case 'p':
      case 'P':
      {
        strcat(arr [0], "111110");
        strcat(arr [1], "1000010");
        strcat(arr [2], "1000010");
        strcat(arr [3], "1111100");
        strcat(arr [4], "1000000");
        strcat(arr [5], "1000000");
  
        break;
      }
  
      case 'q':
      case 'Q':
      {
        strcat(arr [0], "1111110");
        strcat(arr [1], "1000010");
        strcat(arr [2], "1000010");
        strcat(arr [3], "1001010");
        strcat(arr [4], "1000100");
        strcat(arr [5], "1111010");
  
        break;
      }
  
      case 'r':
      case 'R':
      {
        strcat(arr [0], "1111100");
        strcat(arr [1], "1000010");
        strcat(arr [2], "1000010");
        strcat(arr [3], "1111100");
        strcat(arr [4], "1000100");
        strcat(arr [5], "1000010");
  
        break;
      }
  
      case 's':
      case 'S':
      {
        strcat(arr [0], "1111110");
        strcat(arr [1], "1000000");
        strcat(arr [2], "1111110");
        strcat(arr [3], "0000010");
        strcat(arr [4], "0000010");
        strcat(arr [5], "1111110");
  
        break;
      }
  
      case 't':
      case 'T':
      {
        strcat(arr [0], "1111110");
        strcat(arr [1], "0011000");
        strcat(arr [2], "0011000");
        strcat(arr [3], "0011000");
        strcat(arr [4], "0011000");
        strcat(arr [5], "0011000");
  
        break;
      }
  
      case 'u':
      case 'U':
      {
        strcat(arr [0], "1000010");
        strcat(arr [1], "1000010");
        strcat(arr [2], "1000010");
        strcat(arr [3], "1000010");
        strcat(arr [4], "1000010");
        strcat(arr [5], "0111100");
  
        break;
      }
  
      case 'v':
      case 'V':
      {
        if(userCMD == "6")
        {
          strcat(arr [0], "1000010");
          strcat(arr [1], "1000010");
          strcat(arr [2], "0100100");
          strcat(arr [3], "0100100");
          strcat(arr [4], "0100100");
          strcat(arr [5], "0011000");
        }

        else
        {
          strcat(arr [0], "0000");
          strcat(arr [1], "0000");
          strcat(arr [2], "1010");
          strcat(arr [3], "1010");
          strcat(arr [4], "1010");
          strcat(arr [5], "0100");
        }
  
        break;
      }
  
      case 'w':
      case 'W':
      {
        strcat(arr [0], "1000010");
        strcat(arr [1], "1000010");
        strcat(arr [2], "1000010");
        strcat(arr [3], "1011010");
        strcat(arr [4], "1011010");
        strcat(arr [5], "1100110");
  
        break;
      }
  
      case 'x':
      case 'X':
      {
        strcat(arr [0], "1000010");
        strcat(arr [1], "0100100");
        strcat(arr [2], "0011000");
        strcat(arr [3], "0011000");
        strcat(arr [4], "0100100");
        strcat(arr [5], "1000010");
  
        break;
      }
  
      case 'y':
      case 'Y':
      {
        strcat(arr [0], "1000010");
        strcat(arr [1], "1000010");
        strcat(arr [2], "0100100");
        strcat(arr [3], "0011000");
        strcat(arr [4], "0011000");
        strcat(arr [5], "0011000");
  
        break;
      }
  
      case 'z':
      case 'Z':
      {
        strcat(arr [0], "1111110");
        strcat(arr [1], "0000100");
        strcat(arr [2], "0001000");
        strcat(arr [3], "0010000");
        strcat(arr [4], "0100000");
        strcat(arr [5], "1111110");
  
        break;
      }
  
      case ' ':
      {
        strcat(arr [0], "0000");
        strcat(arr [1], "0000");
        strcat(arr [2], "0000");
        strcat(arr [3], "0000");
        strcat(arr [4], "0000");
        strcat(arr [5], "0000");
  
        break;
      }
      
      case '!':
      {
        strcat(arr [0], "1110");
        strcat(arr [1], "1110");
        strcat(arr [2], "1110");
        strcat(arr [3], "1110");
        strcat(arr [4], "0000");
        strcat(arr [5], "1110");
  
        break;
      }
  
      case '?':
      {
        strcat(arr [0], "1110");
        strcat(arr [1], "0010");
        strcat(arr [2], "1110");
        strcat(arr [3], "1000");
        strcat(arr [4], "0000");
        strcat(arr [5], "1000");
  
        break;
      }
  
      case '.':
      {
        strcat(arr [0], "0000");
        strcat(arr [1], "0000");
        strcat(arr [2], "0000");
        strcat(arr [3], "0000");
        strcat(arr [4], "1110");
        strcat(arr [5], "1110");
  
        break;
      }
  
      case ':':
      {
        strcat(arr [0], "0000");
        strcat(arr [1], "1110");
        strcat(arr [2], "1110");
        strcat(arr [3], "0000");
        strcat(arr [4], "1110");
        strcat(arr [5], "1110");
  
        break;
      }
  
      case '0':
      {
        strcat(arr [0], "1110");
        strcat(arr [1], "1110");
        strcat(arr [2], "1010");
        strcat(arr [3], "1010");
        strcat(arr [4], "1110");
        strcat(arr [5], "1110");
  
        break;
      }
  
      case '1':
      {
        strcat(arr [0], "1100");
        strcat(arr [1], "0100");
        strcat(arr [2], "0100");
        strcat(arr [3], "0100");
        strcat(arr [4], "0100");
        strcat(arr [5], "1110");
  
        break;
      }
  
      case '2':
      {
        strcat(arr [0], "1110");
        strcat(arr [1], "0010");
        strcat(arr [2], "1110");
        strcat(arr [3], "1000");
        strcat(arr [4], "1000");
        strcat(arr [5], "1110");
  
        break;
      }
  
      case '3':
      {
        strcat(arr [0], "1110");
        strcat(arr [1], "0010");
        strcat(arr [2], "1110");
        strcat(arr [3], "0010");
        strcat(arr [4], "0010");
        strcat(arr [5], "1110");
  
        break;
      }
  
      case '4':
      {
        strcat(arr [0], "1010");
        strcat(arr [1], "1010");
        strcat(arr [2], "1110");
        strcat(arr [3], "0010");
        strcat(arr [4], "0010");
        strcat(arr [5], "0010");
  
        break;
      }
  
      case '5':
      {
        strcat(arr [0], "1110");
        strcat(arr [1], "1000");
        strcat(arr [2], "1110");
        strcat(arr [3], "0010");
        strcat(arr [4], "0010");
        strcat(arr [5], "1110");
  
        break;
      }
  
      case '6':
      {
        strcat(arr [0], "1000");
        strcat(arr [1], "1000");
        strcat(arr [2], "1000");
        strcat(arr [3], "1110");
        strcat(arr [4], "1010");
        strcat(arr [5], "1110");
  
        break;
      }
  
      case '7':
      {
        strcat(arr [0], "1110");
        strcat(arr [1], "0010");
        strcat(arr [2], "0010");
        strcat(arr [3], "0010");
        strcat(arr [4], "0010");
        strcat(arr [5], "0010");
  
        break;
      }
  
      case '8':
      {
        strcat(arr [0], "1110");
        strcat(arr [1], "1010");
        strcat(arr [2], "1110");
        strcat(arr [3], "1010");
        strcat(arr [4], "1010");
        strcat(arr [5], "1110");
  
        break;
      }
  
      case '9':
      {
        strcat(arr [0], "1110");
        strcat(arr [1], "1010");
        strcat(arr [2], "1110");
        strcat(arr [3], "0010");
        strcat(arr [4], "0010");
        strcat(arr [5], "0010");
  
        break;
      }
  
      default:
      {
        break;
      }
    }
  } 
  
  return arr[ROW_CNT][BUF_LEN];
}

/*
    -Function Name:       displayPattern
    -Purpose:             outputs LED pattern.
    -Input(s):          Variable Name   Variable Type   Purpose
                          arr            char           read the LED pattern from the main loop
                          refresh        int            read the amount of time the LEDs are on
                          delayOff       int            read the amount of time the LEDs are off
                          scrollPointer  int            creates the scrolling animation for the text on the LED display
    -Output(s):         Variable Name   Variable Type   Purpose
                          NA
    Algorithm:            Loads row pattern into shift register and displays the pattern. Repeats for each row. function displays every row once before moving on in the main program loop
*/
int displayPattern(char arr[ROW_CNT][BUF_LEN], int refresh, int delayOff, int scrollPointer)
{
  for(int row = 0; row < ROW_CNT; row++)
  {
    //load pattern for current row into shift register
    for(int col = 0; col < COL_CNT; col++)
    {
      if (arr[row][col + scrollPointer] % BUF_LEN == '1') //turn LED ON if there is a 1 at the index in the matrix
      {
        digitalWrite(SERIAL_DATA, HIGH);
      }
    
      else
      {
        digitalWrite(SERIAL_DATA, LOW);
      }
      
      digitalWrite(SERIAL_CLOCK, HIGH);
      digitalWrite(SERIAL_CLOCK, LOW);
    }
    
    digitalWrite(LATCH_CLOCK, HIGH);
    digitalWrite(LATCH_CLOCK, LOW);

    //display pattern for current row
    switch(row)
    {
      case 0:
      {       
        digitalWrite(DEMUX_A, LOW);
        digitalWrite(DEMUX_B, LOW);
        digitalWrite(DEMUX_C, LOW);
        break;
      }
      
      case 1:
      {      
        digitalWrite(DEMUX_A, LOW);
        digitalWrite(DEMUX_B, LOW);
        digitalWrite(DEMUX_C, HIGH);
        break;
      }
  
      case 2:
      {
        
        digitalWrite(DEMUX_A, LOW);
        digitalWrite(DEMUX_B, HIGH);
        digitalWrite(DEMUX_C, LOW);
        break;
      }
  
      case 3:
      { 
        digitalWrite(DEMUX_A, LOW);
        digitalWrite(DEMUX_B, HIGH);
        digitalWrite(DEMUX_C, HIGH);
        break;
      }
  
      case 4:
      {    
        digitalWrite(DEMUX_A, HIGH);
        digitalWrite(DEMUX_B, LOW);
        digitalWrite(DEMUX_C, LOW);
        break;
      }
  
      case 5:
      {     
        digitalWrite(DEMUX_A, HIGH);
        digitalWrite(DEMUX_B, LOW);
        digitalWrite(DEMUX_C, HIGH);
        break;
      }
  
      default:
      {
        digitalWrite(DEMUX_A, HIGH);
        digitalWrite(DEMUX_B, HIGH);
        digitalWrite(DEMUX_C, HIGH);
        break;
      }
    }
  
    //pulse width modulation (the timing of REFREH and DELAY_OFF makes the brightness level fo the LED display appear to change)
    delayMicroseconds(refresh); //turn row ON for REFRESH amount of time

    if(delayOff >  0) //only turn lights off if brightness is not 100%. If the if() is not here the lights stil turn off for a split second are are never at 100% brightness
    {
      digitalWrite(DEMUX_A, HIGH);
      digitalWrite(DEMUX_B, HIGH);
      digitalWrite(DEMUX_C, HIGH);
    
      delayMicroseconds(delayOff); //turn row OFF for DELAY_OFF amount on time before going to the next row
    }
  }

  return 0;
}

/*
    -Function Name:       updateTemp
    -Purpose:             updates the temperature after a certain amount of time
    -Input(s):        Variable Name   Variable Type   Purpose
                          userCMD       String          read user input from main loop
                          tC            int             get the celcius temp from the main loop
                          aC            int             get the average celsius temp from the main loop
                          tF            int             get the fahrenheit temp from the main loop
                          c1            int             get the light level from config1
                          c2            int             get light level from config2
                          oC            int             read the old C temp from main loop
                          oaC           int             read the old avg C temp from the main loop
                          oF            int             read the old F temp from the main loop
                          oc1           int             read old light level from config1
                          oc2           int             read old light level from config2
    -Output(s):       Variable Name   Variable Type   Purpose
                          stringText    String          update string to display on LED array
    Algorithm:            compares old temps to new temps updates the temp if it is different
*/
String updateSensors(String userCMD, int tC, int aC, int tF, int c1, int c2, int oC, int oaC, int oF, int oc1, int oc2)
{
  //LOCAL VARIABLE DECLARATION AND INITIALIZATION
  String stringText = "";
  
  if(userCMD == "1" && oC != tC)
  {
      Serial.println("The updated temperature in Celsius is " + String(tC));
      stringText = "C:" + String(tC);
  }

  else if(userCMD == "2" && oaC != aC)
  {
    Serial.println("The updated average temperature in Celsius is " + String(aC));
    stringText = "Cav:" + String(aC);
  }

  else if(userCMD == "3" && oF != tF)
  {
    Serial.println("The updated temperature in Fahrenheit is " + String(tF));
    stringText = "F:" + String(tF);
  }

  else if(userCMD == "4" && oc1 != c1)
  {
    Serial.println("The updated light level is " + String(c1));
    stringText = "lvl" + String(c1);
  }

  else if(userCMD == "5" && oc2 != c2)
  {
    Serial.println("The updated light level is " + String(c2));
    stringText = "lv" + String(c2);     
  }

  return stringText;
}




  
