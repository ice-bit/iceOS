/*****************************************
 *          VulcanOS Kernel              *
 * Developed by Marco 'icebit' Cetica    * 
 *            (c) 2019-2021              *
 *       Released under GPLv3            *
 * https://github.com/ice-bit/vulcanos   *
 *****************************************/
#ifndef _STRING_H_
#define _STRING_H_

#include <stdint.h> // For uinx_t
#include <stddef.h> // For size_t

int32_t strcmp(const uint8_t *s1, const uint8_t *s2);
uint8_t *itoa(int32_t val, uint8_t *buf, uint32_t radix);
uint8_t *uitoa(uint32_t val, uint8_t *buf, uint32_t radix);
size_t strlen(const uint8_t *buf);
uint8_t *strcpy(uint8_t *dst, const uint8_t *src);
void strcat(void *dest, const void *src);
void *memset(void *s, uint32_t c, size_t n);
void *memmove(void *dst, const void *src, size_t len);
void *memcpy(void *dst, void const *src, uint32_t n);
void strupper(uint8_t *str);
void strlower(uint8_t *str);

#endif
