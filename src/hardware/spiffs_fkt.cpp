#include <Arduino.h>
#include "SPIFFS.h"
#include "FS.h"
#include "settings.h"

// Initialize SPIFFS
bool initSPIFFS()
{
  // Initialize SPIFFS, if it fails, format it if FORMAT_SPIFFS_IF_FAILED is true
  if (!SPIFFS.begin(FORMAT_SPIFFS_IF_FAILED))
  {
    return false;
  }
  return true;
}

// Read the a line of the file from the passed path and return the content as a String
String readLineOfFile(fs::FS &fs, const char *path)
{
  File file = fs.open(path);
  // If the file doesn't exist, return an empty String
  if (!file || file.isDirectory())
  {
    file.close();
    return "";
  }
  String fileContent;
  // Read the file until new line
  if (file.available())
  {
    fileContent = file.readStringUntil('\n');
  }
  file.close();
  // Return the file content
  return fileContent;
}

// Write passed message to file of passed path
bool writeLineOfFile(fs::FS &fs, const char *path, const char *message)
{
  bool ret;
  File file = fs.open(path, FILE_WRITE);
  // If the file doesn't exist, return false
  if (!file)
  {
    ret= false;
  }
  // if the file exists, write the message to it
  else if (file.print(message))
  {
    ret= true; // return true if the message was written
  }
  else
  {
    ret= false; // return false if the message wasn't written
  }
  file.close();
  // return whether the message was written or not
  return ret;
}