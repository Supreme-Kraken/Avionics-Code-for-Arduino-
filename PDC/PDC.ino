/* SUNSAT Parachute Deployment and Attitude Determination Software
  
   Created 22 Nov 20
   Contributors: Rory Haggart
   
   Description
   -----------
   For the Sheffield University Nova SATellite (SUNSAT) platform, this code is to be onboard the 'parachute deployment computer' (PDC).
   The PDC is an Arduino Nano serving the dual purpose of parachute deployment activities (e.g. apogee detection), and attitude determination.
*/

// the accel/gyro, barometer & micro-SD unit are on SPI, so include library for SPI commands
#include <SPI.h>
// we want the SD card library too
#include <SD.h>

/* ---------- SPI CONFIG ---------- */
// create an SPISettngs object to define the characteristics of the bus
  // the three parameters are: 1. clock input frequency, 2. MSB/LSB first, and 3. SPI mode
  // for more information, see: https://www.arduino.cc/en/reference/SPI 
// 1. altimeter & gyro/accel have max clock input freq. of 10MHz, micro-sd has 25MHz
  // to avoid reconfigs, we'll stick at 10MHz for now - see if this is fast enough
// 2. altimeter & accel/gyro, & micro-sd are MSB first 
// 3. altimeter & accel/gyro, & micro-sd are compatible with mode 00 (clock idle low, output: falling edge, capture: rising edge); 
SPISettings SPIParams(10000000, MSBFIRST, SPI_MODE0); 

// the arduino nano has an 'SS' pin which helps us choose if we want to be master or slave. pin 10 as output = PDC as master
const int PDC_SS = 10;
// define the PDC DIGIN pins that are connected to each devices 'slave select' (SS) pin.
const int altimeter_SS = 4;
const int IMU_SS = 5;
const int microSD_SS = 6;

/* ---------- I2C CONFIG ----------*/

void setup() {

  // variable to keep hold of the return value of the accelerometer 'WHO_AM_I' identification register
  unsigned int WHO_AM_I;
  
  /* ---------- Serial Setup ---------- */
  // open serial comms at 9600 baud
  Serial.begin(9600);
  while(!Serial){
    ; // wait for port to connect
  }

  /* ---------- SPI Setup ---------- */
  // we want to be the master of this bus! so set the 'SS' pin on the PDC as an output
  pinMode(PDC_SS, OUTPUT);
  
  // set each slave select pin as an output. 
    // initialise each pin to be high (i.e. communication disabled)
    // to communicate with a specific device, take its SS pin low with digitalWrite(device_SS, LOW);
  pinMode(altimeter_SS, OUTPUT);
  digitalWrite(altimeter_SS, HIGH);
  pinMode(IMU_SS, OUTPUT);
  digitalWrite(IMU_SS, HIGH);
  pinMode(microSD_SS, OUTPUT);
  digitalWrite(microSD_SS, HIGH);
  
  // Initialise all lines and CPU to use SPI
  SPI.begin();

  /* ---------- I2C Setup ---------- */

  /* ---------- SPI Verification ---------- */
  // communicate with altimeter: set CS pin high and read the 'CHIP_ID' register. expect 0x50
  
  // communicate with IMU: read the 'WHO_AM_I' register. expect 0110110
  WHO_AM_I = readIMU(0x0f, 1);

  // we have read the 'WHO_AM_I' and now should check that the value read is as we expect
  if (WHO_AM_I == 0110110) {
    Serial.println("IMU successfully connected!");
  }
  else {
    Serial.println("IMU could not be reached!");
  }
  
  // attempt to init micro SD card
  if(SD.begin(microSD_SS)) {
    Serial.println("micro-SD card initialised");
  }
  else {
    Serial.println("micro-SD initialisation failed!");
  }
  // communicate with micro SD - write the csv headers to a new file (timestamp.csv after I2C & RTC are set??)
    // if we have a shield with 'CD' (chip detect) pin, make use of this to check pin is in place.

  /* ---------- ---------- */
  // light sensor pin configuration (digital output to SI pin, analogue input(s) from AO pins, clock signal to CLK pins) 

  // setup interrupt pin? TBD - can we simply configure one of the GPIO to go high and connect this to OBC interrupt, and then execute
    // the interrupt routine on OBC?
  
  // initialise Kalman Filter (e.g. velocity = 0, altitude = 0, or whatever else)
  
  // confirm setup has succeeded? e.g. ask SPI for accel values and verify zero (check 'who am i' reg or similar)
    // then alert main OBC that PDC is setup and ready to go
    // for SD card, check the 'carddetect' functionality to see if card is in socket
}

void loop() {
  // parachute deployment tasks
    // light sensor check (poll the sensor every x seconds to check ambient light levels. If new value much greater than old on all 4 sensors,
    // register apogee)
    
    // altimeter check (check stream of readings and determine when the pressure begins to increase again). potentially feed this into the 
    // Kalman Filter to mitigate noise and back up the accel readings)
     
    // accelerometer check (update Kalman Filter of integral of acceleration to detect when the velocity is zero)

    // write data to SD card (line format with timestamped measurements)


  // attitude determination tasks
    // quaternion conversion from gyro Euler output then update kalman filter with gyro readings
    // try coarse sun sensing to determine relative pose of sun
}

// to read a register in the IMU, we pass the address of the register (as provided in the datasheet), and the size (in bytes) of the 
  // register data field
unsigned int readIMU(byte registerSelect, int numBytes) {

  // variable for our register value return
  unsigned int result = 0; 
  
  // LSB is the R/W bit, and the register then occupies the rest, so we shift the register up into place
    // then '1' in the LSB is read
  registerSelect = (registerSelect << 1) | 1;

  // begin a transaction over SPI using our params. this command also stops interrupts from preventing SPI comms
  SPI.beginTransaction(SPIParams);
  
  // to communicate with the IMU, we take its slave select pin on the PDC low
  digitalWrite(IMU_SS, LOW);

  // if we want to read a particular address, we must send the address of the register to the device
  SPI.transfer(registerSelect);

  // now if we send nothing, we are listening for the result - the device will send the value in the register we requested
  // for the first byte, just read the value into 'result'
  result = SPI.transfer(0x00);
  // decrement the number of bytes that we have left to read
  numBytes--;
  
  while(numBytes != 0) {
    // if we have more than one byte to read, shift the result so far up by a byte, and fill the empty space with our new byte
    result = (result << 8) | SPI.transfer(0x00);
    // decrement the number of bytes until we get to zero, when this while() will exit
    numBytes--;
  }

  // stop communications with IMU by setting the corresponding slave select on the PDC to high
  digitalWrite(IMU_SS, HIGH);
  
  // we're done now! restart interrupt mechanisms
  SPI.endTransaction();

  // send our address value back to the caller
  return(result);
}
