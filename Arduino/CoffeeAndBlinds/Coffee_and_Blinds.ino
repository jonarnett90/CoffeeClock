/*
 * This script turns a continuos rotation servo to open blinds and switches a relay to start a coffee maker 
 * at times specified in a weekly schedule. Additionally, it will display the time on an LCD screen.
 *
 * Author: Jonathan Arnett
 * Modified: 05/08/2013
 *
 * Pins:
 *  Day      6
 *  Hour     7
 *  Minute   8
 *  Control  9
 *  Servo    10
 *  Relay    13
 *  LCD RS   12
 *  LCD EN   11
 *  LCD D4   5
 *  LCD D5   4
 *  LCD D6   3
 *  LCD D7   2
 *
 * For LCD V0, 2,200 ohms works for me (r-r-r).
 */


#include <Servo.h>
#include <SimpleTimer.h>
#include <LiquidCrystal.h>
#include "pitches.h"

// Button codes
#define DAY 1
#define HOUR 2
#define MINUTE 3

// Set pins
#define MINUTE_BUTTON 6
#define HOUR_BUTTON 7
#define DAY_BUTTON 8
#define COFFEE_BUTTON 9
#define BLINDS 10
#define RELAY 13

// Set blinds constants
#define OPEN 0
#define CLOSE 180
#define STOP 91
#define BLINDS_TIME 5000

// Delays for button pushing
#define DOUBLE_BUTTON_PAUSE 100
#define DEBOUNCE 250

// String used to clear a line of the LCD
const String clearString = "                ";

// Initialize array of month names
const String months[] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};

// Initialize array for days in each month
const int monthDays[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};

// Initialize days to an array of days of the week
const String days[] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};

// Times of each day at which coffee should be brewed
const String startTimes[] = {"10:00", "09:20", "08:10", "08:10", "08:10", "08:10", "10:00"};

// Times of each day to open the blinds
const String openBlindsTimes[] = {"09:50", "09:10", "08:00", "08:00", "08:00", "08:00", "09:50"};

// Time to close blinds each day
const String closeBlindsTime = "21:00";

// initialize the library with the numbers of the interface pins
LiquidCrystal lcd(12, 11, 5, 4, 3, 2);

// Initialize blinds servo
Servo blindsServo;

SimpleTimer timer;

// Initialize time units
int month = 0;
int monthDay = 1;
int weekDay = 0;
int hour = 0;
int minute = 0;

// Initialize time strings
String timeString = "";
String lastBrewString = "Never";

// Initialize brewing boolean
boolean brewing = false;

void setup() {
  // Start LCD
  lcd.begin(16, 2);
  
  // Set the pin modes for buttons and relay
  pinMode(DAY_BUTTON, INPUT);
  pinMode(HOUR_BUTTON, INPUT);
  pinMode(MINUTE_BUTTON, INPUT);
  pinMode(COFFEE_BUTTON, INPUT);
  pinMode(RELAY, OUTPUT);
  
  // Attach the blinds servo
  blindsServo.attach(BLINDS);
  
  // Update time every minute
  timer.setInterval(60000, updateTime);  // 60,000 milliseconds per minute
  
  // Check time and print to LCD
  checkAndDisplay();
  
  // Relay is set to LOW by default, so stop it
  stopBrew();
  
  // Make sure the servo isn't turning
  blindsServo.write(STOP);
}

void loop() {
  // Needed for timer to work
  timer.run();
  
  // Initialize var to hold button push info
  int buttonCode = 0;
  
  // Find pressed button, if any
  if (digitalRead(DAY_BUTTON)) {
    buttonCode = DAY;
  } else if (digitalRead(HOUR_BUTTON)) {
    buttonCode = HOUR;
  } else if (digitalRead(MINUTE_BUTTON)) {
    buttonCode = MINUTE;
  }
  
  // If the hour button is being pushed, increment
  // the hour and check and display the time
  if (buttonCode != 0) {
    // Wait for possible second button press 
    delay(DOUBLE_BUTTON_PAUSE);
    
    // Check button combos
    switch (buttonCode) {
      // Increment weekday, month, or day of the month based
      // on button combination
      case DAY:
        if (digitalRead(MINUTE_BUTTON)) {
          weekDay++;
        } else if (digitalRead(HOUR_BUTTON)) {
          month++;
        } else {
          monthDay++; 
        }
        
        break;
        
      // Increment month if day is also pressed
      // Otherwise, increment hour
      case HOUR:
        if (digitalRead(DAY_BUTTON)) {
          month++;
        } else {
          hour++;
        }
        
        break;
        
      // Increment weekday if day is also pressed
      // Otherwise, increment minute
      case MINUTE:
        if (digitalRead(DAY_BUTTON)) {
          weekDay++;
        } else {
          minute++;
        }
        
        break;
    }
    
    // Display new time
    checkAndDisplay();
    
    // Prevent DEBOUNCE
    delay(DEBOUNCE - DOUBLE_BUTTON_PAUSE);
  }
  
  // If the force coffee pin is pushed, toggle brew
  if (digitalRead(COFFEE_BUTTON)) {
    if (brewing) {
     stopBrew();
    } else {
     brew();
    }
    
    delay(DEBOUNCE);
  }
}

