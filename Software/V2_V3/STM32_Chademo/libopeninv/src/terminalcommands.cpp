/*
 * This file is part of the libopeninv project.
 *
 * Copyright (C) 2021 Johannes Huebner <dev@johanneshuebner.com>
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
#include <libopencm3/cm3/scb.h>
#include "hwdefs.h"
#include "terminal.h"
#include "params.h"
#include "my_string.h"
#include "my_fp.h"
#include "printf.h"
#include "param_save.h"
#include "stm32_can.h"
#include "terminalcommands.h"

static Terminal* curTerm = NULL;

void TerminalCommands::ParamSet(Terminal* term, char* arg)
{
   char *pParamVal;
   s32fp val;
   Param::PARAM_NUM idx;

   arg = my_trim(arg);
   pParamVal = (char *)my_strchr(arg, ' ');

   if (*pParamVal == 0)
   {
      fprintf(term, "No parameter value given\r\n");
      return;
   }

   *pParamVal = 0;
   pParamVal++;

   val = fp_atoi(pParamVal, FRAC_DIGITS);
   idx = Param::NumFromString(arg);

   if (Param::PARAM_INVALID != idx)
   {
       if (0 == Param::Set(idx, val))
       {
          fprintf(term, "Set OK\r\n");
       }
       else
       {
          fprintf(term, "Value out of range\r\n");
       }
   }
   else
   {
       fprintf(term, "Unknown parameter %s\r\n", arg);
   }
}

void TerminalCommands::ParamGet(Terminal* term, char* arg)
{
   Param::PARAM_NUM idx;
   s32fp val;
   char* comma;
   char orig;

   arg = my_trim(arg);

   do
   {
      comma = (char*)my_strchr(arg, ',');
      orig = *comma;
      *comma = 0;

      idx = Param::NumFromString(arg);

      if (Param::PARAM_INVALID != idx)
      {
         val = Param::Get(idx);
         fprintf(term, "%f\r\n", val);
      }
      else
      {
         fprintf(term, "Unknown parameter: '%s'\r\n", arg);
      }

      *comma = orig;
      arg = comma + 1;
   } while (',' == *comma);
}

void TerminalCommands::ParamFlag(Terminal* term, char *arg)
{
   char *pFlagVal;
   Param::PARAM_NUM idx;

   arg = my_trim(arg);
   pFlagVal = (char *)my_strchr(arg, ' ');

   if (*pFlagVal == 0)
   {
      fprintf(term, "No flag given\r\n");
      return;
   }

   *pFlagVal = 0;
   pFlagVal++;

   idx = Param::NumFromString(arg);

   if (Param::PARAM_INVALID != idx)
   {
      bool clearFlag = false;
      Param::PARAM_FLAG flag = Param::FLAG_NONE;

      if (pFlagVal[0] == '!' || pFlagVal[0] == '~' || pFlagVal[0] == '/')
      {
         clearFlag = true;
         pFlagVal++;
      }

      if (my_strcmp("hidden", pFlagVal) == 0)
      {
         flag = Param::FLAG_HIDDEN;
      }

      if (flag != Param::FLAG_NONE)
      {
         if (clearFlag)
         {
            Param::ClearFlag(idx, flag);
         }
         else
         {
            Param::SetFlag(idx, flag);
         }
         fprintf(term, "Flag change OK\r\n");
      }
      else
      {
         fprintf(term, "Unknown flag\r\n");
      }
   }
   else
   {
       fprintf(term, "Unknown parameter %s\r\n", arg);
   }
}

void TerminalCommands::ParamStream(Terminal* term, char *arg)
{
   Param::PARAM_NUM indexes[10];
   int maxIndex = sizeof(indexes) / sizeof(Param::PARAM_NUM);
   int curIndex = 0;
   int repetitions = -1;
   char* comma;
   char orig;

   arg = my_trim(arg);
   repetitions = my_atoi(arg);
   arg = (char*)my_strchr(arg, ' ');

   if (0 == *arg)
   {
      fprintf(term, "Usage: stream n val1,val2...\r\n");
      return;
   }
   arg++; //move behind space

   do
   {
      comma = (char*)my_strchr(arg, ',');
      orig = *comma;
      *comma = 0;

      Param::PARAM_NUM idx = Param::NumFromString(arg);

      *comma = orig;
      arg = comma + 1;

      if (Param::PARAM_INVALID != idx)
      {
         indexes[curIndex] = idx;
         curIndex++;
      }
      else
      {
         fprintf(term, "Unknown parameter\r\n");
      }
   } while (',' == *comma && curIndex < maxIndex);

   maxIndex = curIndex;
   term->FlushInput();

   while (!term->KeyPressed() && (repetitions > 0 || repetitions == -1))
   {
      comma = (char*)"";
      for (curIndex = 0; curIndex < maxIndex; curIndex++)
      {
         s32fp val = Param::Get(indexes[curIndex]);
         fprintf(term, "%s%f", comma, val);
         comma = (char*)",";
      }
      fprintf(term, "\r\n");
      if (repetitions != -1)
         repetitions--;
   }
}

void TerminalCommands::PrintParamsJson(Terminal* term, char *arg)
{
   arg = my_trim(arg);

   const Param::Attributes *pAtr;
   char comma = ' ';
   bool printHidden = arg[0] == 'h';

   fprintf(term, "{");
   for (uint32_t idx = 0; idx < Param::PARAM_LAST; idx++)
   {
      int canId, canOffset, canLength;
      bool isRx;
      s32fp canGain;
      pAtr = Param::GetAttrib((Param::PARAM_NUM)idx);

      if ((Param::GetFlag((Param::PARAM_NUM)idx) & Param::FLAG_HIDDEN) == 0 || printHidden)
      {
         fprintf(term, "%c\r\n   \"%s\": {\"unit\":\"%s\",\"value\":%f,",comma, pAtr->name, pAtr->unit, Param::Get((Param::PARAM_NUM)idx));

         if (Can::GetInterface(0)->FindMap((Param::PARAM_NUM)idx, canId, canOffset, canLength, canGain, isRx))
         {
            fprintf(term, "\"canid\":%d,\"canoffset\":%d,\"canlength\":%d,\"cangain\":%d,\"isrx\":%s,",
                   canId, canOffset, canLength, canGain, isRx ? "true" : "false");
         }

         if (Param::IsParam((Param::PARAM_NUM)idx))
         {
            fprintf(term, "\"isparam\":true,\"minimum\":%f,\"maximum\":%f,\"default\":%f,\"category\":\"%s\",\"i\":%d}",
                   pAtr->min, pAtr->max, pAtr->def, pAtr->category, idx);
         }
         else
         {
            fprintf(term, "\"isparam\":false}");
         }
         comma = ',';
      }
   }
   fprintf(term, "\r\n}\r\n");
}

//cantx param id offset len gain
void TerminalCommands::MapCan(Terminal* term, char *arg)
{
   Param::PARAM_NUM paramIdx = Param::PARAM_INVALID;
   int values[4];
   int result;
   char op;
   char *ending;
   const int numArgs = 4;

   arg = my_trim(arg);

   if (arg[0] == 'p')
   {
      while (curTerm != NULL); //lock
      curTerm = term;
      Can::GetInterface(0)->IterateCanMap(PrintCanMap);
      curTerm = NULL;
      return;
   }

   if (arg[0] == 'c')
   {
      Can::GetInterface(0)->Clear();
      fprintf(term, "All message definitions cleared\r\n");
      return;
   }

   op = arg[0];
   arg = (char *)my_strchr(arg, ' ');

   if (0 == *arg)
   {
      fprintf(term, "Missing argument\r\n");
      return;
   }

   arg = my_trim(arg);
   ending = (char *)my_strchr(arg, ' ');

   if (*ending == 0 && op != 'd')
   {
      fprintf(term, "Missing argument\r\n");
      return;
   }

   *ending = 0;
   paramIdx = Param::NumFromString(arg);
   arg = my_trim(ending + 1);

   if (Param::PARAM_INVALID == paramIdx)
   {
      fprintf(term, "Unknown parameter\r\n");
      return;
   }

   if (op == 'd')
   {
      result = Can::GetInterface(0)->Remove(paramIdx);
      fprintf(term, "%d entries removed\r\n", result);
      return;
   }

   for (int i = 0; i < numArgs; i++)
   {
      ending = (char *)my_strchr(arg, ' ');

      if (0 == *ending && i < (numArgs - 1))
      {
         fprintf(term, "Missing argument\r\n");
         return;
      }

      *ending = 0;
      int iVal = my_atoi(arg);

      //allow gain values < 1 and re-interpret them
      if (i == (numArgs - 1) && iVal == 0)
      {
         values[i] = fp_atoi(arg, 16);
         //The can values interprets abs(values) < 32 as gain and > 32 as divider
         //e.g. 0.25 means integer division by 4 so we need to calculate div = 1/value
         //0.25 with 16 decimals is 16384, 65536/16384 = 4
         values[i] = (32 << 16) / values[i];
      }
      else
      {
         values[i] = iVal;
      }

      arg = my_trim(ending + 1);
   }

   if (op == 't')
   {
      result = Can::GetInterface(0)->AddSend(paramIdx, values[0], values[1], values[2], values[3]);
   }
   else
   {
      result = Can::GetInterface(0)->AddRecv(paramIdx, values[0], values[1], values[2], values[3]);
   }

   switch (result)
   {
      case CAN_ERR_INVALID_ID:
         fprintf(term, "Invalid CAN Id %x\r\n", values[0]);
         break;
      case CAN_ERR_INVALID_OFS:
         fprintf(term, "Invalid Offset %d\r\n", values[1]);
         break;
      case CAN_ERR_INVALID_LEN:
         fprintf(term, "Invalid length %d\r\n", values[2]);
         break;
      case CAN_ERR_MAXITEMS:
         fprintf(term, "Cannot map anymore items to CAN id %d\r\n", values[0]);
         break;
      case CAN_ERR_MAXMESSAGES:
         fprintf(term, "Max message count reached\r\n");
         break;
      default:
         fprintf(term, "CAN map successful, %d message%s active\r\n", result, result > 1 ? "s" : "");
   }
}

void TerminalCommands::SaveParameters(Terminal* term, char *arg)
{
   arg = arg;
   uint32_t crc = parm_save();
   fprintf(term, "Parameters stored, CRC=%x\r\n", crc);
   Can::GetInterface(0)->Save();
   fprintf(term, "CANMAP stored\r\n");
}

void TerminalCommands::LoadParameters(Terminal* term, char *arg)
{
   arg = arg;
   if (0 == parm_load())
   {
      parm_Change((Param::PARAM_NUM)0);
      fprintf(term, "Parameters loaded\r\n");
   }
   else
   {
      fprintf(term, "Parameter CRC error\r\n");
   }
}

void TerminalCommands::Reset(Terminal* term, char *arg)
{
   term = term;
   arg = arg;
   scb_reset_system();
}

void TerminalCommands::PrintCanMap(Param::PARAM_NUM param, int canid, int offset, int length, s32fp gain, bool rx)
{
   const char* name = Param::GetAttrib(param)->name;
   fprintf(curTerm, "can ");

   if (rx)
      fprintf(curTerm, "rx ");
   else
      fprintf(curTerm, "tx ");
   fprintf(curTerm, "%s %d %d %d %d\r\n", name, canid, offset, length, gain);
}
