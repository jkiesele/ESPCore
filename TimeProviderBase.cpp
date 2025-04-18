
#include <TimeProviderBase.h>

TimeProviderBase* gTimeProvider;
NullTimeProvider gNullTimeProvider;

void setTimeProvider(TimeProviderBase* timeProvider) {
    if(timeProvider == 0)
         gTimeProvider = &gNullTimeProvider;
    gTimeProvider = timeProvider;
}