//#define DEBUG_SERIALSETUP
//#define DEBUG_READPOTIVALUES
//#define DEBUG_DISABLE_EEPROM_READ
//#define DEBUG_DISABLE_EEPROM_WRITE
//#define DEBUG
//#define SIMULATION
#define WITH_LCD

#include <SPI.h>
#include <Ethernet.h>
#include <EEPROMex.h>
#include "SoftReset.h"
#include <Wire.h>

#ifdef WITH_LCD
    #include <LiquidCrystal_I2C.h>
#endif

/* some other way of smoothing...
#ifndef SIMULATION
    #include "Filter.h"
#endif
*/

#define ON 0
#define OFF 1
#define RelayCWRightP6 6
#define RelayCCWLeftP7 7
#define PotiPin 0
#define STATUS_TURNING_CCWLEFT0 0
#define STATUS_TURNING_CWRIGHT1 1
#define STATUS_TURNING_NOT -1
#define ADDRESSIP0 0
#define ADDRESSIP2 2
#define ADDRESSIP3 4
#define ADDRESSIP4 6
#define ADDRESSDHCP 8
#define ADDRESSCCWLimit 9
#define ADDRESSCWLimit 11

byte mac[] = {
    0xDE,
    0xAD,
    0xBE,
    0xEF,
    0xFE,
    0x66};

int ip1 = 192;
int ip2 = 168;
int ip3 = 1;
int ip4 = 66;
byte isDHCP = 3;

EthernetServer server(80);
String readString = ""; // string for fetching data from address
String cmd = "";


int ccwleftLimit = 200;
int cwrightLimit = 800;
float middle = 0;
float calculationFactorDeg = 0;

float currentPostitionDeg = 180;
float currentPositionValue = 200;
int goToDeg = 180;
int goToValue = 200;

bool isTurningActionCalled = false;
bool isStopped = true;
int status = STATUS_TURNING_NOT;

bool isConfigMode = false;

#ifdef WITH_LCD
    LiquidCrystal_I2C lcd(0x27,16,2);
    long previousMillis = 0;        // will store last time LED was updated
    long interval = 1000;           // interval at which to blink (milliseconds)
#endif



#ifdef DEBUG_SERIALSETUP
void setupSerial(){
    Serial.begin(115200);
    while (!Serial)
    {
        ; // wait for serial port to connect. Needed for native USB port only
    }
}
#endif

void calculateFactorAndMiddle(){
        middle = ((cwrightLimit -ccwleftLimit) / 2)+ccwleftLimit;
        calculationFactorDeg = ((cwrightLimit-ccwleftLimit) / 360.00)+0.000000066667;
}


float GetValueByDegSouth(int deg)
{
    if (deg > 180 && deg < 360)
        return ((deg - 180) * calculationFactorDeg) + ccwleftLimit;
    if (deg < 180 && deg > 0)
        return cwrightLimit - ((180 - deg) * calculationFactorDeg);

    if (deg == 180 && currentPostitionDeg < 360 && currentPostitionDeg > 180)
        return ccwleftLimit;
    if (deg == 180 && currentPostitionDeg < 180)
        return cwrightLimit;

    if (deg == 0 || deg == 360)
        return middle;

    if (deg == 180 && currentPostitionDeg == 180)
        return currentPositionValue;

    return -1;
}

float GetDegByValueSouth(int value)
{
    if (value > ccwleftLimit && value < middle)
        return ((value - ccwleftLimit) / calculationFactorDeg) + 180.00;
    if (value > middle && value < cwrightLimit)
        return 180.00 - ((cwrightLimit - value) / calculationFactorDeg);

    if (value == middle)
        return 0;

    if (value == currentPositionValue)
        return currentPostitionDeg;

    if (value == ccwleftLimit || value == cwrightLimit)
        return 180;

    return -1;
}

#ifndef SIMULATION
void readPotiAndSetCurrentDegCurrentValue(){

    unsigned long  sum = 0;

    for(int y = 0; y<10; y++)
    {
        int RawValue = analogRead(PotiPin);
        sum += RawValue;
        delay(2);
    }
    currentPositionValue = (sum / 10);
    currentPostitionDeg = GetDegByValueSouth(currentPositionValue);
#ifdef DEBUG_READPOTIVALUES
    Serial.print(" finalsum ");
    Serial.println(currentPositionValue);
#endif
}
#endif

