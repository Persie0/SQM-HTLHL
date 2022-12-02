#include <Arduino.h>
#include "SPIFFS.h"
#include "FS.h"
#include "settings.h"

// Initialize SPIFFS
bool initSPIFFS()
{
  if (!SPIFFS.begin(FORMAT_SPIFFS_IF_FAILED))
  {
    return false;
  }
  return true;
}

// Read File from SPIFFS
String readLineOfFile(fs::FS &fs, const char *path)
{
  File file = fs.open(path);
  if (!file || file.isDirectory())
  {
    file.close();
    return "";
  }
  String fileContent;
  if (file.available())
  {
    fileContent = file.readStringUntil('\n');
  }
  file.close();
  return fileContent;
}

// Write file to SPIFFS
bool writeLineOfFile(fs::FS &fs, const char *path, const char *message)
{
  bool ret;
  File file = fs.open(path, FILE_WRITE);
  if (!file)
  {
    ret= false;
  }
  else if (file.print(message))
  {
    ret= true;
  }
  else
  {
    ret= false;
  }
  file.close();
  return ret;
}