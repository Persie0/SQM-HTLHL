#ifndef SPIFFS_FKT_H
#define SPIFFS_FKT_H
bool initSPIFFS();
String readFile(fs::FS &fs, const char *path);
bool writeFile(fs::FS &fs, const char *path, const char *message);
#endif