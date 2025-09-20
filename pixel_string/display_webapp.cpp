#include "display_webapp.hpp"
#include "display.hpp"

extern LedDisplay display;  

bool DisplayWebapp::matches(const char* verb, const char* path){
    bool accept = false;
    if(strcmp(verb,"GET") == 0) {
        accept = 
            strncmp(path,"/set",4) == 0 ||
            strncmp(path,"/rainbow_on",11) == 0 ||
            strncmp(path,"/rainbow_off",12) == 0 ||
            strncmp(path,"/chase_on",9) == 0 ||
            strncmp(path,"/chase_off",10) == 0 ||
            strncmp(path,"/rate",5) == 0 ||
            strncmp(path,"/sparkle",8) == 0 ||
            strncmp(path,"/colours",8) == 0 ||
            false;
            ;
        if(accept) printf("Neopixel Webapp Accepting path %s\n", path);
    }
    return accept;
}

void DisplayWebapp::process( HttpRequest& request, HttpResponse& response){

    float hue = 0.0f;
    float sat = 1.0f;
    float val = 1.0f;
    float dv = 0.0f;

    BlockListIter<Parameter> iter = request.Parameters().iter();
    Parameter* p;
    while(p = iter.next()){
        //printf("Parameter %s -> %s\n", p->name(), p->value());
        if(strcmp(p->name(), "dv") == 0) dv = p->asFloat();
        if(strcmp(p->name(), "h") == 0) hue = p->asFloat();
        if(strcmp(p->name(), "s") == 0) sat = p->asFloat();
        if(strcmp(p->name(), "v") == 0) val = p->asFloat();
    }

    if(strncmp(request.path(),"/set",4) == 0) {
         display.setHSV(hue, sat, val);
    } else if (strncmp(request.path(),"/rainbow_on",11) == 0) {
        display.setRainbow(true);
    } else if (strncmp(request.path(),"/rainbow_off",12) == 0) {
        display.setRainbow(false);
    } else if (strncmp(request.path(),"/chase_on",9) == 0) {
        display.setChase(true);
    } else if (strncmp(request.path(),"/chase_off",10) == 0) {
        display.setChase(false);
    } else if (strncmp(request.path(),"/rate",6) == 0) {
        display.setHueChange(dv);
    }  else if (strncmp(request.path(),"/sparkle",8) == 0) {
        display.setSparkle(true);
    }  else if (strncmp(request.path(),"/colours",8) == 0) {
        display.setSparkle(false);
    } else {

    }
  

    response.setStatus(200,"OK");
    response.addHeader("Server", "PicoW");
    response.addHeader("Access-Control-Allow-Origin","*");
    
}
