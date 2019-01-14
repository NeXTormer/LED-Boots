
#include <Wire.h>
#include <LSM303.h>
#include <FastLED.h>
#include <ButtonDebounce.h>

#define NUM_LEDS 43
#define DATA_PIN 7

/*
    Modes:
        0: change brightness
        1: Rotation=hue, step=complementary color
        2: Stepping effect: step=color which fades after some time
        3: select hue with potentiometer, step=complementary color
        4: step=changes between red and blue
        5: color wheel

*/

CRGB leds[NUM_LEDS];

LSM303 sensor1;
char report[80];

int last_ax = 0;
int last_ay = 0;
int last_az = 0;

int threshhold_a = 10000;
int threshhold_time = 130;

int hue = 0;
int mode6_delay = 60;

long last_btn_change = 0;
long last_change = 0;
long mode6_lastchange = 0;

byte brightness = 42;
byte hue_analog = 0;
byte current_mode = 1;
byte num_modes = 7;
byte mode5_currindex = 0;
byte mode5_hue = 0;
byte mode1_lastrotation = 0;

bool temp_color = false;
bool mode3_temp = false;
bool mode4_temp = false;
bool mode6_on = true;


void printReport();
void step();
void buttonPressed(const int state);
void nextMode();

void setup()
{
    Serial.begin(115200);
    Wire.begin();
    sensor1.init();
    sensor1.enableDefault();

    FastLED.addLeds<WS2812, DATA_PIN, RGB>(leds, NUM_LEDS);
    FastLED.setBrightness(42);

    pinMode(3, INPUT_PULLUP);
    pinMode(4, INPUT_PULLUP);

    Serial.println("Werner Findenig");
}

void loop()
{
    sensor1.read();
    //printReport();

    int x = sensor1.a.x;
    int y = sensor1.a.y;
    int z = sensor1.a.z;

    /* Detect change in direction */

    int dx = abs(x - last_ax);
    int dy = abs(y - last_ay);
    int dz = abs(z - last_az);

    hue_analog = map(analogRead(A0), 40, 800, 0,255);
    //Serial.println(analogRead(A0));

    if((x <= 0 && last_ax > 0) || (last_ax <= 0 && x > 0))
    {
        if(dx > threshhold_a)
        {
            step();
        }
    }

    if((y <= 0 && last_ay > 0) || (last_ay <= 0 && y > 0))
    {
        if(dy > threshhold_a)
        {
            step();
        }    
    }

    if((z <= 0 && last_az > 0) || (last_az <= 0 && z > 0))
    {
        if(dz > threshhold_a)
        {
            step();
        }    
    }

    // Serial.println(dx);
    // Serial.println(dy);
    // Serial.println(dz);
    // Serial.println("====");
    
    last_ax = x;
    last_ay = y;
    last_az = z;

    if(current_mode == 1)
    {
        

        hue = map(sensor1.m.y, -260, 60, 0, 255);
        /* Rotation */
        /*int rot = map(sensor1.m.y, -260, 60, 0, 255);

            mode1_lastrotation = rot;

        int rotdiff = mode1_lastrotation - rot;
        if(rotdiff > 10)
        {
            Serial.println(rotdiff);

            hue += rotdiff / 8;
            hue %= 256;
        }

        */

        if(temp_color)
        {
            for(int i = 0; i < NUM_LEDS; i++)
            {
                leds[i] = CHSV(abs(hue-127), 255, 255);
            }
        }
        else
        {
            for(int i = 0; i < NUM_LEDS; i++)
            {
                leds[i] = CHSV(hue, 255, 255);
            }
        }
    }
    else if(current_mode == 2)
    {
        for(int i = 0; i < NUM_LEDS; i++)
        {
            leds[i].nscale8(245);
        }
    }
    else if(current_mode == 3)
    {    
        for(int i = 0; i < NUM_LEDS; i++)
        {
            if(mode3_temp)
            {
                leds[i] = CHSV(hue_analog, 255, 255);
            }   
            else
            {
                leds[i] = CHSV(255 - hue_analog, 255, 255);
            } 
        }
    }
    else if(current_mode == 5)
    {
        mode5_hue = (mode5_hue + 1) % 255;
        leds[mode5_currindex] = CHSV(mode5_hue, 255, 255);
        
        mode5_currindex = (mode5_currindex + 1) % NUM_LEDS;

        for(int i = 0; i < NUM_LEDS; i++)
        {
            leds[i].nscale8(240);
        }
    }
    else if(current_mode == 0)
    {
        for(int i = 0; i < NUM_LEDS; i++)
        {
            leds[i] = CRGB(255, 255, 255);
            int analog = analogRead(A0);
            if(analog > 500) analog = 500;
            if(analog < 60) analog = 60;
            brightness = analog;
            FastLED.setBrightness(brightness);
        }
    }
    else if(current_mode == 6)
    {
        if(millis() - mode6_lastchange > mode6_delay)
        {
            mode6_on = !mode6_on;
            if(mode6_on)
            {
                for(int i = 0; i < NUM_LEDS; i++)
                {
                    leds[i] = CRGB(255, 255, 255);
                }
            }
            else
            {
                for(int i = 0; i < NUM_LEDS; i++)
                {
                    leds[i] = CRGB(0, 0, 0);
                }
            }
            mode6_lastchange = millis();
            //mode6_delay = map(analogRead(A0), 200, 800, 30, 400);
        }
    }

    FastLED.show();

    
    if(!digitalRead(3))
    {
        nextMode();
    }

    Serial.println(analogRead(A0));

    delay(10);
}

void step()
{
    if(current_mode == 1)
    {
        if(millis() - last_change > threshhold_time)
        {
            temp_color = !temp_color;
            Serial.println("changed color");
            last_change = millis();
        }
    }
    else if(current_mode == 2)
    {
        if(millis() - last_change > threshhold_time)
        {
            for(int i = 0; i < NUM_LEDS; i++)
            {
                leds[i] = CHSV(hue_analog, 255, 255);
            }
            
            
            Serial.println("stepped");
            last_change = millis();
        }
    }
    else if(current_mode == 3)
    {
        if(millis() - last_change > threshhold_time)
        {
            mode3_temp = !mode3_temp;
            
            for(int i = 0; i < NUM_LEDS; i++)
            {
                if(mode3_temp)
                {
                    leds[i] = CHSV(hue_analog, 255, 255);
                }   
                else
                {
                    leds[i] = CHSV(255 - hue_analog, 255, 255);
                } 
            }
            last_change = millis();
        }
    }
    else if(current_mode == 4)
    {
        if(millis() - last_change > threshhold_time)
        {
            for(int i = 0; i < NUM_LEDS; i++)
            {
                if(mode4_temp)
                {
                    leds[i] = CRGB::Red;
                }
                else
                {
                    leds[i] = CRGB::Blue;
                }
            }
            mode4_temp = !mode4_temp;
            
            
            Serial.println("stepped");
            last_change = millis();
        }
    }
}

void nextMode()
{
    if(millis() - last_btn_change > 500)
    {
        current_mode++;
        current_mode %= num_modes;
        Serial.println("Changed Mode");
        step();
        last_btn_change = millis();
    }
}


void printReport()
{
  Serial.println("REPORT");
  snprintf(report, sizeof(report), "A: %6d %6d %6d    M: %6d %6d %6d",
    sensor1.a.x, sensor1.a.y, sensor1.a.z,
    sensor1.m.x, sensor1.m.y, sensor1.m.z);
  
}
