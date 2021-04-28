/*
 * This file is part of the libopeninv project.
 *
 * Copyright (C) 2018 Johannes Huebner <dev@johanneshuebner.com>
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
#ifndef STM32_LOADER_H_INCLUDED
#define STM32_LOADER_H_INCLUDED
#include <stdint.h>

#define PINDEF_ADDRESS 0x0801F400
#define NUM_PIN_COMMANDS 10
#define PIN_IN 0
#define PIN_OUT 1

struct pindef
{
   uint32_t port;
   uint16_t pin;
   uint8_t inout;
   uint8_t level;
};

struct pincommands
{
   struct pindef pindef[NUM_PIN_COMMANDS];
   uint32_t crc;
};

#define PINDEF_NUMWORDS (sizeof(struct pindef) * NUM_PIN_COMMANDS / 4)


#endif // STM32_LOADER_H_INCLUDED
