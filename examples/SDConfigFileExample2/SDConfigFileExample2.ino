#include <SD.h>
#include <SDConfigFile2.h>

/*
Example use of the SDConfigFile2 library.
This sketch reads the configuration file from the SD card,
then prints the configuration and prints a hello message
at a rate given in the configuration file.
*/
 
/*
To operate:
1) format your SD card.
2) copy the examples/SDConfigFileExample/example.cfg file to the SD card.
3) Download and run this Sketch
4) Open the SERIAL Monitor at 115200 baud.
See that the SERIAL Monitor shows the settings
and the greeting, and the hello message is printed
with the timing given in the configuration file.
Change the example.cfg file contents and see how the
hello message and timing change - without downloading
a new version of this sketch.
*/

//Make sure that pinSelectSD (below) is correct for the SD card you're using.
const int pinSelectSD = 4; // SD shield Chip Select pin.

// The filename of the configuration file on the SD card
const char CONFIG_FILE[] = "example.cfg";

/*
Settings we read from the configuration file.
	didReadConfig = true if the configuration-reading succeeded; false otherwise.
    Used to prevent odd behaviours if the configuration file  corrupt or missing.
   hello = the "hello world" string, allocated via malloc().
   doDelay = if true, delay waitMs in loop(). if false, don't delay.
   waitMs = time (milliseconds) to wait after printing hello.
    
   each time the sketch is run, the sketch will write new settings to the config file. 
   1) the hello string will be changed
   2) doDelay will go from true, to false or false to true
   3) waitMs will be incremented by one
*/


//************************************************************************************


boolean didReadConfig;
char *hello;
boolean doDelay;
int waitMs;

// which Serial port to communicate over
#define SERIAL SerialUSB //for usb devices like SAMD21
//#define SERIAL Serial  //for uart devices like UNO

void setup() 
{
  SERIAL.begin(115200);
  delay(1000);
  while (!SERIAL); // Wait for serial terminal to open port before starting program, only needed for controlled debugging
  
  pinMode(pinSelectSD, OUTPUT);

  didReadConfig = false;
  hello = 0;
  doDelay = false;
  waitMs = 0;
  
  
  // Setup the SD card 
  SERIAL.println("Calling SD.begin()...");
  if (!SD.begin(pinSelectSD)) 
  {
    SERIAL.println("SD.begin() failed. Check: ");
    SERIAL.println("  card insertion,");
    SERIAL.println("  SD shield I/O pins and chip select,");
    SERIAL.println("  card formatting.");
    return;
  }
  SERIAL.println("...succeeded.");

  // Read our configuration from the SD card file.
  didReadConfig = readConfiguration();
}

void loop() 
{


  //If we didn't read the configuration, do nothing.
  if (!didReadConfig) 
  {
    return;
  }

  // print the hello message, then wait the configured time.
  if (hello) 
  {

    SERIAL.println(hello);
    if (doDelay) 
    {
      delay(waitMs);
    }
    else
    {
      delay(1000);
    }

  }
  
}

// Read our settings from our SD configuration file.
// Returns true if successful, false if it failed.
boolean readConfiguration() 
{
  /*
   * Length of the longest line expected in the config file.
   * The larger this number, the more memory is used
   * to read the file.
   * You probably won't need to change this number.
   */
  const uint8_t CONFIG_LINE_LENGTH = 64;
  
  // The open configuration file.
  SDConfigFile cfg;
  
  // Open the configuration file.
  if (!cfg.begin(CONFIG_FILE, CONFIG_LINE_LENGTH)) 
  {
    SERIAL.print("Failed to open configuration file: ");
    SERIAL.println(CONFIG_FILE);
    return false;
  }

  SERIAL.println("Reading Settings...");
  
  // Read each setting from the file.
  while (cfg.readNextSetting())
  {
    
    // Put a nameIs() block here for each setting you have.

    // doDelay
    if (cfg.nameIs("doDelay")) 
    {
        doDelay = cfg.getBooleanValue();
        SERIAL.print("Read doDelay: ");
        if (doDelay) 
        {
          SERIAL.println("true");
        } 
        else 
        {
          SERIAL.println("false");
        }

        // save this inverted value to the config file
        // will load this the next time the program is run
        cfg.setBooleanValue( !doDelay );
    } 
    // waitMs integer
    else if (cfg.nameIs("waitMs")) 
    {
      waitMs = cfg.getIntValue();
      SERIAL.print("Read waitMs: ");
      SERIAL.println(waitMs);

      // increment the time by one and save it to the config file
      // will load this the next time the program is run
      cfg.setIntValue(waitMs + 1);
    } 
    // hello string (char *)
    else if (cfg.nameIs("hello")) 
    {
      // Dynamically allocate a copy of the string.
      hello = cfg.getStringValue();
      SERIAL.print("Read hello: ");
      SERIAL.println(hello);


      // write a new string to the config file
      // will load this the next time the program is run
      int x = waitMs % 3;
      if(x == 0)
      {
        cfg.setStringValue("All Your Base Are Belong To Us...");
      }
      else if(x == 1)
      {
        cfg.setStringValue("Howdy Ho Neighbor");
      }
      else if(x == 2)
      {
        cfg.setStringValue("Minors not Miners");
      }
      
      
    }
    else 
    {
      // report unrecognised names.
      SERIAL.print("Unknown line in config: ");
   
      SERIAL.print(cfg.getName());
      SERIAL.print(" = ");
      SERIAL.println(cfg.getRawValue());
    }
    
  }

  SERIAL.println("End Of File");
  
  // clean up
  cfg.end();
  
  return true;
}

