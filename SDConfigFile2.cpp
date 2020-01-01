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
#define SDCONFIGFILE_DEBUG_BASIC
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
	#ifdef SDCONFIGFILE_DEBUG_BASIC
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
	#ifdef SDCONFIGFILE_DEBUG_BASIC
		SERIAL.print("Could not open SD file: ");
		SERIAL.println(configFileName);
	#endif
    _atEnd = true;
    return false;
  }
  
  #ifdef SDCONFIGFILE_DEBUG_BASIC
  	  SERIAL.println("file opened");
  #endif

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


// will remove all occurrences of a specific character from a string
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

void SDConfigFile::setNewValue(const char *newValue) 
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

  int startingPosition = _file.position();

  // go back to the equals character
  int read;
  do
  {
    _file.seek( _file.position() - 2 );
	read = _file.read();
	
	#ifdef SDCONFIGFILE_DEBUG
	  SERIAL.print("Current: ");
	  SERIAL.print(_file.position());
	  SERIAL.println(" Seek");
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
  
  //fill rest of line to new line character with characters
  read = _file.read();

  while(read != '\n' && read != '\r' && _file.position() < startingPosition)
  {
    _file.seek( _file.position() - 1 );
    _file.write(BUFFER_MARKER);
	read = _file.read();

	#ifdef SDCONFIGFILE_DEBUG
		SERIAL.print("Current: ");
		SERIAL.print(_file.position());
		SERIAL.println(" Fill");
    #endif
  }
  
  #ifdef SDCONFIGFILE_DEBUG
	SERIAL.println("New Line");
  #endif
  
  _file.flush();
  
}

//******************************************************************************

void SDConfigFile::setValue(const char *newValue)
{
	setStringValue(newValue);
}


void SDConfigFile::setValue(bool newValue)
{
	setBooleanValue(newValue);
}


void SDConfigFile::setValue(int newValue)
{
	setIntValue(newValue);
}


void SDConfigFile::setValue(double newValue)
{
	setDoubleValue(newValue);
}


//******************************************************************************

void SDConfigFile::setStringValue(const char *newValue) 
{
	#ifdef SDCONFIGFILE_DEBUG
		SERIAL.println("SDConfigFile Set String");
	#endif
	char buffer[_lineSize];
    sprintf (buffer, "\"%s\"", newValue);
    setNewValue(buffer);
}


void SDConfigFile::setBooleanValue(bool newValue) 
{
	#ifdef SDCONFIGFILE_DEBUG
		SERIAL.println("SDConfigFile Set Bool");
	#endif
	if(newValue == true)
	{
		setNewValue("true");
	}
	else
	{
		setNewValue("false");
	}
}


void SDConfigFile::setIntValue(int newValue) 
{
	#ifdef SDCONFIGFILE_DEBUG
		SERIAL.println("SDConfigFile Set Int");
	#endif
	char stringValue[5];
	itoa(newValue, stringValue, 10);
	setNewValue( stringValue );
}


////https://stackoverflow.com/questions/23191203/convert-float-to-string-without-sprintf
//void floatToStr(char *out, float x, int decimalPoint)
//{
//    uint16_t absval = fabs(x);
//    uint16_t absvalcopy = absval;
//
//
//    int decimalcount = 0;
//
//    while(absvalcopy != 0)
//    {
//        absvalcopy /= 10;
//        decimalcount ++;
//    }
//
//    uint8_t *absbuffer = new uint8_t[sizeof(uint8_t) * (decimalcount + decimalPoint + 1)];
//    int absbufferindex = 0;
//    absvalcopy = absval;
//    uint8_t temp;
//
//    int i = 0;
//    for(i = decimalcount; i > 0; i--)
//    {
//        uint16_t frst1 = fabs((absvalcopy / pow(10.0, i-1)));
//        temp = (frst1 % 10) + 0x30;
//        *(absbuffer + absbufferindex) = temp;
//        absbufferindex++;
//    }
//
//    if(decimalPoint > 0)
//    {
//        *(absbuffer + absbufferindex) = '.';
//        absbufferindex ++;
//
//        //------------------- Decimal Extractor ---------------------//
//       for(i = 1; i < decimalPoint + 1; i++)
//       {
//
//           uint32_t valueFloat = (x - (float)absval)*pow(10,i);
//           *(absbuffer + absbufferindex) = ((valueFloat) % 10) + 0x30;
//           absbufferindex++;
//       }
//    }
//
//   for(i=0; i< (decimalcount + decimalPoint + 1); i++)
//   {
//       *(out + i) = *(absbuffer + i);
//   }
//
//   i=0;
//   if(decimalPoint > 0)
//       i = 1;
//   *(out + decimalcount + decimalPoint + i) = 0;
//
//}
//

