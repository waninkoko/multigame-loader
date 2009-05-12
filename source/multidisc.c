#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>
#include <ogcsys.h>

#include "multidisc.h"
#include "wdvd.h"

/* Constants */
#define MAGIC_WORD	0x4D474C57

/* Variables */
static u32 buffer[0x20] ATTRIBUTE_ALIGN(32);


s32 Multidisc_GetEntries(struct wiigame **outbuf, u32 *outlen)
{
	struct wiigame *gameBuffer = NULL;

	u32 cnt;
	s32 ret;

	/* Read disc header */
	ret = WDVD_UnencryptedRead(buffer, sizeof(buffer), 0); 
	if (ret < 0)
		return ret;

	/* Check disc ID */
	if (buffer[0] != MAGIC_WORD)
		return -1;

	/* Number of entries */
	cnt = buffer[1];

	/* Entries found */
	if (cnt) {
		u32 len = sizeof(struct wiigame) * cnt;

		/* Allocate memory */
		gameBuffer = (struct wiigame *)memalign(32, len);
		if (!gameBuffer)
			return -2;

		/* Read entries */
		ret = WDVD_UnencryptedRead(gameBuffer, len, 8);
		if (ret < 0)
			goto err;
	} else
		return -3;

	/* Set values */
	*outbuf = gameBuffer;
	*outlen = cnt;

	return 0;

err:
	/* Free memory */
	if (gameBuffer)
		free(gameBuffer);

	return ret;
}
