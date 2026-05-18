//GROUP 56
//MEMBERS: Uvensis Martinez

#include <DHT.h>
#include <LiquidCrystal.h>

#define DHTPIN 12
#define DHTTYPE DHT11
#define ON 2
#define OFFBUTTON 3
#define RESET 8
#define HIGHTEMP 24
#define DANGERTEMP 30

LiquidCrystal lcd(22, 23, 4, 5, 6, 7);
DHT dht(DHTPIN, DHTTYPE);

enum State
{
    OFF,
    IDLE,
    ACTIVE,
    ERROR
};

State currentState = OFF;
State previousState = ERROR;

unsigned long previousMillis = 0;
const unsigned long interval = 2000;

// debounce
unsigned long lastButtonPress = 0;
const unsigned long debounceDelay = 250;

// UART timestamp timer
unsigned long startTime = 0;

volatile bool systemEnabled = false;
volatile bool startPressed = false;

//LEDs
volatile unsigned char *portB = (unsigned char *)0x25;
volatile unsigned char *ddrB  = (unsigned char *)0x24;

// buzzer/reset
volatile unsigned char *portH = (unsigned char *)0x102;
volatile unsigned char *ddrH  = (unsigned char *)0x101;
volatile unsigned char *pinH  = (unsigned char *)0x100;

//ON/OFF 
volatile unsigned char *portE = (unsigned char *)0x2E;
volatile unsigned char *ddrE  = (unsigned char *)0x2D;
volatile unsigned char *pinE  = (unsigned char *)0x2C;

//OFF LED pin 24
volatile unsigned char *portA = (unsigned char *)0x22;
volatile unsigned char *ddrA  = (unsigned char *)0x21;


volatile unsigned char *ubrr0h = (unsigned char *)0xC5;
volatile unsigned char *ubrr0l = (unsigned char *)0xC4;
volatile unsigned char *ucsr0a = (unsigned char *)0xC0;
volatile unsigned char *ucsr0b = (unsigned char *)0xC1;
volatile unsigned char *ucsr0c = (unsigned char *)0xC2;
volatile unsigned char *udr0   = (unsigned char *)0xC6;


void uartInit(){
    *ubrr0h = 0;
    *ubrr0l = 103;

    *ucsr0b = (1 << 3);

    *ucsr0c = (1 << 1) | (1 << 2);}

void uartSendChar(char c){
    while(((*ucsr0a) & (1 << 5)) == 0);
    
    *udr0 = c;}

void uartPrint(const char *str){
    while(*str){
        uartSendChar(*str);
        str++;}
}

void uartPrintTime(){
    unsigned long totalSeconds =
        (millis() - startTime) / 1000;

    unsigned int hours =
        totalSeconds / 3600;

    unsigned int minutes =
        (totalSeconds % 3600) / 60;

    unsigned int seconds =
        totalSeconds % 60;

    // HOURS

    if(hours < 10){
        uartSendChar('0');}

    uartSendChar((hours / 10) + '0');
    uartSendChar((hours % 10) + '0');

    uartSendChar(':');

    // MINUTES

    if(minutes < 10){
        uartSendChar('0'); }

    uartSendChar((minutes / 10) + '0');
    uartSendChar((minutes % 10) + '0');
    uartSendChar(':');
    
    // SECONDS
    if(seconds < 10) {
        uartSendChar('0');
    }

    uartSendChar((seconds / 10) + '0');
    uartSendChar((seconds % 10) + '0');

    uartPrint(" - ");
}

void uartLog(const char *message){
    uartPrintTime();

    uartPrint(message);

    uartPrint("\r\n");
}


void startSystem(){
    startPressed = true;
}

// IDLE LED
void greenOn()  { *portB |= (1 << 4); }
void greenOff() { *portB &= ~(1 << 4); }

// ACTIVE LED
void yellowOn()  { *portB |= (1 << 5); }
void yellowOff() { *portB &= ~(1 << 5); }

// ERROR LED
void redOn()  { *portB |= (1 << 7); }
void redOff() { *portB &= ~(1 << 7); }

// OFF LED (PIN 24 = PA2)
void blueOn()  { *portA |= (1 << 2); }
void blueOff() { *portA &= ~(1 << 2); }
//buzzer 4 alarm
void buzzerOn()  { *portH |= (1 << 6); }
void buzzerOff() { *portH &= ~(1 << 6); }

