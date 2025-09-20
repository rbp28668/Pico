#ifndef WEATHER_WEBAPP_HPP
#define WEATHER_WEBAPP_HPP

#include "../WebServer/webserver.hpp"

class WeatherWebapp: public WebApp{
   public:
    virtual bool matches(const char* verb, const char* path);
    virtual void process(HttpRequest& request, HttpResponse& response);

};
#endif