/*
 * This file is part of the libopeninv project.
 *
 * Copyright (C) 2016 Nail Güzel
 * Johannes Huebner <dev@johanneshuebner.com>
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
#include "fu.h"

uint32_t MotorVoltage::boost = 0;
u32fp MotorVoltage::fac;
uint32_t MotorVoltage::maxAmp;
u32fp MotorVoltage::endFrq = 1; //avoid division by 0 when not set

/** Set 0 Hz boost to overcome winding resistance */
void MotorVoltage::SetBoost(uint32_t boost /**< amplitude in digit */)
{
   MotorVoltage::boost = boost;
   CalcFac();
}

/** Set frequency where the full amplitude is to be provided */
void MotorVoltage::SetWeakeningFrq(u32fp frq)
{
   endFrq = frq;
   CalcFac();
}

/** Get amplitude for a given frequency */
uint32_t MotorVoltage::GetAmp(u32fp frq)
{
   return MotorVoltage::GetAmpPerc(frq, FP_FROMINT(100));
}

/** Get amplitude for given frequency multiplied with given percentage */
uint32_t MotorVoltage::GetAmpPerc(u32fp frq, u32fp perc)
{
   uint32_t amp = FP_MUL(perc, (FP_TOINT(FP_MUL(fac, frq)) + boost)) / 100;
   if (frq < FP_FROMFLT(0.2))
   {
      amp = 0;
   }
   if (amp > maxAmp)
   {
      amp = maxAmp;
   }

   return amp;
}

void MotorVoltage::SetMaxAmp(uint32_t maxAmp)
{
   MotorVoltage::maxAmp = maxAmp;
   CalcFac();
}

/** Calculate slope of u/f */
void MotorVoltage::CalcFac()
{
   fac = FP_DIV(FP_FROMINT(maxAmp - boost), endFrq);
}
