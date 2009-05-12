#ifndef _FAT_H_
#define _FAT_H_

/* Prototypes */
s32 Fat_MountSD(void);
s32 Fat_UnmountSD(void);
s32 Fat_ReadFile(const char *, void **);

#endif
