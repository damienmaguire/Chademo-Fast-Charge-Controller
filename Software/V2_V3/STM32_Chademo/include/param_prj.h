/*
 * This file is part of the stm32-template project.
 *
 * Copyright (C) 2020 Johannes Huebner <dev@johanneshuebner.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

/* This file contains all parameters used in your project
 * See main.cpp on how to access them.
 * If a parameters unit is of format "0=Choice, 1=AnotherChoice" etc.
 * It will be displayed as a dropdown in the web interface
 * If it is a spot value, the decimal is translated to the name, i.e. 0 becomes "Choice"
 * If the enum values are powers of two, they will be displayed as flags, example
 * "0=None, 1=Flag1, 2=Flag2, 4=Flag3, 8=Flag4" and the value is 5.
 * It means that Flag1 and Flag3 are active -> Display "Flag1 | Flag3"
 *
 * Every parameter/value has a unique ID that must never change. This is used when loading parameters
 * from flash, so even across firmware versions saved parameters in flash can always be mapped
 * back to our list here. If a new value is added, it will receive its default value
 * because it will not be found in flash.
 * The unique ID is also used in the CAN module, to be able to recover the CAN map
 * no matter which firmware version saved it to flash.
 * Make sure to keep track of your ids and avoid duplicates. Also don't re-assign
 * IDs from deleted parameters because you will end up loading some random value
 * into your new parameter!
 * IDs are 16 bit, so 65535 is the maximum
 */

 //Define a version string of your firmware here
#define VER 1.00.R

/* Entries must be ordered as follows:
   1. Saveable parameters (id != 0)
   2. Temporary parameters (id = 0)
   3. Display values
 */
//Next param id (increase when adding new parameter!): 3
//Next value Id: 2010
/*              category     name         unit       min     max     default id */
#define PARAM_LIST \
    PARAM_ENTRY(CAT_POWER,    chargelimit, "A",       0,      255,    255,    10  ) \
    PARAM_ENTRY(CAT_POWER,    soclimit,    "%",       0,      100,    100,    12  ) \
    PARAM_ENTRY(CAT_CONTACT,  udcthresh,   "V",       0,      500,    380,    92  ) \
    VALUE_ENTRY(opmode,      OPMODES, 2000 ) \
    VALUE_ENTRY(cdmstatus,    CDMSTAT, 2070 ) \
    VALUE_ENTRY(version,     VERSTR,  2001 ) \
    VALUE_ENTRY(soc,          "%",     2052 ) \
    VALUE_ENTRY(batfull,      ONOFF,   2069 ) \
    VALUE_ENTRY(cdmcureq,    "A",     2076 ) \
    VALUE_ENTRY(chgcurlim,    "A",     2066 ) \
    VALUE_ENTRY(idccdm,       "A",     2067 ) \
    VALUE_ENTRY(udccdm,       "V",     2068 ) \
    VALUE_ENTRY(udcbms,       "V",     2048 ) \
    VALUE_ENTRY(lasterr,     errorListString,  2002 ) \
    VALUE_ENTRY(tmpisa,       "°C",    2005 ) \
    VALUE_ENTRY(cpuload,     "%",     2004 )    \
    VALUE_ENTRY(IsaIdc,     "A",     2006 ) \
    VALUE_ENTRY(power,     "kw",     2007 ) \
    VALUE_ENTRY(KWh,     "KWh",     2008 )  \
    VALUE_ENTRY(AMPh,     "Ah",     2009 )


/***** Enum String definitions *****/
#define OPMODES      "0=Off, 1=ChargeStart, 2=ConnectorLock, 3=Charge, 4=ChargeStop"
#define CDMSTAT      "1=Charging, 2=Malfunction, 4=ConnLock, 8=BatIncomp, 16=SystemMalfunction, 32=Stop"
#define CANSPEEDS    "0=250k, 1=500k, 2=800k, 3=1M"
#define CANPERIODS   "0=100ms, 1=10ms"
#define ONOFF        "0=Off, 1=On, 2=na"
#define CAT_POWER    "Power Limit"
#define CAT_CONTACT  "Contactor Control"

#define VERSTR STRINGIFY(4=VER-name)

/***** enums ******/


enum _canspeeds
{
   CAN_PERIOD_100MS = 0,
   CAN_PERIOD_10MS,
   CAN_PERIOD_LAST
};

enum _modes
{
   MOD_OFF = 0,
   MOD_CHARGESTART,
   MOD_CHARGELOCK,
   MOD_CHARGE,
   MOD_CHARGEND
};

//Generated enum-string for possible errors
extern const char* errorListString;