/* Some other posibilities to smoothen ADC readings
long oversample(int pin, int pbit) {
long summe=0;
  for(int i=0; i < pow(4.0,pbit); i++) {
    summe += analogRead(pin);
  }
  return summe >> pbit;
}

void readPotiAndSetCurrentDegCurrentValue(){

    int RawValue = analogRead(0);
    ADCFilter.Filter(RawValue);
    currentPositionValue = ADCFilter.Current();
    currentPostitionDeg = GetDegByValueSouth(currentPositionValue)
}


void readPotiAndSetCurrentDegCurrentValue__(){

    float sum = 0;

    for(int y = 0; y<10; y++)
    {
        int RawValue = analogRead(0);
        sum += RawValue;
    } 
    currentPositionValue = sum / 10;
    currentPostitionDeg = GetDegByValueSouth(currentPositionValue)
}

void readPotiAndSetCurrentDegCurrentValue_(){
    float RawValue = oversample(analogRead(0),4) << 4;
    ADCFilter.Filter(RawValue);
    currentPositionValue = ADCFilter.Current();
    currentPostitionDeg = GetDegByValueSouth(currentPositionValue)
}
*/


#ifdef DEBUG
void DebugPrintGlobals()
{
    Serial.print("middle:");
    Serial.println(middle);

    Serial.print("goToDeg:");
    Serial.println(goToDeg);

    Serial.print("goToValue:");
    Serial.println(goToValue);

    Serial.print("currentPostitionDeg:");
    Serial.println(currentPostitionDeg);

    Serial.print("calculationFactorDeg:");
    Serial.println(calculationFactorDeg, 16);

    delay(1000);
}
#endif



#ifndef SIMULATION
void setRelayCWRightP6(byte state){
    digitalWrite(RelayCWRightP6, state);
}

void setRelayCCWLeftP7(byte state){
    digitalWrite(RelayCCWLeftP7, state);
}
#endif

void stop()
{
#ifndef SIMULATION
    readPotiAndSetCurrentDegCurrentValue();
    setRelayCWRightP6(OFF);
    setRelayCCWLeftP7(OFF);
#endif

    status = STATUS_TURNING_NOT;
    isStopped = true;
    goToDeg = currentPostitionDeg;
    goToValue = currentPositionValue;
    isTurningActionCalled = false;
#ifdef DEBUG
    Serial.println("ich habe gestoppt");
    DebugPrintGlobals();
    delay(2000);
#endif
}

int getTurningDirectionByValue(int valueToTurnTo)
{
    if (valueToTurnTo < currentPositionValue)
        return 0; // ccw
    else
        return 1; // cw
}

String getValue(String data, char separator, int index)
{
    int found = 0;
    int strIndex[] = {
        0,
        -1};
    int maxIndex = data.length() - 1;

    for (int i = 0; i <= maxIndex && found <= index; i++)
    {
        if (data.charAt(i) == separator || i == maxIndex)
        {
            found++;
            strIndex[0] = strIndex[1] + 1;
            strIndex[1] = (i == maxIndex) ? i + 1 : i;
        }
    }
    return found > index ? data.substring(strIndex[0], strIndex[1]) : "";
}

void EEPROM_ReadIP(){
    ip1 = EEPROM.readInt(ADDRESSIP0);
    ip2 = EEPROM.readInt(ADDRESSIP2);
    ip3 = EEPROM.readInt(ADDRESSIP3);
    ip4 = EEPROM.readInt(ADDRESSIP4);
}

void EEPROM_WriteIP(){
    EEPROM.updateInt(ADDRESSIP0, ip1);
    EEPROM.updateInt(ADDRESSIP2, ip2);
    EEPROM.updateInt(ADDRESSIP3, ip3);
    EEPROM.updateInt(ADDRESSIP4, ip4);
}   

void EEPROM_ReadDHCP(){
    isDHCP = EEPROM.readByte(ADDRESSDHCP);
}

