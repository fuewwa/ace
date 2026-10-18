#ifndef ACE_CONFIG_H_
#define ACE_CONFIG_H_

#include <string>
#include <vector>

std::string GetConfigPath();
void EnsureConfigExists();
std::string GetModifierValue();
std::vector<std::string> GetBindLines();
std::vector<std::string> GetDefaultBindLines();
std::vector<std::string> GetAutostartCommands();

#endif  // ACE_CONFIG_H_
