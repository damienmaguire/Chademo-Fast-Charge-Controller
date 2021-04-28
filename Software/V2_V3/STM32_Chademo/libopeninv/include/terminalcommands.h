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
#ifndef TERMINALCOMMANDS_H
#define TERMINALCOMMANDS_H


class TerminalCommands
{
   public:
      static void ParamSet(Terminal* term, char* arg);
      static void ParamGet(Terminal* term, char *arg);
      static void ParamFlag(Terminal* term, char *arg);
      static void ParamStream(Terminal* term, char *arg);
      static void PrintParamsJson(Terminal* term, char *arg);
      static void MapCan(Terminal* term, char *arg);
      static void SaveParameters(Terminal* term, char *arg);
      static void LoadParameters(Terminal* term, char *arg);
      static void Reset(Terminal* term, char *arg);

   protected:

   private:
      static void PrintCanMap(Param::PARAM_NUM param, int canid, int offset, int length, s32fp gain, bool rx);
};

#endif // TERMINALCOMMANDS_H
