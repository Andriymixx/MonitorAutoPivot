// config_util.h
#ifndef CONFIG_UTIL_H
#define CONFIG_UTIL_H
#include "g_flags.h"
bool SaveConfigJson(const std::string& filename = "config.json");
bool LoadConfigJson(const std::string& filename = "config.json");

#endif // !CONFIG_UTIL_H