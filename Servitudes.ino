#include <EEPROM.h>
#include <Bounce2.h>
#include <TimerOne.h>

#include "servitudes.h"

#include <Wire.h>

Servitudes* servitudes;

void setup()
{
  // put your setup code here, to run once:
  Serial.begin(115200);
  servitudes = new Servitudes();
  servitudes->init();
}

void loop()
{
  // put your main code here, to run repeatedly:
  servitudes->gerer();
}
