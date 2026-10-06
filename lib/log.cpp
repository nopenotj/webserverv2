#include "log.h"
#include <iostream>

Logger logger(std::cout);
void Logger::info(const std::string &msg) { this->dest << msg << std::endl; }
