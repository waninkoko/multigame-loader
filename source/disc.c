#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <malloc.h>
#include <ogcsys.h>
#include <ogc/lwp_watchdog.h>

#include "apploader.h"
#include "disc.h"
#include "sysmenu.h"
#include "video.h"
#include "wdvd.h"
#include "wpad.h"

/* Constants */
#define BC_TITLEID	0x100000100ULL
#define GC_MAGIC	0xC2339F3D

/* Variables */
static u8  *diskid    = (u8  *)0x80000000;
static u32 *tmdbuffer = (u32 *)0x93000000;


void __Disc_SetLowMem(void)
{
	/* Setup low memory */
	*(vu32 *)0x80000030 = 0x00000000;
	*(vu32 *)0x80000060 = 0x38A00040;
	*(vu32 *)0x800000E4 = 0x80431A80;
	*(vu32 *)0x800000EC = 0x81800000;
	*(vu32 *)0x800000F4 = 0x817E5480;
	*(vu32 *)0x800000F8 = 0x0E7BE2C0;
	*(vu32 *)0x800000FC = 0x2B73A840;

	/* Copy disc ID */
	memcpy((void *)0x80003180, (void *)0x80000000, 4);

	/* Flush cache */
	DCFlushRange((void *)0x80000000, 0x3F00);
}

void __Disc_SetVMode(void)
{
	GXRModeObj *vmode     = NULL;
	u32         vmode_reg = 0;

	u32 progressive, tvmode;

	/* Get video mode configuration */
	progressive = (CONF_GetProgressiveScan() > 0) && VIDEO_HaveComponentCable();
	tvmode      =  CONF_GetVideo();

	/* Select video mode register */
	switch (tvmode) {
	case CONF_VIDEO_PAL:
		vmode_reg = (CONF_GetEuRGB60() > 0) ? 5 : 1;
		break;

	case CONF_VIDEO_MPAL:
		vmode_reg = 4;
		break;

	case CONF_VIDEO_NTSC:
		vmode_reg = 0;
		break;
	}

	/* Select video mode */
	switch(diskid[3]) {
	/* PAL */
	case 'D':
	case 'F':
	case 'P':
	case 'X':
	case 'Y':
		if (tvmode != CONF_VIDEO_PAL) {
			vmode_reg = 1;
			vmode     = (progressive) ? &TVNtsc480Prog : &TVNtsc480IntDf;
		}

		break;

	/* NTSC or unknown */
	case 'E':
	case 'J':
		if (tvmode != CONF_VIDEO_NTSC) {
			vmode_reg = 0;
			vmode     = (progressive) ? &TVNtsc480Prog : &TVEurgb60Hz480IntDf;
		}

		break;
	}

	/* Set video mode register */
	*(vu32 *)0x800000CC = vmode_reg;

	/* Set video mode */
	if (vmode)
		Video_Configure(vmode);

	/* Clear screen */
	Video_Clear(COLOR_BLACK);
}

void __Disc_SetTime(void)
{
	/* Extern */
	extern void settime(long long);

	/* Set proper time */
	settime(secs_to_ticks(time(NULL) - 946684800));
}

s32 __Disc_ReloadIOS(void)
{
	tmd *p_tmd = NULL;

	u8  version;
	s32 ret;

	/* Retrieve IOS version */
	p_tmd   = (tmd *)SIGNATURE_PAYLOAD(tmdbuffer);
	version = (u8)(p_tmd->sys_version & 0xFF);

	/* Initialize ES */
	ret = __ES_Init();
	if (ret < 0)
		return ret;

	/* Load IOS */
	ret = __IOS_LaunchNewIOS(version);

	/* Close ES */
	__ES_Close();

	return ret;
}

s32 __Disc_FindPartition(u64 *outbuf)
{
	static u32 buffer[8] ATTRIBUTE_ALIGN(32);

	u64 offset = 0, table_offset = 0;

	u32 cnt, nb_partitions;
	s32 ret;

	/* Read partition info */
	ret = WDVD_UnencryptedRead(buffer, sizeof(buffer), 0x40000);
	if (ret < 0)
		return ret;

	/* Get data */
	nb_partitions = buffer[0];
	table_offset  = buffer[1] << 2;

	/* Read partition table */
	ret = WDVD_UnencryptedRead(buffer, sizeof(buffer), table_offset);
	if (ret < 0)
		return ret;

	/* Find game partition */
	for (cnt = 0; cnt < nb_partitions; cnt++) {
		u32 type = buffer[cnt * 2 + 1];

		/* Game partition */
		if(!type)
			offset = buffer[cnt * 2] << 2;
	}

	/* No game partition found */
	if (!offset)
		return -1;

	/* Set output buffer */
	*outbuf = offset;

	return 0;
}


