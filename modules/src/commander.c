/**
 *    ||          ____  _ __
 * +------+      / __ )(_) /_______________ _____  ___
 * | 0xBC |     / __  / / __/ ___/ ___/ __ `/_  / / _ \
 * +------+    / /_/ / / /_/ /__/ /  / /_/ / / /_/  __/
 *  ||  ||    /_____/_/\__/\___/_/   \__,_/ /___/\___/
 *
 * Crazyflie Firmware
 *
 * Copyright (C) 2011-2012 Bitcraze AB
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, in version 3.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 *
 *
 */
#include "FreeRTOS.h"
#include "task.h"

#include "commander.h"
#include "crtp.h"
#include "configblock.h"
#include "log.h"
#include "param.h"

#define MIN_THRUST  10000
#define MAX_THRUST  60000

struct CommanderCrtpValues
{
  float roll;
  float pitch;
  float yaw;
  uint16_t thrust;
} __attribute__((packed));

static struct CommanderCrtpValues targetVal[2];
static bool isInit;
static int side=0;
static uint32_t lastUpdate;
static bool isInactive;
static bool altHoldMode = false;
static bool altHoldModeOld = false;

static bool positionControlMode = false;
static bool positionControlModeOld = false;
static bool takeoffMode = false;
static bool takeoffModeOld = false;
static bool landingMode = false;
static bool landingModeOld = false;
static bool manualOverrideMode = false;

static void commanderCrtpCB(CRTPPacket* pk);
static void commanderWatchdogReset(void);

void commanderInit(void)
{
  if(isInit)
    return;


  crtpInit();
  crtpRegisterPortCB(CRTP_PORT_COMMANDER, commanderCrtpCB);

  lastUpdate = xTaskGetTickCount();
  isInactive = true;
  isInit = true;
}

bool commanderTest(void)
{
  crtpTest();
  return isInit;
}

static void commanderCrtpCB(CRTPPacket* pk)
{
  targetVal[!side] = *((struct CommanderCrtpValues*)pk->data);
  side = !side;
  commanderWatchdogReset();
}

void commanderWatchdog(void)
{
  int usedSide = side;
  uint32_t ticktimeSinceUpdate;

  ticktimeSinceUpdate = xTaskGetTickCount() - lastUpdate;

  if (ticktimeSinceUpdate > COMMANDER_WDT_TIMEOUT_STABALIZE)
  {
    targetVal[usedSide].roll = 0;
    targetVal[usedSide].pitch = 0;
    targetVal[usedSide].yaw = 0;
  }
  if (ticktimeSinceUpdate > COMMANDER_WDT_TIMEOUT_SHUTDOWN)
  {
    targetVal[usedSide].thrust = 0;
    altHoldMode = false; // do we need this? It would reset the target altitude upon reconnect if still hovering
    isInactive = true;
  }
  else
  {
    isInactive = false;
  }
}

static void commanderWatchdogReset(void)
{
  lastUpdate = xTaskGetTickCount();
}

uint32_t commanderGetInactivityTime(void)
{
  return xTaskGetTickCount() - lastUpdate;
}

void commanderGetRPY(float* eulerRollDesired, float* eulerPitchDesired, float* eulerYawDesired)
{
  int usedSide = side;

  *eulerRollDesired  = targetVal[usedSide].roll;
  *eulerPitchDesired = targetVal[usedSide].pitch;
  *eulerYawDesired   = targetVal[usedSide].yaw;
}

void commanderGetAltHold(bool* altHold, bool* setAltHold, float* altHoldChange)
{
  *altHold = altHoldMode; // Still in altitude hold mode
  *setAltHold = !altHoldModeOld && altHoldMode; // Hover just activated
  *altHoldChange = altHoldMode ? ((float) targetVal[side].thrust - 32767.) / 32767. : 0.0; // Amount to change altitude hold target
  altHoldModeOld = altHoldMode;
}


void commanderGetRPYType(RPYType* rollType, RPYType* pitchType, RPYType* yawType)
{
  *rollType  = ANGLE;
  *pitchType = ANGLE;
  *yawType   = RATE;
}

void commanderGetThrust(uint16_t* thrust)
{
  int usedSide = side;
  uint16_t rawThrust = targetVal[usedSide].thrust;

  if (rawThrust > MIN_THRUST)
  {
    *thrust = rawThrust;
  }
  else
  {
    *thrust = 0;
  }

  if (rawThrust > MAX_THRUST)
  {
    *thrust = MAX_THRUST;
  }

  commanderWatchdog();
}


void commanderGetPositionControl(bool* positionControl, bool* setPositionControl)
{
	*positionControl = positionControlMode; // Still in positionControl mode
	*setPositionControl = !positionControlModeOld && positionControlMode; // positionControl just activated
	positionControlModeOld = positionControlMode;
}

void commanderGetPositionControlNoSet(bool* positionControl) //Todo: just return value
{
	*positionControl = positionControlMode; // Still in positionControl mode
}

void commanderSetPositionControl(bool positionControl)
{
	positionControlMode = positionControl;
}

void commanderGetTakeoff(bool* takeoff, bool* setTakeoff)
{
	*takeoff = takeoffMode; // Still in takeoff mode
	*setTakeoff = !takeoffModeOld && takeoffMode; //takeoff just activated
	takeoffModeOld = takeoffMode;
}

void commanderGetTakeoffNoSet(bool* takeoff) //Todo: just return value
{
	*takeoff = takeoffMode;
}

void commanderSetTakeoff(bool takeoff)
{
	takeoffMode = takeoff;
}

void commanderGetLanding(bool* landing, bool* setLanding)
{
	*landing = landingMode; // Still in landing mode
	*setLanding = !landingModeOld && landingMode; //landing just activated
	landingModeOld = landingMode;
}

void commanderGetLandingNoSet(bool* landing) //Todo: just return value
{
	*landing = landingMode;
}

void commanderSetLanding(bool landing)
{
	landingMode = landing;
}

void commanderGetManualOverride(bool *manOverride) //Todo: just return value
{
	*manOverride = manualOverrideMode;
}


// logs for flight modes (for when the firmware changes flightmode)
LOG_GROUP_START(flightmode)
LOG_ADD(LOG_UINT8, althold, &altHoldMode)
LOG_ADD(LOG_UINT8, posCtrl, &positionControlMode)
LOG_ADD(LOG_UINT8, takeoff, &takeoffMode)
LOG_ADD(LOG_UINT8, landing, &landingMode)
LOG_ADD(LOG_UINT8, manOvrd, &manualOverrideMode)
LOG_GROUP_STOP(flightmode)

// Params for flight modes
PARAM_GROUP_START(flightmode)
PARAM_ADD(PARAM_UINT8, althold, &altHoldMode)
PARAM_ADD(PARAM_UINT8, posCtrl, &positionControlMode)
PARAM_ADD(PARAM_UINT8, takeoff, &takeoffMode)
PARAM_ADD(PARAM_UINT8, landing, &landingMode)
PARAM_ADD(PARAM_UINT8, manOvrd, &manualOverrideMode)
PARAM_GROUP_STOP(flightmode)
