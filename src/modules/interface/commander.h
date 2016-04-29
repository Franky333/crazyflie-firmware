/**
 *    ||          ____  _ __  ______
 * +------+      / __ )(_) /_/ ____/_________ _____  ___
 * | 0xBC |     / __  / / __/ /    / ___/ __ `/_  / / _	\
 * +------+    / /_/ / / /_/ /___ / /  / /_/ / / /_/  __/
 *  ||  ||    /_____/_/\__/\____//_/   \__,_/ /___/\___/
 *
 * Crazyflie control firmware
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
 */

#ifndef COMMANDER_H_
#define COMMANDER_H_
#include <stdint.h>
#include <stdbool.h>
#include "config.h"
#include "stabilizer_types.h"

#ifdef PLATFORM_CF1
  #define DEFUALT_YAW_MODE  PLUSMODE
#else
  #define DEFUALT_YAW_MODE  XMODE
#endif

#define COMMANDER_WDT_TIMEOUT_STABILIZE  M2T(500)
#define COMMANDER_WDT_TIMEOUT_SHUTDOWN   M2T(2000)

/**
 * CRTP commander data struct
 */
struct CommanderCrtpValues
{
  float roll;
  float pitch;
  float yaw;
  uint16_t thrust;
} __attribute__((packed));

void commanderInit(void);
bool commanderTest(void);
uint32_t commanderGetInactivityTime(void);

void commanderExtrxSet(struct CommanderCrtpValues* val);

void commanderGetSetpoint(setpoint_t *setpoint, const state_t *state);

void commanderGetPositionControl(bool* positionControl, bool* setPositionControl);
void commanderGetPositionControlNoSet(bool* positionControl);
void commanderSetPositionControl(bool positionControl);
void commanderGetTakeoff(bool* takeoff, bool* setTakeoff);
void commanderGetTakeoffNoSet(bool* takeoff);
void commanderSetTakeoff(bool takeoff);
void commanderGetLanding(bool* landing, bool* setLanding);
void commanderGetLandingNoSet(bool* landing);
void commanderSetLanding(bool landing);
void commanderGetManualOverride(bool *manOverride);

#endif /* COMMANDER_H_ */