//lcd
void updateLCD(const char* line1, float temp, float hum)
{
    lcd.clear();

    lcd.setCursor(0,0);
    lcd.print(line1);

    lcd.setCursor(0,1);

    lcd.print(temp);
    lcd.print(" C ");

    lcd.print(hum);
    lcd.print("%");
}

void setup()
{
    lcd.begin(16,2);

    dht.begin();

    uartInit();

    startTime = millis();

//LED
    *ddrB |= (1 << 4);
    *ddrB |= (1 << 5);
    *ddrB |= (1 << 7);

// OFF LED pin 24 = PA2
    *ddrA |= (1 << 2);
    
// BUZZER OUTPUT
    *ddrH |= (1 << 6);
    
// BUTTON INPUTS

// START BUTTON
    *ddrE &= ~(1 << 4);
    *portE |= (1 << 4);

// OFF BUTTON
    *ddrE &= ~(1 << 5);
    *portE |= (1 << 5);

// RESET BUTTON
    *ddrH &= ~(1 << 5);
    *portH |= (1 << 5);

greenOff();
yellowOff();
redOff();
blueOn();
buzzerOff();

    lcd.clear();
    lcd.print("SYSTEM OFF");

    uartLog("SYSTEM Started");
    
    attachInterrupt(
        digitalPinToInterrupt(ON),
        startSystem,
        FALLING
    );
}

void loop(){
    unsigned long currentMillis = millis();

    if(startPressed)
    {
        startPressed = false;

        if(currentState == OFF)
        {
            systemEnabled = true;

            currentState = IDLE;

            uartLog("SYSTEM STARTED");
        }
    }

    if(((*pinE) & (1 << 5)) == 0)
    {
        if(currentMillis - lastButtonPress > debounceDelay)
        {
            lastButtonPress = currentMillis;

            systemEnabled = false;

            currentState = OFF;

            uartLog("SYSTEM OFF");
        }
    }

//rexet
    if(((*pinH) & (1 << 5)) == 0)
    {
        if(currentMillis - lastButtonPress > debounceDelay)
        {
            lastButtonPress = currentMillis;

            systemEnabled = true;

            currentState = IDLE;

            previousState = OFF;

            buzzerOff();

            uartLog("RESET PRESSED/n");
            uartLog("Entering The Idle State");
        }
    }
//error lock
if(systemEnabled == false &&
       currentState != OFF){
        currentState = OFF; }
if(currentState == OFF &&
       previousState != OFF)
    {
        previousState = OFF;

        blueOn();

        greenOff();
        yellowOff();
        redOff();

        buzzerOff();

        lcd.clear();
        lcd.print("SYSTEM OFF");

        uartLog("System OFFLINE");
    }

    if(currentMillis - previousMillis >= interval)
    {
        previousMillis = currentMillis;

     
        if(currentState != OFF)
        {
            float humidity = dht.readHumidity();
            float temperature = dht.readTemperature();

   
            if(currentState != ERROR)
            {
                if(isnan(humidity) || isnan(temperature))
                {
                    currentState = ERROR;
                }

                else
                {
                    if(temperature >= DANGERTEMP)
                    {
                        currentState = ACTIVE;
                    }
                    else
                    {
                        currentState = IDLE;
                    }
                }
            }

          
            if(currentState != previousState)
            {
                previousState = currentState;

                switch(currentState)
                {
                    case IDLE:

                        blueOff();

                        greenOn();

                        yellowOff();
                        redOff();

                        buzzerOff();

                        updateLCD(
                            "SAFE",
                            temperature,
                            humidity
                        );

                        uartLog("IDLE STATE (Monitoring Terrerium)");

                    break;

                    case ACTIVE:

                        blueOff();

                        greenOff();

                        yellowOn();

                        redOff();

                        buzzerOn();

                        updateLCD(
                            "DANGER!",
                            temperature,
                            humidity
                        );

                        uartLog("ACTIVE STATE");

                    break;

                    case ERROR:

                        blueOff();

                        greenOff();
                        yellowOff();

                        redOn();

                        buzzerOff();

                        lcd.clear();

                        lcd.setCursor(0,0);
                        lcd.print("ERROR");

                        lcd.setCursor(0,1);
                        lcd.print("Sensor ERROR");

                        uartLog("EREROR ");

                    break;

                    default:
                    break;
                }
            }
        }
    }
}
