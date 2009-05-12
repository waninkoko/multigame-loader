#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>
#include <ogcsys.h>
#include <fat.h>
#include <sys/stat.h>

/* Constants */
#define SD_MOUNT	"sd"

/* Disc interfaces */
extern const DISC_INTERFACE __io_wiisd;


s32 Fat_MountSD(void)
{
	s32 ret;

	/* Initialize SD interface */
	ret = __io_wiisd.startup();
	if (!ret)
		return -1;

	/* Mount device */
	ret = fatMountSimple(SD_MOUNT, &__io_wiisd);
	if (!ret)
		return -2;

	return 0;
}

s32 Fat_UnmountSD(void)
{
	s32 ret;

	/* Unmount device */
	fatUnmount(SD_MOUNT);

	/* Shutdown SD interface */
	ret = __io_wiisd.shutdown();
	if (!ret)
		return -1;

	return 0;
}

s32 Fat_ReadFile(const char *filepath, void **outbuf)
{
	FILE *fp     = NULL;
	void *buffer = NULL;

	struct stat filestat;
	u32         filelen;

	s32 ret;

	/* Get filestats */
	stat(filepath, &filestat);

	/* Get filesize */
	filelen = filestat.st_size;

	/* Allocate memory */
	buffer = memalign(32, filelen);
	if (!buffer)
		goto err;

	/* Open file */
	fp = fopen(filepath, "rb");
	if (!fp)
		goto err;

	/* Read file */
	ret = fread(buffer, 1, filelen, fp);
	if (ret != filelen)
		goto err;

	/* Set pointer */
	*outbuf = buffer;

	goto out;

err:
	/* Free memory */
	if (buffer)
		free(buffer);

	/* Error code */
	ret = -1;

out:
	/* Close file */
	if (fp)
		fclose(fp);

	return ret;
}
