#include <stdio.h>
#include <ogcsys.h>

#include "disc.h"
#include "fat.h"
#include "gui.h"
#include "menu.h"
#include "restart.h"
#include "sys.h"
#include "video.h"
#include "wpad.h"


int main(int argc, char **argv)
{
	/* Load Custom IOS */
	IOS_ReloadIOS(249);

	/* Initialize subsystems */
	Sys_Init();

	/* Set video mode */
	Video_SetMode();

	/* Initialize console */
	Gui_InitConsole();

	/* Draw background */
	Gui_DrawBackground();

	/* Initialize Wiimote subsystem */
	Wpad_Init();

	/* Initialize disc subsystem */
	Disc_Init();

	/* Mount SD card */
	Fat_MountSD();

	/* Menu loop */
	Menu_Loop();

	/* Restart */
	Restart();

	return 0;
}
