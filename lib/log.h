

#include <ostream>
class Logger {
private:
  std::ostream &dest;

public:
  Logger(std::ostream &os) : dest(os) {};
  void log(const std::string &msg);
};

extern Logger logger;
