/*
 * This file is part of the libopeninv project.
 *
 * Copyright (C) 2011 Johannes Huebner <dev@johanneshuebner.com>
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
#ifndef FU_H_INCLUDED
#define FU_H_INCLUDED

#include <stdint.h>
#include "my_fp.h"

class MotorVoltage
{
public:
   static void SetBoost(uint32_t boost);
   static void SetWeakeningFrq(u32fp frq);
   static void SetMaxAmp(uint32_t maxAmp);
   static uint32_t GetAmp(u32fp frq);
   static uint32_t GetAmpPerc(u32fp frq, u32fp perc);

private:
   static void CalcFac();
   static uint32_t boost;
   static u32fp fac;
   static uint32_t maxAmp;
   static u32fp endFrq;
   static u32fp maxFrq;
};

#endif // FU_H_INCLUDED