/**
 * Check if coffee should be made based on the current time
 */
void checkMakeCoffee() {
  // If the time matches for today, make coffee
  if (timeString.equals(startTimes[weekDay])) {
    brew();
  }
  
  if (timeString.equals(openBlindsTimes[weekDay])) {
    open_blinds();
  }
  
  if (timeString.equals(closeBlindsTime)) {
    close_blinds();
  }
}

/**
 * Turns the servo to open the blinds
 */
void open_blinds() {
  blindsServo.write(OPEN);
 
  delay(BLINDS_TIME);
  
  blindsServo.write(STOP); 
}

/**
 * Turns the servo to close the blinds
 */
void close_blinds() {
  blindsServo.write(CLOSE);
 
  delay(BLINDS_TIME);
  
  blindsServo.write(STOP); 
}

/**
 * Starts making coffee and displays a message
 */
void brew() {
  // Update brewing
  brewing = true;
  
  // Turn the relay on, turning the coffee maker on
  digitalWrite(RELAY, LOW);

  // Set lastBrewString to the current timeString
  lastBrewString = timeString;
  
  // Clear line 2 of the LCD
  lcd.setCursor(0, 1);
  lcd.print("                ");
  
  // Write the start time for the coffee on line 2 of the LCD
  lcd.setCursor(0, 1);
  lcd.print("Brew Since " + lastBrewString);
}

void stopBrew() {
  // Update brewing
  brewing = false;
  
  // Turn off the relay
  digitalWrite(RELAY, HIGH);
  
  // Clear line 2 of the LCD
  lcd.setCursor(0, 1);
  lcd.print(clearString);
  
  // Write the time of last brew on line 2 of the LCD
  lcd.setCursor(0, 1);
  lcd.print("Last Brew: " + lastBrewString);
}


/**
 * Update the time and display it
 */
void updateTime() {
  // Update minute
  minute++;
  
  // Check and display the time
  checkAndDisplay();
  
  // Check if coffee should be made
  checkMakeCoffee();
}


/**
 * Check if any time values need to be changed
 */
void checkTime() {
  // If minute is 60, update hour and minute
  if (minute == 60) {
    hour++;
    minute = 0;
  }
  
  if (hour == 24) {
    weekDay++;
    monthDay++;
    hour = 0;
  }
    
  if (weekDay == 7) {
    weekDay = 0;
  }
  
  if (monthDay > monthDays[month]) {
    month++;
    monthDay = 1;
  }
  
  if (month == 12) {
    month = 0;
  }
}

/*
 * Updates timeString to signify the surrent time
 */
void setTimeString() {
  // Create string versions of hour and minute
  String hourString = String(hour);
  String minuteString = String(minute);
  
  // Add a "0" to the front of numbers under 10
  if (hour < 10) {
    hourString = "0" + hourString;
  }
  
  // Add a "0" to the front of numbers under 10
  if (minute < 10) {
    minuteString = "0" + minuteString;
  }
  
  // Update timeString
  timeString = hourString + ":" + minuteString;
}

/**
 * Display the current time on the LCD
 */
void showTime() {
  // Display time on line 0 of the LCD
  lcd.setCursor(0, 0);
  lcd.print(clearString);
  lcd.setCursor(0, 0);
  lcd.print(days[weekDay] + " " + months[month] + " " + String(monthDay) + " " + timeString);
}

/**
 * Check time values, set the time string, and display the time
 */
void checkAndDisplay() {
  checkTime();
  setTimeString();
  showTime();
}