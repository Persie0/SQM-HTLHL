#ifndef SPIFFS_FKT_H
#define SPIFFS_FKT_H
bool initSPIFFS();
String readLineOfFile(fs::FS &fs, const char *path);
bool writeLineOfFile(fs::FS &fs, const char *path, const char *message);
#endif