void EEPROM_WriteDHCP(){
    EEPROM.updateByte(ADDRESSDHCP, isDHCP);
}

void EEPROM_ReadCCWLimit(){
    ccwleftLimit = EEPROM.readInt(ADDRESSCCWLimit);
}

void EEPROM_WriteCCWLimit(){
    EEPROM.updateInt(ADDRESSCCWLimit, ccwleftLimit);
}

void EEPROM_ReadCWLimit(){
    cwrightLimit = EEPROM.readInt(ADDRESSCWLimit);
}

void EEPROM_WriteCWLimit(){
    EEPROM.updateInt(ADDRESSCWLimit, cwrightLimit);
}

void EEPROM_WriteAllVariablesToEprom(){
    EEPROM_WriteDHCP();
    EEPROM_WriteIP();
    EEPROM_WriteCWLimit();
    EEPROM_WriteCCWLimit();
}


void WebServerHeader(EthernetClient client)
{
                    client.println("HTTP/1.1 200 OK");
                    client.println("Content-Type: application/json");
                    client.println("Access-Control-Allow-Origin: *");
                    client.println("Accept: application/json");
                    client.println("Connection: close"); // the connection will be closed after completion of the response
                    client.println();
}

void WebServerRestart(EthernetClient client){
                    WebServerHeader(client);
                    client.println("{\"Warning\":\"Restart\"}");
                    client.stop();
                    delay(5000);
                    soft_restart();
}


void WebServer()
{
    EthernetClient client = server.available();
    if (client)
    {
#ifdef DEBUG
        Serial.println("new client");
        // an http request ends with a blank line
#endif
        boolean currentLineIsBlank = true;
        while (client.connected())
        {
            if (client.available())
            {
                char c = client.read();
                int lenStr = readString.length();

                if (lenStr < 12)
                {
                    readString.concat(c);
                }

                if (lenStr == 12 || (c == '\n' && currentLineIsBlank))
                {
#ifdef DEBUG
                    //Serial.println(readString);
#endif
                    String cmd = getValue(readString, '/', 1);
                    int cmd_2 = getValue(readString, '/', 2).toInt();

                    int icmd = 0;

                    if(cmd == "Stop")
                        icmd = 1;
                    else if(cmd == "Go")
                        icmd = 2;
                    else if(cmd == "CONF")
                        isConfigMode = true;

                    if(isConfigMode)
                    {
#ifdef DEBUG
                        Serial.print("...ConfigModeIfs ");
                        Serial.println(icmd);
                        Serial.print("...cmd_2 ");
                        Serial.println(cmd_2);
#endif
                        if(cmd == "DHCP")
                            icmd = 3;
                        else if(cmd == "E1")
                            icmd = 40;
                        else if(cmd == "E2")
                            icmd = 41;
                        else if(cmd == "E3")
                            icmd = 42;
                        else if(cmd == "E4")
                            icmd = 43;
                        else if(cmd == "CC")
                            icmd = 5;
                        else if(cmd == "CW")
                            icmd = 6;
                        else if (cmd == "CONF")
                        {
                            if(cmd_2 == 0)
                                isConfigMode = false;
                        }
                       else if (cmd == "EWRT")
                        {
#ifdef DEBUG
                            Serial.println("Writing to the EEProm..");

#endif
#ifndef DEBUG_DISABLE_EEPROM_WRITE
                            EEPROM_WriteAllVariablesToEprom();
#endif
                            WebServerRestart(client);
                        }
                       else if(cmd == "RST")
                       {
                          WebServerRestart(client);
                       } 
                    }

#ifdef DEBUG
                    Serial.print("Icmdset to: ");
                    Serial.println(icmd);
#endif

                    switch (icmd)
                    {
                        case 1: // stop
                            stop();
                            isTurningActionCalled = false;
                            isStopped = true;
                            break;
                        case 2: // go
                            goToDeg = cmd_2;
                            isTurningActionCalled = true;
#ifdef DEBUG
                            Serial.print("Go command rotation to ");
                            Serial.println(goToDeg);
#endif
                            break;
                        case 3: // DHCP
                            isDHCP = cmd_2;
                            break;    
                        case 40: // EEprom IP
                            ip1 = cmd_2;
                            break;
                        case 41: // EEprom IP
                            ip2 = cmd_2;
                            break;
                        case 42: // EEprom IP
                            ip3 = cmd_2;
                            break;
                        case 43: // EEprom IP
                            ip4 = cmd_2;
                            break;
                        case 5: // CCWL
                            ccwleftLimit = cmd_2;
                            calculateFactorAndMiddle(); // middle and calculationFactorDeg are (re)calculated    
                            break;    
                        case 6: // CWL
                            cwrightLimit = cmd_2;
                            calculateFactorAndMiddle(); // middle and calculationFactorDeg are (re)calculated    
                            break;    
                        default:
#ifndef SIMULATION
                            readPotiAndSetCurrentDegCurrentValue();
#endif                        
                            isTurningActionCalled = false;
                        break;
                    }

#ifdef DEBUG
                    Serial.println(cmd);
                    Serial.println(goToDeg);
                    DebugPrintGlobals();
#endif
                    WebServerHeader(client);
                    if(isConfigMode)
                    {  
                        client.print("{\"dhcp\": ");
                        client.print(isDHCP);
                        client.print(", ");
                        client.print("\"eip\": \"");
                        client.print(ip1);
                        client.print(".");
                        client.print(ip2);
                        client.print(".");
                        client.print(ip3);
                        client.print(".");
                        client.print(ip4);
                        client.print("\", \"CCWLimit\": ");
                        client.print(ccwleftLimit);
                        client.print(", \"CWLimit\": ");
                        client.print(cwrightLimit);
                        client.print(", \"CenterValue\": ");
                        client.print(middle);
                        client.print(", \"CalcFactor\": ");
                        client.print(calculationFactorDeg, 16);
                    }
                    else
                    {  
                        client.print("{\"p\": ");
                        client.print(currentPostitionDeg);
                        client.print(", ");
                        client.print("\"v\": ");
                        client.print(currentPositionValue);
                        client.print(", \"s\": ");
                        client.print(status);
                    }
                    
                    client.print("}");
                    break;
                }
                if (c == '\n')
                {
                    currentLineIsBlank = true;
                }
                else if (c != '\r')
                {
                    currentLineIsBlank = false;
                }
            }
        }
        delay(2);
        client.stop();
        readString = "";
        cmd = "";
#ifdef DEBUG
        Serial.println("client disconnected");
#endif
    }
}

