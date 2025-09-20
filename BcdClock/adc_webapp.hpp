#ifndef ADC_WEBAPP_HPP
#define ADC_WEBAPP_CPP

#include "../WebServer/webserver.hpp"

class AdcWebapp : public WebApp
{
    static char* id(char* pos, const char* name);
    static char* value(char* pos, uint64_t value);
public:
    virtual bool matches(const char *verb, const char *path);
    virtual void process(HttpRequest &request, HttpResponse &response);
};

#endif