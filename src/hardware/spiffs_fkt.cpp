#include <Arduino.h>
#include "SPIFFS.h"


// Initialize SPIFFS
bool initSPIFFS()
{
  if (!SPIFFS.begin(true))
  {
    return false;
  }
  return true;
}

// Read File from SPIFFS
String readFile(fs::FS &fs, const char *path)
{
  File file = fs.open(path);
  if (!file || file.isDirectory())
  {
    return String();
  }
  String fileContent;
  while (file.available())
  {
    fileContent = file.readStringUntil('\n');
    break;
  }
  return fileContent;
}

// Write file to SPIFFS
bool writeFile(fs::FS &fs, const char *path, const char *message)
{
  File file = fs.open(path, FILE_WRITE);
  if (!file)
  {
    return false;
  }
  if (file.print(message))
  {
    return true;
  }
  else
  {
    return false;
  }
}