s32 Disc_Init(void)
{
	/* Init DVD subsystem */
	return WDVD_Init();
}

s32 Disc_Open(void)
{
	s32 ret;

	/* Reset DVD drive */
	ret = WDVD_Reset();
	if (ret < 0)
		return ret;

	/* Read disc ID */
	ret = WDVD_ReadDiskId(diskid);
	if (ret < 0)
		return ret;

	return 0;
}

s32 Disc_Status(void)
{
	u32 status;
	s32 ret;

	/* Get cover status */
	ret = WDVD_GetCoverStatus(&status);
	if (ret < 0)
		return ret;

	/* Disc inserted */
	if (status & 0x2)
		return 0;

	return 1;
}

s32 Disc_SetOffset(u64 offset)
{
	/* Set offset */
	return WDVD_Offset(offset);
}

s32 Disc_ReadHeader(void *outbuf)
{
	static struct discHdr buffer ATTRIBUTE_ALIGN(32);

	s32 ret;

	/* Read disc header */
	ret = WDVD_UnencryptedRead(&buffer, sizeof(buffer), 0);
	if (ret >= 0)
		memcpy(outbuf, &buffer, sizeof(buffer));

	return ret;
}

s32 Disc_IsGamecube(struct discHdr *header)
{
	/* Check for GameCube magic word */
	return (header->magic == GC_MAGIC);
}

s32 Disc_BootPartition(u64 offset, u8 iosreload)
{
	entry_point p_entry;

	s32 ret;

	/* Open specified partition */
	ret = WDVD_OpenPartition(offset, tmdbuffer);
	if (ret < 0)
		return ret;

	/* Run apploader */
	ret = Apploader_Run(&p_entry);
	if (ret < 0)
		return ret;

	/* Setup low memory */
	__Disc_SetLowMem();

	/* Set an appropiate video mode */
	__Disc_SetVMode();

	/* Set time */
	__Disc_SetTime();

	/* Disconnect Wiimote */
	Wpad_Disconnect();

	/* Shutdown IOS subsystems */
 	SYS_ResetSystem(SYS_SHUTDOWN, 0, 0);

	/* Reload IOS */
	if (iosreload)
		__Disc_ReloadIOS();

	/* Jump to entry point */
	p_entry();

	return 0;
}

s32 Disc_WiiBoot(u8 iosreload)
{
	u64 offset;
	s32 ret;

	/* Read disk ID */
	ret = WDVD_ReadDiskId(diskid);
	if (ret < 0)
		return ret;

	/* Find game partition offset */
	ret = __Disc_FindPartition(&offset);
	if (ret < 0)
		return ret;

	/* Boot partition */
	return Disc_BootPartition(offset, iosreload);
}

s32 Disc_GCBoot(void)
{
	static tikview view ATTRIBUTE_ALIGN(32);

	s32 ret;

	/* Read disk ID */
	ret = WDVD_ReadDiskId(diskid);
	if (ret < 0)
		return ret;

	/* Set GC register */
	*(vu32 *)0xCC003024 |= 7;

	/* Get ticket view */
	ret = ES_GetTicketViews(BC_TITLEID, &view, 1);
	if (ret < 0)
		return ret;

	/* Disconnect Wiimote */
	Wpad_Disconnect();

	/* Launch BC */
	return ES_LaunchTitle(BC_TITLEID, &view);
}

s32 Disc_MenuBoot(u8 iosreload)
{
	s32 ret;

	/* Disable reset */
	ret = WDVD_DisableReset(1);
	if (ret < 0)
		return ret;

	/* Launch System Menu */
	ret = Sysmenu_Launch();
	if (ret < 0)
		return ret;

	/* Enable reset */
	ret = WDVD_DisableReset(0);
	if (ret < 0)
		return ret;

	return 0;
}
