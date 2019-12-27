// default implemantion
let url =  '192.168.1.123'; // change this to the ip of your webRotator or add ?ip=192.168.1.123 to the url - but just the ip! no "http.." or somethign else.. :P

// default implementation
let debug = false;

// time is in ms, so 1000 = 1s, 10000 = 10s, 30000 = 30s ans 900000 = 15min.
// default implementation
let standardIntervalRotating =      1000;   // dont be faster like 1000! webRotatorInterface based on a "slow" mega with 10mbit Ethernet shield via SPI...
let standardIntervalTime =          10000;  // 30000 speed this up in contest
let cleanupStandardIntervalTime =   30000;  // 30000 speed this up in contest
let cleanupLongRunIntervalTime =    900000; // 900000 15min speed this up in contest


/*********************************************************************************************************************************************************/
/*************************************************************** Makro & Keyboardshortcuts ***************************************************************/
/*********************************************************************************************************************************************************/
// defin the makro class with the headings. makro1 will be keyboard "1".... Create your own shortcust by just defining <keyname>:<heading> in the list
// IMPORTANT: If you change that, make sure no to forgot the "," ---- But if you want you overwrite the shortcust in the url like the ip
// ....www.theaddressyouknow.de/rotator.html?ip=192.168.1.120 goes to /rotator.html?ip=192.168.1.120&shortcut=1:250,2:290

// default implemantion
let makroDefinition = "1:0,2:45,3:90,4:135,5:180,6:225,7:270,8:315,9:20,0:350,x:110";



/*********************************************************************************************************************************************************/
/************************************************************************ Helpers ************************************************************************/
/*********************************************************************************************************************************************************/

function generateMakro()
{
    makroDefinition = getUrlParam("shortcut", makroDefinition);
    var makroDefinitionArray = decodeURIComponent(makroDefinition).split(",");
    var makroDefinitionString = "";

    makroDefinitionArray.forEach(element => {

        let part = element.split(":");
        makroDefinitionString+= ' "makro'+part[0]+'":'+part[1]+',';
    });

    makroDefinitionString = makroDefinitionString.slice(0,-1);
    makroDefinitionString = "{"+makroDefinitionString+"}";8
    makro = JSON.parse(makroDefinitionString);
}

function getParamsFromUrl()
{
    standardIntervalRotating = getUrlParam("sir", standardIntervalRotating);
    standardIntervalTime = getUrlParam("sit", standardIntervalTime);
    cleanupStandardIntervalTime = getUrlParam("csit", cleanupStandardIntervalTime);
    cleanupLongRunIntervalTime = getUrlParam("clrit", cleanupLongRunIntervalTime);

    debug = (getUrlParam("debug", "false") == 'true');
    url = getUrlParam('ip',url);
}

function getUrlVars() {
    var vars = {};
    var parts = window.location.href.replace(/[?&]+([^=&]+)=([^&]*)/gi, function(m,key,value) {
        vars[key] = decodeURIComponent(value);
    });
    return vars;
}

function getUrlParam(parameter, defaultvalue){
    var urlparameter = defaultvalue;
    if(window.location.href.indexOf(parameter) > -1){
        urlparameter = getUrlVars()[parameter];
        }
    return urlparameter;
}