//http://www.onarm.com/forum/17514/
void ftoa(char *str, int arraySize, float num, int precision)
{
  int intpart = num;
  int intdecimal;
  int i;
  float decimal_part;
  char decimal[arraySize];

  memset(str, 0x0, arraySize);
  itoa(num, str, 10);

  strcat(str, ".");

  decimal_part = num - intpart;
  intdecimal = decimal_part * pow(10, precision);

  if(intdecimal < 0)
  {
    intdecimal = -intdecimal;
  }
  itoa(intdecimal, decimal, 10);
  for(i=0; i < (precision - strlen(decimal)); i++)
  {
    strcat(str, "0");
  }
  strcat(str, decimal);
}

void SDConfigFile::setDoubleValue(double newValue)
{
	char stringValue[20];
	ftoa(stringValue, 20, newValue, 6);

	#ifdef SDCONFIGFILE_DEBUG
		SERIAL.print("SDConfigFile Set Double: ");
	#endif

	setNewValue( stringValue );
}




//******************************************************************************

void SDConfigFile::getValue(char* updateValue)
{
  const char *temp = getCleanString();
  
  strcpy(updateValue, temp);
}

void SDConfigFile::getValue(char** updateValue)
{
  const char *temp = getCleanString();

  strcpy(*updateValue, temp);
}

// Returns the value part of the most-recently-read setting as a boolean
// The value "true" corresponds to true
// all other values correspond to false
void SDConfigFile::getValue(boolean* updateValue)
{
  if( strcmp("true", getCleanValue()) == 0) 
  {
	  *updateValue = true;
  }
  else
  {
	  *updateValue = false;
  }
}


// Returns the value part of the most-recently-read setting as an integer, or 0 if an error occurred
void SDConfigFile::getValue(int* updateValue)
{
  const char *str = getCleanValue();
  if (!str) 
  {
	  *updateValue = 0;
  }
  else
  {
	  *updateValue = atoi(str);
  }
}

// Returns the value part of the most-recently-read setting as an integer, or 0 if an error occurred
void SDConfigFile::getValue(double* updateValue)
{
  const char *str = getCleanValue();
  if (!str)
  {
	  *updateValue = 0;
  }
  else
  {
	  *updateValue = atof(str);
  }
}


//******************************************************************************

// Returns a persistent, dynamically-allocated copy of the value part
// of the most-recently-read setting, or a error string if a failure occurred.
// 
// Unlike getValue(), the return value of this function
// persists after readNextSetting() is called or end() is called.
//
// this function is not reccomendeed! dynamic allocation in a
// embedded system will come back and byte you in the butt!
//
char *SDConfigFile::getStringValue() 
{
  const char *temp = getCleanString();
  char *result = 0;
  int length;

  if (_lineLength <= 0 || _valueIdx <= 1)
  {
    return "Not init"; // begin() wasn't called, or failed.
  }

  length = strlen(temp);
  result = (char *) malloc(length + 1);
  if (result == 0)
  {
    return "Out Of Memmory"; // out of memory
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


double SDConfigFile::getDoubleValue()
{
  const char *str = getCleanValue();
  if (!str)
  {
    return 0;
  }
  return atof(str);
}
