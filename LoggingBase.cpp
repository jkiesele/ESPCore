#include <LoggingBase.h>

// One instance of each backend
static SerialLogging  _serialLogger;
static NullLogging    _nullLogger;

// Default logger is Serial
LoggingBase* gLogger = &_serialLogger;
void setLogger(LoggingBase* logger) {
    gLogger = logger ? logger : &_nullLogger;
}
