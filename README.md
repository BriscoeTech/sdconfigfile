# SDConfigFile2

## Introduction

SDConfigFile2 is an Arduino library to read saved settings from a configuration file on an SD card.

This library is based of off bneedhamia original SDConfigFile library. This library expands upon the origional, but adds the ability to both read and write settings to the sd card!


*************************************************************************************************************

##### BriscoeTech comments:

I love the SAMD21, but have realized that not having eeprom has made afew of my projects more difficult. I have been making puzzle boxes and laser tag guns, and they need to save information away for the next time they are turned back on.  

With the new additions, the config file acts as a non volatile buffer, ready to be read and rewritten.



##### bneedhamia comments:
I wrote the library so that I didn't have to hard-code private WiFi settings in my Sketches.  Instead, I have a config.cfg file on an SD card, that contains things like:

    ssid=wickedOz
    password=flyingMonkeys

Now I can change the WiFi settings, or any other configuration, by simply editing the SD card's **config.cfg** file.


*************************************************************************************************************

## To use

See **examples/SDConfigFileExample2** for a full example Sketch and its .cfg file.  See readConfiguration() in that sketch for the code that reads the settings from the .cfg file.

The basic flow of reading a configuration file is:

    #include <SD.h>
    #include <SDConfigFile.h>

    SDConfigFile cfg;
    
    SD.begin(...);
    cfg.begin(...);
    while (cfg.readNextSetting()) 
	{
      if (cfg.nameIs("mySetting1")) 
	  {
			   call cfg.copyValue(),
            cfg.getBooleanValue(), or
            cfg.getIntValue(), as appropriate to read the settings;
			
			   call setStringValue(), 
			      setBooleanValue(), or 
			      setIntValue, as appropriate to updated that setting;
      }
      ...do the same for the other setting names.
    }
    cfg.end();
