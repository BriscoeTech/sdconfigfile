/*
 * SD card configuration file reading library
 *
 * Copyright (c) 2014 Bradford Needham
 * (@bneedhamia, https://www.needhamia.com )
 * Licensed under LGPL version 2.1
 * a version of which should have been supplied with this file.
 */
 
#include <SDConfigFile2.h>

//#define SDCONFIGFILE_DEBUG
#define SERIAL SerialUSB

/*
 * Opens the given file on the SD card.
 * Returns true if successful, false if not.
 *
 * configFileName = the name of the configuration file on the SD card.
 *
 * NOTE: SD.begin() must be called before calling our begin().
 */
boolean SDConfigFile::begin(const char *configFileName, uint8_t maxLineLength) 
{
  _lineLength = 0;
  _lineSize = 0;
  _valueIdx = -1;
  _atEnd = true;

  /*
   * Allocate a buffer for the current line.
   */
  _lineSize = maxLineLength + 1;
  _line = (char *) malloc(_lineSize);
  if (_line == 0) 
  {
	#ifdef SDCONFIGFILE_DEBUG
		SERIAL.println("out of memory");
	#endif
    _atEnd = true;
    return false;
  }

  /*
   * To avoid stale references to configFileName
   * we don't save it. To minimize memory use, we don't copy it.
   */
   
  _file = SD.open(configFileName, FILE_WRITE);
  if (!_file) 
  {
	#ifdef SDCONFIGFILE_DEBUG
		SERIAL.print("Could not open SD file: ");
		SERIAL.println(configFileName);
	#endif
    _atEnd = true;
    return false;
  }
  
  // seek back to the beginning of the file
  // write mode starts at end of file
  _file.seek(0);
  
  // Initialize our reader
  _atEnd = false;
  
  return true;
}

/*
 * Cleans up our SDCOnfigFile object.
 */
void SDConfigFile::end() 
{
  if (_file) 
  {
    _file.close();
  }

  if (_line != 0) 
  {
    free(_line);
    _line = 0;
  }
  _atEnd = true;
}

/*
 * Reads the next name=value setting from the file.
 * Returns true if the setting was successfully read,
 * false if an error occurred or end-of-file occurred.
 */
boolean SDConfigFile::readNextSetting() 
{

  int bint;
  boolean foundQuote = false;
  
  if (_atEnd) 
  {
    return false;  // already at end of file (or error).
  }
  
  _lineLength = 0;
  _valueIdx = -1;
  
  // Assume beginning of line.
  // Skip blank and comment lines
  // until we read the first character of the key
  // or get to the end of file.
  while (true) 
  {
    bint = _file.read();
	
    if (bint < 0) 
	{
      _atEnd = true;
      return false;
    }
    
	// Comment line.
    if ((char) bint == '#') 
	{
      // Read until end of line or end of file.
      while (true) 
	  {
        bint = _file.read();
        if (bint < 0) 
		{
          _atEnd = true;
          return false;
        }
		
        if ((char) bint == '\r' || (char) bint == '\n') 
		{
          break;
        }
      }
      continue; // look for the next line.
    }
    
    // Ignore line ends and blank text
    if ((char) bint == '\r' || (char) bint == '\n' || (char) bint == ' ' || (char) bint == '\t') 
	{
      continue;
    }
        
    break; // bint contains the first character of the name
  }
  
  // Copy from this first character to the end of the line.

  while (bint >= 0 && (char) bint != '\r' && (char) bint != '\n') 
  {
    if (_lineLength >= _lineSize - 1) // -1 for a terminating null.
	{ 
		_line[_lineLength] = '\0';
		#ifdef SDCONFIGFILE_DEBUG
			SERIAL.print("Line too long: ");
			SERIAL.println(_line);
		#endif
		_atEnd = true;
		return false;
    }
	
    // End of Name; the next character starts the value.
    if ((char) bint == '=') 
	{
      _line[_lineLength++] = '\0';
      _valueIdx = _lineLength;
	  
	  //_valueIdxFile = _file.position();
	  
	  //#ifdef SDCONFIGFILE_DEBUG
	//	SERIAL.print("Current: ");
	//	SERIAL.println(_valueIdxFile);
	  //#endif
	  
    } 
	// found the start/end of a string
	else if ((char) bint == '"') 
	{
      foundQuote = !foundQuote;
    } 
	// found a space or tab not in a string
	else if ( ( (char)bint == ' ' || (char)bint == '\t' ) && !foundQuote) 
	{
      // do nothing, eliminate white space
    } 
	else 
	{
      _line[_lineLength++] = (char) bint;
    }
    
    bint = _file.read();
  }
  
  
  if (bint < 0) 
  {
    _atEnd = true;
    // Don't exit. This is a normal situation:
    // the last line doesn't end in newline.
  }
  
  _line[_lineLength] = '\0';
  
  // Sanity checks of the line:
  //   No =
  //  No name
  // It's OK to have a null value (nothing after the '=')
  if (_valueIdx < 0) 
  {
	#ifdef SDCONFIGFILE_DEBUG
		SERIAL.print("Missing '=' in line: ");
		SERIAL.println(_line);
	#endif
	
    //_atEnd = true;
    //return false;
  }
  else if (_valueIdx == 1) 
  {
	#ifdef SDCONFIGFILE_DEBUG
		SERIAL.print("Missing Name in line: ");
		SERIAL.println(_line);
	#endif
    //_atEnd = true;
    //return false;
  }
  
  // Name starts at _line[0]; Value starts at _line[_valueIdx].
  return true;
  
}


