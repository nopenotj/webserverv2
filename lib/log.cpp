#include "log.h"

void Logger::log(const std::string &msg) { this->dest << msg << std::endl; }
