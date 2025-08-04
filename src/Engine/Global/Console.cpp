#include "Console.h"

std::vector<std::string> gbe::Console::logs;
std::vector < std::function<void(std::string)>> gbe::Console::subscribed_delegates;