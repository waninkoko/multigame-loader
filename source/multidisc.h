#ifndef _MULTIDISC_H_
#define _MULTIDISC_H_

#include "disc.h"

/* Wii Game structure */
struct wiigame {
	/* Game header */
	struct discHdr header;

	/* Game offset */
	u64 offset;

	/* Game size */
	u64 size;

	/* Padding */
	u8 padding[16];
} ATTRIBUTE_PACKED;


/* Prototypes */
s32 Multidisc_GetEntries(struct wiigame **, u32 *);

#endif
