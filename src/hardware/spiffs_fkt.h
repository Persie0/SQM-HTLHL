#ifndef SPIFFS_FKT_H
#define SPIFFS_FKT_H
#include "FS.h"

/// @brief Initialize the SPIFFS file system
/// @return true if the SPIFFS file system was initialized successfully, false otherwise
bool initSPIFFS();

/// @brief Read the specified file from the SPIFFS file system
/// @param fs the SPIFFS file system
/// @param path the path to the file
/// @return the content of the file
String readLineOfFile(fs::FS &fs, const char *path);

/// @brief Write the specified message to the specified file in the SPIFFS file system
/// @param fs the SPIFFS file system
/// @param path the path to the file
/// @param message the message to write to the file
bool writeLineOfFile(fs::FS &fs, const char *path, const char *message);
#endif