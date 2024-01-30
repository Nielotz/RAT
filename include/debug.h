#ifndef DEBUG_H
#define DEBUG_H

#include <string>

#define DEBUG_USER_ACTION_MSG(serial, msg) DEBUG_PRINTER(serial.println, "[ACTION REQUIRED] ", msg)
#define DEBUG_MSG(serial, msg) DEBUG_PRINTER(serial.println, String("[") + __FILE__ + ":" + __LINE__ + "] ", msg)

#define DEBUG_PRINTER(print_function, header, msg) print_function((\
String(header) + msg\
).c_str())

#endif //DEBUG_H