void turnCW()
{
#ifdef DEBUG
    Serial.println("Ich drehe mich im Uhrzeigersinn");
#endif
    while (!isStopped && currentPositionValue < goToValue)
    {
#ifndef SIMULATION 
        // turn on relays
        setRelayCWRightP6(ON);
        readPotiAndSetCurrentDegCurrentValue();
        // read poti
#else
        currentPositionValue += 2;
        currentPostitionDeg = GetDegByValueSouth(currentPositionValue);

#endif
        status = STATUS_TURNING_CWRIGHT1;
#ifdef DEBUG        
        Serial.print("Bin gerade ");
        Serial.print(currentPositionValue);
        Serial.print(" Bin Deg ");
#endif
        if (currentPostitionDeg == -1)
            currentPostitionDeg = 180;
#ifdef DEBUG
        Serial.println(currentPostitionDeg);
#endif
        WebServer();
#ifdef SIMULATION
        delay(300);
#endif
    }
    stop();
}

void turnCCW()
{
#ifdef DEBUG
    Serial.println("Ich drehe mich gegen Uhrzeigersinn");
#endif
    while (!isStopped && currentPositionValue > goToValue)
    {
        // turn on relays
        // read poti
#ifndef SIMULATION 
        // turn on relays
        setRelayCCWLeftP7(ON);
        readPotiAndSetCurrentDegCurrentValue();
        // read poti
#else
        currentPositionValue -= 2;
        currentPostitionDeg = GetDegByValueSouth(currentPositionValue);
#endif
        status = STATUS_TURNING_CCWLEFT0;
#ifdef DEBUG
        Serial.print("Bin gerade ");
        Serial.print(currentPositionValue);
        Serial.print(" Bin Deg ");
#endif
        if (currentPostitionDeg == -1)
            currentPostitionDeg = 180;
#ifdef DEBUG            
        Serial.println(currentPostitionDeg);
#endif
        WebServer();
#ifdef SIMULATION
        delay(300);
#endif
    }
    stop();
}


