#include <sstream>
#include "weather_webapp.hpp"
#include "letterbox.hpp"

 

bool WeatherWebapp::matches(const char* verb, const char* path){
    bool accept = false;
    if(strcmp(verb,"GET") == 0) {
        accept = 
            strncmp(path,"/data",5) == 0 ||
            false;
            ;
        if(accept) printf("Weather Webapp Accepting path %s\n", path);
    }
    return accept;
}

void WeatherWebapp::process( HttpRequest& request, HttpResponse& response){

    printf("Weather Webapp Processing request for %s\n",request.path());
 

    BlockListIter<Parameter> iter = request.Parameters().iter();
    Parameter* p;
    while(p = iter.next()){
        printf("Parameter %s -> %s\n", p->name(), p->value());
        
    }

    std::ostringstream os;
    os << "{";
    os << "\"pressure\" : " << letterbox.pressure << ",";
    os << "\"humidity\" : " << letterbox.humidity << ",";
    os << "\"lux\" : "      << letterbox.lux << ",";
    os << "\"temperature\" : " << letterbox.primaryTemp << ",";
    os << "\"temp2\" : "    << letterbox.temp2 << ",";
    os << "\"temp3\" : "    << letterbox.temp3 ;
    os << "}";

    response.setStatus(200,"OK");
    response.addHeader("Server", "PicoW");
    response.addHeader("Content-Type", "application/json");
    response.addHeader("Access-Control-Allow-Origin","*");
    response.setBody(os.str().c_str());
}