//******************************************************************************


// will remove all occurances of a specific character from a string
void SDConfigFile::remove_chars(char* str, char c) 
{
    char *pr = str, *pw = str;
    while (*pr) 
	{
        *pw = *pr++;
        pw += (*pw != c);
    }
    *pw = '\0';
}


//******************************************************************************


/*
 * Returns true if the most-recently-read setting name
 * matches the given name, false otherwise.
 */
boolean SDConfigFile::nameIs(const char *name) 
{
  if (strcmp(name, _line) == 0) 
  {
    return true;
  }
  return false;
}

//******************************************************************************


// // returns the last line that was read from the file
// const char *SDConfigFile::getRawLine() 
// {
  // return _line;
// }


/*
 * Returns the name part of the most-recently-read setting.
 * or null if an error occurred.
 * WARNING: calling this when an error has occurred can crash your sketch.
 */
const char *SDConfigFile::getName() 
{
  //if (_lineLength <= 0 || _valueIdx <= 1) 
  //{
  //  return 0;
  //}
  return &_line[0];

}

/*
 * Returns the value part of the most-recently-read setting,
 * or null if there was an error.
 * WARNING: calling this when an error has occurred can crash your sketch.
 */
const char *SDConfigFile::getRawValue() 
{
  if (_lineLength <= 0 || _valueIdx <= 1) 
  {
    return "???";
  }
  return &_line[_valueIdx];

}


const char *SDConfigFile::getCleanValue() 
{

  #ifdef SDCONFIGFILE_DEBUG
	SERIAL.print("Raw: ");
    SERIAL.println(&_line[_valueIdx]);
  #endif

  remove_chars(&_line[_valueIdx], ' '); //make sure all spaces have been removed
  remove_chars(&_line[_valueIdx], BUFFER_MARKER); //make sure all buffer markers removed

  #ifdef SDCONFIGFILE_DEBUG
	SERIAL.print("Clean: ");
    SERIAL.println(&_line[_valueIdx]);
  #endif
  
  
  if (_lineLength <= 0 || _valueIdx <= 1) 
  {
    return 0;
  }
  return &_line[_valueIdx];

}

