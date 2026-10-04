#include "log.h"
#include <iostream>

Logger logger(std::cout);
void Logger::log(const std::string &msg) { this->dest << msg << std::endl; }
