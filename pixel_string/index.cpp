#include <string.h>
#include <sstream>
#include "index.hpp"

// Toolchain for generating this data from form.html
// https://www.toptal.com/developers/html-minifier
// https://tomeko.net/online_tools/cpp_text_escape.php?lang=en
static const char *page =
"<html>\n"
"<head>\n"
"<title>Pixel String</title>\n"
"<meta name=viewport content=\"width=device-width,initial-scale=1\">\n"
"<style>legend{font-style:italic;font-weight:700}fieldset{margin-bottom:7px;max-width:1024px}canvas{border:1px solid #000;height:20;width:100%;grid-column:1}input[type=range]{width:100%}input[type=number]{width:100%;max-width:10em}.grid{display:grid;row-gap:10px;grid-template-columns:auto}input{grid-column:1}label{grid-column:1}@media screen and (min-width:550px){.grid{display:grid;column-gap:20px;grid-template-columns:100px auto 10em}input{grid-column:2}label{grid-column:1}canvas{border:1px solid #000;height:20;width:100%;grid-column:2}.c3{grid-column:3}.r1{grid-row:1}.r2{grid-row:2}.r3{grid-row:3}.r4{grid-row:4}}</style>\n"
"<script>var baseURI=document.baseURI;baseURI=\"http://pixel_string.local\";var huePixels=0,satPixels=1,valPixels=1;function hsvToRgb(e,t,n){let o=g=b=0;e>1&&(e=1),t>1&&(t=1),n>1&&(n=1);const s=Math.floor(6*e),i=6*e-s,l=n*(1-t),a=n*(1-i*t),c=n*(1-(1-i)*t);function h(e){let t=Math.floor(e).toString(16);return 1==t.length?\"0\"+t:t}0==s?(o=n,g=c,b=l):1==s?(o=a,g=n,b=l):2==s?(o=l,g=n,b=c):3==s?(o=l,g=a,b=n):4==s?(o=c,g=l,b=n):s<=6&&(o=n,g=l,b=a);return\"#\"+h(255*o)+h(255*g)+h(255*b)}function h2RGB(e,t,n){let o=g=b=0;e>1&&(e=1);let s=Math.floor(6*e),i=6*e-s,l=1-i;0==s?(o=1,g=i,b=0):1==s?(o=l,g=1,b=0):2==s?(o=0,g=1,b=i):3==s?(o=0,g=l,b=1):4==s?(o=i,g=0,b=1):s<=6&&(o=1,g=0,b=l),t[n+0]=Math.floor(255*o),t[n+1]=Math.floor(255*g),t[n+2]=Math.floor(255*b),t[n+3]=255}function drawRGB(e,t){var n=document.getElementById(e),o=n.getContext(\"2d\");const s=o.getImageData(0,0,n.width,n.height),i=s.data;let l=0;for(let e=0;e<s.height;e++)for(let e=0;e<s.width;e++)h2RGB(e/s.width,i,l),l+=4;o.putImageData(s,0,0),n.addEventListener(\"click\",(e=>function(e){const o=n.getBoundingClientRect(),s=e.clientX-o.left,i=(e.clientY,o.top,s/o.width);return t&&t(i),i}(e)))}function sendPixelColour(){const e=new URL(\"/set\",baseURI);e.search=`?h=${huePixels}&s=${satPixels}&v=${valPixels}`;fetch(e,{method:\"GET\"})}function setChase(e){const t=e?new URL(\"/chase_on\",baseURI):new URL(\"/chase_off\",baseURI);fetch(t,{method:\"GET\"})}function setRainbow(e){const t=e?new URL(\"/rainbow_on\",baseURI):new URL(\"/rainbow_off\",baseURI);fetch(t,{method:\"GET\"})}function setDeltaHue(e){const t=new URL(\"/rate\",baseURI);t.search=`?dv=${e}`,console.log(t.search);fetch(t,{method:\"GET\"})}function setBrightPixels(e){valPixels=e,sendPixelColor()}function setSatPixels(e){satPixels=e,sendPixelColour()}function configure(){drawRGB(\"rgbPixels\",(function(e){huePixels=e,sendPixelColour()}))}</script>\n"
"</head>\n"
"<body onload=configure()>\n"
"<h1>Pixel String</h1>\n"
"<h2>Display Colours</h2>\n"
"<fieldset>\n"
"<legend>String colour and brightness</legend>\n"
"<div class=grid>\n"
"<label for=rgbPixels>Colour</label>\n"
"<canvas id=rgbPixels> </canvas>\n"
"<label for=brightnessPixels>Brightness</label>\n"
"<input id=brightnessPixels type=range min=0 max=1.0 step=0.001 value=0.5 onchange=setBrightPixels(this.value)>\n"
"<label for=satPixels>Saturation</label>\n"
"<input id=satPixels type=range min=0 max=1.0 step=0.001 value=1.0 onchange=setSatPixels(this.value)>\n"
"</div>\n"
"</fieldset>\n"
"<fieldset>\n"
"<legend>Modes</legend>\n"
"<div>\n"
"<label for=chase>Enable Chase</label>\n"
"<input id=chase type=checkbox checked onchange=setChase(this.checked)>\n"
"</div>\n"
"<div>\n"
"<label for=rainbow>Enable Rainbow</label>\n"
"<input id=rainbow type=checkbox checked onchange=setRainbow(this.checked)>\n"
"</div>\n"
"<div>\n"
"<label for=deltahue>Hue change for rainbow</label>\n"
"<input id=deltahue type=range min=0 max=0.25 step=0.001 value=0.01 onchange=setDeltaHue(this.value)>\n"
"</div>\n"
"</fieldset>\n"
"</body>\n"
"</html>"
;

// extern Letterbox letterbox;

bool IndexPage::matches(const char *verb, const char *path)
{
    bool accept = (strcmp(verb, "GET") == 0) && (strncmp(path, "/", 1) == 0 ||
                                                 strncmp(path, "/index", 6) == 0 ||
                                                 false);

    if (accept)
        printf("Index Webapp Accepting path %s\n", path);

    return accept;
}

void IndexPage::process(HttpRequest &request, HttpResponse &response)
{
    response.setStatus(200, "OK");
    response.addHeader("Server", "PicoW-Web");
    response.addHeader("Content-Type", "text/html");
    response.addHeader("Access-Control-Allow-Origin", "*");

    response.setBody(page);
}
