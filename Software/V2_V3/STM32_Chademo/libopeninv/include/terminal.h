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
#ifndef TERMINAL_H
#define TERMINAL_H
#include <stdint.h>
#include "printf.h"

class Terminal;

typedef struct
{
   char const *cmd;
   void (*CmdFunc)(Terminal*, char*);
} TERM_CMD;

class Terminal: public IPutChar
{
public:
   Terminal(uint32_t usart, const TERM_CMD* commands, bool remap = false);
   void SetNodeId(uint8_t id) { nodeId = id; }
   void Run();
   void PutChar(char c);
   bool KeyPressed();
   void FlushInput();
   void DisableTxDMA();
   static Terminal* defaultTerminal;

private:
   struct HwInfo
   {
      uint32_t usart;
      uint8_t dmatx;
      uint8_t dmarx;
      uint32_t port;
      uint16_t pin;
      uint32_t port_re;
      uint16_t pin_re;
   };

   void ResetDMA();
   const TERM_CMD *CmdLookup(char *buf);
   void EnableUart(char* arg);
   void FastUart(char* arg);
   void Send(const char *str);

   static const int bufSize = 128;
   static const HwInfo hwInfo[];
   const HwInfo* hw;
   uint32_t usart;
   bool remap;
   const TERM_CMD* termCmds;
   uint8_t nodeId;
   bool enabled;
   bool txDmaEnabled;
   const TERM_CMD *pCurCmd;
   int lastIdx;
   uint8_t curBuf;
   uint32_t curIdx;
   bool firstSend;
   char inBuf[bufSize];
   char outBuf[2][bufSize]; //double buffering
   char args[bufSize];
};

#endif // TERMINAL_H