void setup()
{
#ifdef DEBUG_SERIALSETUP
    setupSerial();
#endif

#ifndef DEBUG_DISABLE_EEPROM_READ
    EEPROM_ReadDHCP();
    EEPROM_ReadIP();
    EEPROM_ReadCCWLimit();
    EEPROM_ReadCWLimit();
#endif

#ifdef WITH_LCD
    lcd.init();                      // initialize the lcd  
    lcd.backlight();
  
    lcd.print("Deg: ");
    lcd.setCursor(0, 1);
    lcd.print("Wert: ");
#endif 

#ifdef DEBUG
    Serial.print("DHCP ");
    Serial.println(isDHCP);

    Serial.print("IP ");
    Serial.print(ip1);
    Serial.print(".");
    Serial.print(ip2);
    Serial.print(".");
    Serial.print(ip3);
    Serial.print(".");
    Serial.println(ip4);
    Serial.print("CCW ");
    Serial.println(ccwleftLimit);
    Serial.print("CW ");
    Serial.println(cwrightLimit);
#endif


IPAddress ip(ip1,ip2,ip3,ip4);

if(isDHCP != 3 && isDHCP != 2)
    isDHCP = 3;

if(isDHCP == 3)
    Ethernet.begin(mac);
else
    Ethernet.begin(mac, ip);

    server.begin();
#ifdef DEBUG
    Serial.print("server is at ");
    Serial.println(Ethernet.localIP());
#endif


#ifndef SIMULATION
    calculateFactorAndMiddle(); // middle and calculationFactorDeg are (re)calculated    
    pinMode(RelayCWRightP6, OsUTPUT);   
    pinMode(RelayCCWLeftP7, OUTPUT);   

    digitalWrite(RelayCWRightP6, OFF);
    digitalWrite(RelayCCWLeftP7, OFF);
    readPotiAndSetCurrentDegCurrentValue();
#else
    currentPositionValue = ccwleftLimit;
    goToValue = ccwleftLimit;
    currentPostitionDeg =180; // TODo: is adjust if lock is not in the south at 180deg
    goToDeg = 180;
    calculateFactorAndMiddle();
#endif

#ifdef DEBUG
    Serial.print("MiddleValue ");
    Serial.println(middle);
    Serial.print("Factor ");
    Serial.println(calculationFactorDeg, 16);
#endif


/* other type of smoothing
#ifndef SIMULATION
    int counter = 0;
    while (counter < 30) 
    {
        ADCFilter.SetCurrent(ADCFilter.Current());
        readPotiAndSetCurrentDegCurrentValue();
        Serial.println(currentPositionValue);
        counter++;
        delay(5);
    }
#endif
//word */
} 

#ifdef WITH_LCD
void setLCDValues(){

        lcd.setCursor(5, 0);
        lcd.print("      ");
        lcd.setCursor(5, 0);
        lcd.print(currentPostitionDeg);
        lcd.setCursor(12, 0);
        lcd.print(goToDeg);
        lcd.setCursor(5, 1);
        lcd.print("      ");
        lcd.setCursor(5, 1);
        lcd.print(currentPositionValue);
}
#endif

void loop()
{

    if (isTurningActionCalled == true && currentPostitionDeg != goToDeg)
    {
#ifdef DEBUG
        Serial.print("Ich muss drehen!");
#endif
        goToValue = GetValueByDegSouth(goToDeg);
        int turningDirection = getTurningDirectionByValue(goToValue);
        isStopped = false;
        if (turningDirection == 0)
            turnCCW();
        else
            turnCW();
    }
    else
    {
#ifndef SIMULATION
        
        readPotiAndSetCurrentDegCurrentValue();
#endif
        WebServer();
    }

#ifdef WITH_LCD
    unsigned long currentMillis = millis();
 
  if(currentMillis - previousMillis > interval) {
    // save the last time you blinked the LED 
    previousMillis = currentMillis;   
        setLCDValues();
 
     }
#endif
}