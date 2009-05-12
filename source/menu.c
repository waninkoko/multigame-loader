#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ogcsys.h>

#include "disc.h"
#include "gui.h"
#include "multidisc.h"
#include "restart.h"
#include "video.h"
#include "wdvd.h"
#include "wpad.h"

/* Constants */
#define ENTRIES_PER_PAGE	12
#define MAX_CHARACTERS		30


s32 __Menu_EntryCmp(const void *p1, const void *p2)
{
	struct wiigame *g1 = (struct wiigame *)p1;
	struct wiigame *g2 = (struct wiigame *)p2;

	/* Compare entries */
	return strcmp(g1->header.title, g2->header.title);
}

char *__Menu_GameTitle(char *name)
{
	static char buffer[MAX_CHARACTERS + 4];

	/* Clear buffer */
	memset(buffer, 0, sizeof(buffer));

	/* Check string length */
	if (strlen(name) > (MAX_CHARACTERS + 3)) {
		strncpy(buffer, name,  MAX_CHARACTERS);
		strncat(buffer, "...", 3);

		return buffer;
	}

	return name;
}


void Menu_RetrieveList(struct wiigame **outbuf, u32 *outlen)
{
	s32 ret;

	/* Free memory */
	if (*outbuf) {
		free(*outbuf);
		*outbuf = NULL;
	}

	/* Clear console */
	Con_Clear();

	printf("[+] Insert a DVD disc inside the drive...\n");
	printf("    Press HOME button to restart.\n\n");

	/* Wait for disc */
	while (Disc_Status()) {
		u32 buttons = Wpad_GetButtons();

		/* HOME button */
		if (buttons & WPAD_BUTTON_HOME)
			return;

		VIDEO_WaitVSync();
	}

	printf("[+] Opening DVD disc...");
	fflush(stdout);

	/* Open DVD disc */
	ret = Disc_Open();
	if (ret < 0) {
		printf(" ERROR! (ret = %d)\n", ret);
		goto retry;
	}
	printf(" OK!\n");

	printf("[+] Getting game list...");
	fflush(stdout);

	/* Get game list */
	ret = Multidisc_GetEntries(outbuf, outlen);
	if (ret < 0) {
		printf(" ERROR! (ret = %d)\n", ret);
		goto retry;
	}

	/* Sort list */
	qsort(*outbuf, *outlen, sizeof(struct wiigame), __Menu_EntryCmp);

	return;

retry:
	printf("\n");
	printf("    Press any button to retry...\n");

	/* Wait for any button */
	Wpad_WaitButtons();

	/* Retry */
	Menu_RetrieveList(outbuf, outlen);
}

void Menu_Boot(struct wiigame *game)
{
	u8  cios = 0, gamecube = 0, iosreload = 1;
	s32 ret;

	/* Clear console */
	Con_Clear();

	/* Set disc offset */
	Disc_SetOffset(game->offset);

	/* Check Custom IOS/disc type */
	cios     = IOS_GetVersion() == 249;
	gamecube = Disc_IsGamecube(&game->header);

	for (;;) {
		/* Clear console */
		Con_Clear();

		/* Show game info */
		printf("[+] Game Name      : %s\n",   __Menu_GameTitle(game->header.title));
		printf("    Game ID        : %s\n",   game->header.id);
		printf("    Game Plataform : %s\n\n", (gamecube) ? "Gamecube" : "Wii");

		/* Wii options */
		if (!gamecube) {
			printf("[+] Wii Options:\n");
			printf("    IOS Reloading  : %s (press 1)\n\n", (iosreload) ? "Enabled" : "Disabled");
		}

		printf("    Press A button to boot this game\n");
		printf("    Press B button to go back.\n\n");

		/* Custom IOS options */
		if (cios)
			printf("    Press HOME button to launch Menu.\n\n");

		printf("\n");


		/** Controls **/
		u32 buttons = Wpad_WaitButtons();

		/* A button */
		if (buttons & WPAD_BUTTON_A) {
			printf("[+] Booting %s game, please wait...", (gamecube) ? "Gamecube" : "Wii");
			fflush(stdout);

			/* Wii/GC boot */
			if (!gamecube)
				ret = Disc_WiiBoot(iosreload);
			else
				ret = Disc_GCBoot();

			printf(" RETURNED! (ret = %d)\n", ret);

			break;
		}

		/* B button  */
		if (buttons & WPAD_BUTTON_B)
			return;

		/* Wii options */
		if (!gamecube) {
			/* ONE (1) button (Toggle IOS Reloading) */
			if (buttons & WPAD_BUTTON_1)
				iosreload ^= 1;
		}

		/* Custom IOS options */
		if (cios) {
			/* HOME button (Launch System Menu) */
			if (buttons & WPAD_BUTTON_HOME) {
				printf("[+] Launching System Menu, please wait...");
				fflush(stdout);

				/* System Menu boot */
				ret = Disc_MenuBoot(iosreload);

				printf(" RETURNED! (ret = %d)\n", ret);

				break;
			}
		}
	}

	printf("\n");
	printf("    Press any button to continue...\n");

	/* Wait for any button */
	Wpad_WaitButtons();
}

void Menu_Loop(void)
{
	struct wiigame *gameList = NULL;
	u32             gameCnt;

	s32 selected = 0, start = 0;

	/* Get game list */
	Menu_RetrieveList(&gameList, &gameCnt);

	for (;;) {
		u32 cnt;
		s32 index;

		/* Clear console */
		Con_Clear();

		/** Entries **/
		printf("[+] Games found in the disc:\n\n");

		/* Print gamelist */
		for (cnt = start; cnt < gameCnt; cnt++) {
			struct wiigame *game = &gameList[cnt];

			/* Entries per page limit reached */
			if ((cnt - start) >= ENTRIES_PER_PAGE)
				break;

			/* Print entry */
			printf("\t%2s \"%s\"\n", (cnt == selected) ? ">>" : "  ", __Menu_GameTitle(game->header.title));
		}

		/* Show cover */
		Gui_DrawCover(gameList[selected].header.id);


		/** Controls **/
		u32 buttons = Wpad_WaitButtons();

		/* DPAD buttons */
		if (buttons & (WPAD_BUTTON_UP | WPAD_BUTTON_LEFT)) {
			selected -= (buttons & WPAD_BUTTON_LEFT)  ? ENTRIES_PER_PAGE : 1;

			if (selected <= -1)
				selected = (gameCnt - 1);
		}
		if (buttons & (WPAD_BUTTON_DOWN | WPAD_BUTTON_RIGHT)) {
			selected += (buttons & WPAD_BUTTON_RIGHT) ? ENTRIES_PER_PAGE : 1;

			if (selected >= gameCnt)
				selected = 0;
		}

		/* HOME button */
		if (buttons & WPAD_BUTTON_HOME)
			Restart();

		/* ONE (1) button */
		if (buttons & WPAD_BUTTON_1)
			Menu_RetrieveList(&gameList, &gameCnt);

		/* A button */
		if (buttons & WPAD_BUTTON_A)
			Menu_Boot(&gameList[selected]);


		/** Scrolling **/
		/* List scrolling */
		index = (selected - start);

		if (index >= ENTRIES_PER_PAGE)
			start += index - (ENTRIES_PER_PAGE - 1);
		if (index <= -1)
			start += index;
	}

	/* Free memory */
	if (gameList)
		free(gameList);
}