const char *SDConfigFile::getCleanString() 
{

  #ifdef SDCONFIGFILE_DEBUG
	SERIAL.print("Raw: ");
    SERIAL.println(&_line[_valueIdx]);
  #endif

  remove_chars(&_line[_valueIdx], '"'); //make sure all quotes have been removed
  remove_chars(&_line[_valueIdx], BUFFER_MARKER); //make sure all buffer markers removed

  #ifdef SDCONFIGFILE_DEBUG
	SERIAL.print("Clean: ");
    SERIAL.println(&_line[_valueIdx]);
  #endif
  
  
  if (_lineLength <= 0 || _valueIdx <= 1) 
  {
    return 0;
  }
  return &_line[_valueIdx];

}


//******************************************************************************


void SDConfigFile::setValue(const char *newValue) 
{
  
  #ifdef SDCONFIGFILE_DEBUG
	//SERIAL.print("Seek: ");
    //SERIAL.println(_valueIdxFile);
	SERIAL.print("Current: ");
    SERIAL.println(_file.position());
	
    SERIAL.print("String: ");
    SERIAL.println(newValue);
    SERIAL.print("String Length: ");
    SERIAL.println(strlen(newValue));
  #endif

  // go back to the equals character
  int read;
  do
  {
    _file.seek( _file.position() - 2 );
	read = _file.read();
	
	#ifdef SDCONFIGFILE_DEBUG
	  SERIAL.print("Seek,");
    #endif
  }
  while(read != '=');
  
  // move one space after equal sign
  _file.write(' ');
  
  //write the new value to the line
  int written = 0;
  written = _file.write(newValue, strlen(newValue));
  
  #ifdef SDCONFIGFILE_DEBUG
	SERIAL.print("Written Bytes: ");
	SERIAL.println(written);
  #endif
  
  //fill rest of line to new line character with spaces
  read = _file.read();
  
  while(read != '\n' && read != '\r')
  {
    _file.seek( _file.position() - 1 );
    _file.write(BUFFER_MARKER);
	read = _file.read();
	
	#ifdef SDCONFIGFILE_DEBUG
		SERIAL.print("Fill,");
    #endif
  }
  
  #ifdef SDCONFIGFILE_DEBUG
	SERIAL.println("New Line");
  #endif
  
  _file.flush();
  
}


void SDConfigFile::setStringValue(const char *newValue) 
{
	char buffer[_lineSize];
    sprintf (buffer, "\"%s\"", newValue);
	setValue(buffer);
}


void SDConfigFile::setBooleanValue(bool newValue) 
{
	if(newValue == true)
	{
		setValue("true");
	}
	else
	{
		setValue("false");
	}
}


void SDConfigFile::setIntValue(int newValue) 
{
	char stringValue[5];
	itoa(newValue, stringValue, 10);
	
	#ifdef SDCONFIGFILE_DEBUG
	  SERIAL.print("New Int: ");
	  SERIAL.println(stringValue);
    #endif
	
	setValue( stringValue );
}


//******************************************************************************


// Returns a persistent, dynamically-allocated copy of the value part
// of the most-recently-read setting, or null if a failure occurred.
// 
// Unlike getValue(), the return value of this function
// persists after readNextSetting() is called or end() is called.
char *SDConfigFile::getStringValue() 
{
  const char *temp = getCleanString();
  char *result = 0;
  int length;

  //if (_lineLength <= 0 || _valueIdx <= 1) 
  //{
  //  return 0; // begin() wasn't called, or failed.
  //}

  length = strlen(temp);
  result = (char *) malloc(length + 1);
  if (result == 0) 
  {
    return 0; // out of memory
  }
  
  strcpy(result, temp);

  return result;
}


// Returns the value part of the most-recently-read setting as a boolean
// The value "true" corresponds to true
// all other values correspond to false
boolean SDConfigFile::getBooleanValue() 
{
  if( strcmp("true", getCleanValue()) == 0) 
  {
    return true;
  }
  return false;
}


// Returns the value part of the most-recently-read setting as an integer, or 0 if an error occurred
int SDConfigFile::getIntValue() 
{
  const char *str = getCleanValue();
  if (!str) 
  {
    return 0;
  }
  return atoi(str);
}