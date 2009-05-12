	.rodata

	.globl  bgImg
	.balign 32
bgImg:
	.incbin "../data/background"

	.globl  coverImg
	.balign 32
coverImg:
	.incbin "../data/nocover"



#include "asm.h"
#define UNSTUB_ADDR	0x3400

	.text
	.globl unstub_start
unstub_start:
	isync

	lis	r3, UNSTUB_ADDR@h
	ori	r3, r3, UNSTUB_ADDR@l
	mtsrr0	r3
	isync

	mfmsr	r3

	li	r4, 0x30
	andc	r3, r3, r4

	mtsrr1	r3
	isync

	rfi
