#include <string.h>
#include <sstream>
#include "index.hpp"

// Toolchain for generating this data from form.html
// https://www.toptal.com/developers/html-minifier
// https://tomeko.net/online_tools/cpp_text_escape.php?lang=en
static const char *page =
"<html>\n"
"<head>\n"
"<title>BCD Clock</title>\n"
"<meta name=viewport content=\"width=device-width,initial-scale=1\">\n"
"<style>legend{font-style:italic;font-weight:700}fieldset{margin-bottom:7px;max-width:1024px}canvas{border:1px solid #000;height:20;width:100%;grid-column:1}input[type=range]{width:100%}input[type=number]{width:100%;max-width:10em}.grid{display:grid;row-gap:10px;grid-template-columns:auto}input{grid-column:1}label{grid-column:1}@media screen and (min-width:550px){.grid{display:grid;column-gap:20px;grid-template-columns:100px auto 10em}input{grid-column:2}label{grid-column:1}canvas{border:1px solid #000;height:20;width:100%;grid-column:2}.c3{grid-column:3}.r1{grid-row:1}.r2{grid-row:2}.r3{grid-row:3}.r4{grid-row:4}}</style>\n"
"<script>var baseURI=document.baseURI,hueDigits=0,satDigits=1,valDigits=1,hueColons=0,satColons=1,valColons=1;function hsvToRgb(t,o,n){let s=g=b=0;t>1&&(t=1),o>1&&(o=1),n>1&&(n=1);const e=Math.floor(6*t),i=6*t-e,a=n*(1-o),l=n*(1-i*o),h=n*(1-(1-i)*o);function c(t){let o=Math.floor(t).toString(16);return 1==o.length?\"0\"+o:o}0==e?(s=n,g=h,b=a):1==e?(s=l,g=n,b=a):2==e?(s=a,g=n,b=h):3==e?(s=a,g=l,b=n):4==e?(s=h,g=a,b=n):e<=6&&(s=n,g=a,b=l);return\"#\"+c(255*s)+c(255*g)+c(255*b)}function h2RGB(t,o,n){let s=g=b=0;t>1&&(t=1);let e=Math.floor(6*t),i=6*t-e,a=1-i;0==e?(s=1,g=i,b=0):1==e?(s=a,g=1,b=0):2==e?(s=0,g=1,b=i):3==e?(s=0,g=a,b=1):4==e?(s=i,g=0,b=1):e<=6&&(s=1,g=0,b=a),o[n+0]=Math.floor(255*s),o[n+1]=Math.floor(255*g),o[n+2]=Math.floor(255*b),o[n+3]=255}function drawRGB(t,o){var n=document.getElementById(t),s=n.getContext(\"2d\");const e=s.getImageData(0,0,n.width,n.height),i=e.data;let g=0;for(let t=0;t<e.height;t++)for(let t=0;t<e.width;t++)h2RGB(t/e.width,i,g),g+=4;s.putImageData(e,0,0),n.addEventListener(\"click\",(t=>function(t){const s=n.getBoundingClientRect(),e=t.clientX-s.left,i=(t.clientY,s.top,e/s.width);return o&&o(i),i}(t)))}function sendDigits(){const t=new URL(\"/digitshsv\",baseURI);t.search=`?h=${hueDigits}&s=${satDigits}&v=${valDigits}`;fetch(t,{method:\"GET\"})}function sendDigitsRGB(){const t=new URL(\"/digits\",baseURI),o=hsvToRgb(hueDigits,satDigits,valDigits);t.search=`?rgb=${o}`;fetch(t,{method:\"GET\"})}function sendColons(){const t=new URL(\"/colonshsv\",baseURI);t.search=`?h=${hueColons}&s=${satColons}&v=${valColons}`;fetch(t,{method:\"GET\"})}function sendColonsRGB(){const t=new URL(\"/colons\",baseURI),o=hsvToRgb(hueColons,satColons,valColons);t.search=`?rgb=${o}`;fetch(t,{method:\"GET\"})}function setBrightDigits(t){valDigits=t,sendDigits()}function setSatDigits(t){satDigits=t,sendDigits()}function setBrightColons(t){valColons=t,sendColons()}function setSatColons(t){satColons=t,sendColons()}function setAutoBrightness(t){const o=new URL(\"/autobright\",baseURI);o.search=`?on=${t}`;fetch(o,{method:\"GET\"})}function configure(){drawRGB(\"rgbDigits\",(function(t){hueDigits=t,sendDigits()})),drawRGB(\"rgbColons\",(function(t){hueColons=t,sendColons()}))}</script>\n"
"</head>\n"
"<body onload=configure()>\n"
"<h1>BCD Clock</h1>\n"
"<h2>Display Colours</h2>\n"
"<fieldset>\n"
"<legend>Digit colour and brightness</legend>\n"
"<div class=grid>\n"
"<label for=rgbDigits>Colour</label>\n"
"<canvas id=rgbDigits> </canvas>\n"
"<label for=brightnessDigits>Brightness</label>\n"
"<input id=brightnessDigits type=range min=0 max=1.0 step=0.001 value=0.5 onchange=setBrightDigits(this.value)>\n"
"<label for=satDigits>Saturation</label>\n"
"<input id=satDigits type=range min=0 max=1.0 step=0.001 value=1.0 onchange=setSatDigits(this.value)>\n"
"</div>\n"
"</fieldset>\n"
"<fieldset>\n"
"<legend>Colon colour and brightness</legend>\n"
"<div class=grid>\n"
"<label for=rgbColons>Colour</label>\n"
"<canvas id=rgbColons> </canvas>\n"
"<label for=brightnessColons>Brightness</label>\n"
"<input id=brightnessColons type=range min=0 max=1.0 step=0.001 value=0.5 onchange=setBrightColons(this.value)>\n"
"<label for=satColons>Saturation</label>\n"
"<input id=satColons type=range min=0 max=1.0 step=0.001 value=1.0 onchange=setSatColons(this.value)>\n"
"</div>\n"
"</fieldset>\n"
"<fieldset>\n"
"<legend>Auto brightness</legend>\n"
"<div>\n"
"<label for=autoBrightness>Enable</label>\n"
"<input id=autoBrightness type=checkbox onchange=setAutoBrightness(this.checked)>\n"
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
