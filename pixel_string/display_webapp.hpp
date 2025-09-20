#ifndef DISPLAY_WEBAPP_HPP
#define DISPLAY_WEBAPP_HPP

#include "../WebServer/webserver.hpp"

class DisplayWebapp: public WebApp{
   public:
    virtual bool matches(const char* verb, const char* path);
    virtual void process(HttpRequest& request, HttpResponse& response);

};
#endif