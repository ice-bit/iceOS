/*****************************************
 *          VulcanOS Kernel              *
 * Developed by Marco 'icebit' Cetica    * 
 *            (c) 2019-2021              *
 *       Released under GPLv3            *
 * https://github.com/ice-bit/vulcanos   *
 *****************************************/
#ifndef _SHELL_H_
#define _SHELL_H_

#include <stdint.h>

#define bit(n) (1 << (n))
#define check_flag(flags, n) ((flags) & bit(n))

void helper();
void processCommand(uint8_t *cmd);

#endif
