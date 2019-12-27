$(document).ready(function(){

    let deg = 0;
    let interval = window.setInterval(RotationManager, standardIntervalTime);
    
/*********************************************************************************************************************************************************/
/************************************************************************ Events *************************************************************************/
/*********************************************************************************************************************************************************/

    window.addEventListener("mousedown", handleCtrlOnClick , true);
    window.addEventListener("keydown", handleKeydown, true);
    window.addEventListener("mousemove", handleMouseMove, true);

    // handle the Cntrl+Onclick event and set it to the mousepositiion deg in mousePostionDeg
    function handleCtrlOnClick(event) {
        if(event.ctrlKey)
            setHeading(mousePositionDeg);
    }

    // handle the keydown evants
    function handleKeydown(event) {
        debugHelper(event.key);
        debugHelper(event.code);
        
        if(event.code == "AltLeft")
            Stop();
        else
            executeShortcut(event);
    }

    // convert the mouseXY position into currect deg
    function handleMouseMove(event) {
        let eventDoc, doc, body;

        event = event || window.event;

        if (event.pageX == null && event.clientX != null) {
            eventDoc = (event.target && event.target.ownerDocument) || document;
            doc = eventDoc.documentElement;
            body = eventDoc.body;

            event.pageX = event.clientX +
              (doc && doc.scrollLeft || body && body.scrollLeft || 0) -
              (doc && doc.clientLeft || body && body.clientLeft || 0);
            event.pageY = event.clientY +
              (doc && doc.scrollTop  || body && body.scrollTop  || 0) -
              (doc && doc.clientTop  || body && body.clientTop  || 0 );
        }

        let diffx = event.pageX-mapx;
        let diffy = mapy-event.pageY;

        $('#mx').html(event.pageX-mapx);
        $('#my').html(mapy-event.pageY);

        let quadrant = 0;
        let rad = (Math.PI/180);

        if(diffx > 0 && diffy > 0)
            quadrant = 1;
        
        if(diffx < 0 && diffy > 0)
            quadrant = 2;

        if(diffx < 0 && diffy < 0)
            quadrant = 3;

        if(diffx > 0 && diffy < 0)
            quadrant = 4;

        if(quadrant== 1 || quadrant == 4)
            mousePositionDeg = Math.round(90-(Math.atan(diffy/diffx)/rad));
        else
            mousePositionDeg = Math.round(270-(Math.atan(diffy/diffx)/rad));

        $('#w').html(mousePositionDeg);
        $('#q').html(quadrant);
    }


/*********************************************************************************************************************************************************/
/******************************************************************* Rotation Function *******************************************************************/
/*********************************************************************************************************************************************************/

    // Corssbrowser rotation function
    function rotate($element, degree) {
        $element.css({
            '-webkit-transform': 'rotate(' + degree + 'deg)',
               '-moz-transform': 'rotate(' + degree + 'deg)',
                '-ms-transform': 'rotate(' + degree + 'deg)',
                 '-o-transform': 'rotate(' + degree + 'deg)',
                    'transform': 'rotate(' + degree + 'deg)'
        });
    }

    // set the rorator background into the middle of the page
    function centerImages() {
        let div = $('.rotator__background');
        let divCoords = { 
            x : div.width() * 0.5, 
            y : div.height() * 0.5
        };
     
        mapx = divCoords.x;
        mapy = divCoords.y;
    
        $('#ix').html(mapx);
        $('#iy').html(mapy);
    
    }

    // set the heading to eee
    function setHeading(eee)
    {
        debugHelper("Function heading called with " + eee);
        goToHeading = eee;
        // if rotating, stop it an go for the new heading.. :P
        if(rotatingStatus != -1)
            Stop();

        remoteWatchCount = 0;   // wichtig zur Überwachung von externen bewegungen
        rotate($('.rotator__target'), goToHeading);
        rotateToCurrentHeading();    
    }

    // request the Deg and Value from the webRotator Interface
    function getCurrentHeading()
    {
        if(error)
        {
            debugHelper("sleeping..");
            Sleep(3000);
            debugHelper("Starting again!");
            error = false;
        }

        $.getJSON('http://'+url, function(data) {
            responseHeading = Math.round(data.p);
            responseValue = data.v;
            rotatingStatus = data.s;
            debugHelper('CurrentDeg:' + responseHeading + ' CurrentValue: ' + responseValue + ' Rotation: ' + rotatingStatus + " Callback Error: " + error) ;
        })
        .fail(async function() { debugHelper("Error occured! Call failed!");  error = true; })
    }

    // Json Set the Go-To Heading
    function rotateToCurrentHeading()
    {
        $.get('http://'+url+'/Go/'+goToHeading+'/', function(data, status) { debugHelper('GoTo Ajay Fired') ; isRotatingStarted = true; RotationManager()});
    }

    // Stop Rotating
    function Stop()
    {
        $.get('http://'+url+'/Stop/1/', function(data, status) {
            debugHelper('Rotator stoppped! Cooling down for ' +coolDownTime) ;
            isRotatingStarted = false;
            Sleep(coolDownTime); // wait for final stopping..
        });
    }

    // Function to manage orchestrating the data.
    function RotationManager()
    {
        debugHelper("Function RotationManager called..");

        getCurrentHeading();
        rotate($('.rotator__current'), responseHeading);

        if(isRotatingStarted && rotatingStatus!=-1)
        {
            debugHelper("RM: Rotation statedb4 " + responseHeading);

            if(!rotatingWasStartedB4)
            {
                rotatingWasStartedB4 = true;
                setupCallLoopInterval(standardIntervalRotating);       // do faster requests while rotating!
                remoteWatchCount = 0;                                // when rotating, the counter must be set to 0...
            }
        }
        else
        {
            if(rotatingWasStartedB4 && isRotatingStarted)
                isRotatingStarted = false;

            debugHelper("ResonseDeg is " + responseHeading);
            debugHelper("GoTo is " + goToHeading);

            // if it was rotating before, but now stopped...
            if(rotatingWasStartedB4 && isRotatingStarted == false)
            {
                rotatingWasStartedB4 = false;                        // mark rotation as finished.
                setupCallLoopInterval(standardIntervalTime);           // 
                rotate($('.rotator__target'), responseHeading);
            }

            if(!isRotatingStarted && !rotatingWasStartedB4 && responseHeading != goToHeading)
            {
                debugHelper("Equalizer must be called by counter " + remoteWatchCount);
                goToHeading = responseHeading;                
                rotate($('.rotator__target'), responseHeading);
            }
        }
    }

    // set the interval used to call the webRotator Interface to get new Values
    function setupCallLoopInterval(currentITime)
    {
        debugHelper("SetCurrentInterval called and set to " + currentITime);

        window.clearInterval(interval);
        RotationManager();
        interval = window.setInterval(RotationManager, currentITime);
    }

    // ugly syncronical sleep function, but working...
    function Sleep(ms) { // not nice, but working :P
        var start = new Date().getTime(), expire = start + ms;
        while (new Date().getTime() < expire) { }
        return;
    }

    // abstraction to disable debugging information by just not logging them out.
    function debugHelper(text)
    {
        if(debug)
            console.log(text);
        else
        {
            $('#debugInfo').css("visibility", "hidden");
            return; //console.log(text);    
        }
    }

    // Shortcut handlers sse down below the js-file
    function executeShortcut(calledEvent)
    {
        let classname = "makro.makro"+calledEvent.key;
        if(eval(classname) != undefined)
            setHeading(eval(classname));
    }    

/*********************************************************************************************************************************************************/
/******************************************************************** Initialisation *********************************************************************/
/*********************************************************************************************************************************************************/

    generateMakro();
    getParamsFromUrl();
    centerImages();
    getCurrentHeading();
        
    window.setTimeout(function() {
        rotate($('.rotator__current'), responseHeading);
        rotate($('.rotator__target'), responseHeading);    
        goToHeading = responseHeading;
        debugHelper("Initialisation finished");
    }, 2000);
});

let mapx = 0;
let mapy = 0;
let makro;

let mousePositionDeg = 0;
let goToHeading = mousePositionDeg;
let responseHeading = 0;
let responseValue = 0;
let coolDownTime = 3000;
let remoteWatchCount = 0;
let error = false;

let rotatingStatus = -1;        // -1 : --, 0: ccw, 1: cw
let rotatingWasStartedB4 = false;    // -1 was stopped, 0 was rotating
let isRotatingStarted = false;