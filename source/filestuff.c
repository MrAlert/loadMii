#include <stdio.h>
#include <string.h>
#include <malloc.h>
#include <stdlib.h>
#include <unistd.h>
#include <dirent.h> 
#include <gccore.h>
#include <fat.h>
#include <iso9660.h>

#include "filestuff.h"
#include "tools.h"
                         
const fatdev devlst[] = {
                        {"Internal SD slot"   , "sd" , &__io_wiisd     },
                        {"Usb stick"          , "usb", &__io_usbstorage},
                        {"Dvd (Requires DVDX)", "dvd", &__io_wiidvd    },
                        {"SD Gecko in slot A" , "sda", &__io_gcsda     },
                        {"SD Gecko in slot B" , "sdb", &__io_gcsdb     }
                         };                   
                         
const int maxdev = sizeof(devlst) / sizeof(fatdev);

static const fatdev * inuse = NULL; /* Fat device in use.    */
static char bPath[MAXPATHLEN];      /* Browsing path.        */
static item * list = NULL; 	    /* Global list of files. */
static int  fCount = 0;    	    /* Files count.          */      

item *getItem (int n)
{
        return &list[n];
}

int getFilesCount ()
{
        return fCount;
}

int isDeviceInserted ()
{
        return inuse->io->isInserted();
}

char *getCurrentPath ()
{
        char ret[MAXPATHLEN];
        
        memset(ret, 0, MAXPATHLEN);
        
        if (strlen(bPath) >= 40)
        {
                strncpy(ret, bPath, 18);
                strcat(ret, "...");
                strncat(ret, bPath + (strlen(bPath) - 19), 18);
        }
        else
        {
                strcpy(ret, bPath);
        }
        
        return strdup(ret);
}

int supportedFile (char *name)
{
        if (strcasestr(name, ".elf") ||
            strcasestr(name, ".dol"))
        {
                return 1;
        }
        return 0;
}

void getFiles ()
{
        int index;
        char name[MAXPATHLEN];

        DIR_ITER *iter = diropen(bPath);
        struct stat fstat;

        if (list != NULL)
        {
                free(list);
                list = NULL;
                fCount = 0;
        }

        list = malloc(sizeof(item));

        if (iter == NULL)
        {
                return;
        }

        index = 0;

        while (dirnext(iter, name, &fstat) == 0)
        {
                list = (item *)realloc(list, sizeof(item) * (index + 1));
                memset(&(list[index]), 0, sizeof(item));
                
                sprintf(list[index].name, "%s", name);
                
                if (fstat.st_mode & S_IFDIR)
                {
                        list[index].size = 0;
                }
                else
                {
                        list[index].size = fstat.st_size;
                }
                
                if (matchStr(list[index].name, "."))
                {
                        sprintf(list[index].labl, "[Current directory]");
                }
		else if (matchStr(list[index].name, ".."))
                {
                        sprintf(list[index].labl, "[Parent directory]");
                }
                else
                {
			if (list[index].size > 0)
			{
				sprintf(list[index].labl, "%.40s", list[index].name);
			}
			else
			{
				sprintf(list[index].labl, "[%.380s]", list[index].name);
			}
                }
                
                index++;
        }

        dirclose(iter);

        fCount = index;
}

void unmountDevice ()
{
        if (inuse != NULL)
        {
                if (matchStr(inuse->root, "dvd"))
                {
                        ISO9660_Unmount();
                }
                else
                {
                        fatUnmount(inuse->root);
                }
                inuse = NULL;
        }
}

void doStartup (int activate)
{
        int dev;
        for (dev=0;dev<(maxdev-1);dev++)
        {
                if (activate)
                {
                        devlst[dev].io->startup();
                }
                else
                {
                        devlst[dev].io->shutdown();
                }
        }
}

int setDevice (const fatdev *device)
{
        unmountDevice();

        if (!(device->io->isInserted()))
        {
                return 0;
        }
        
        if (matchStr(device->root, "dvd"))
        {
                DI_Mount();
                if (!(ISO9660_Mount()))
                {
                        setError(1);
                        return 0;
                }
        }
        else
        {                       
                if (!fatMount(device->root, device->io, 0, 8, 512))
                {
                        setError(2);
                        return 0;
                }
        }
        
        inuse = device;

        memset(&bPath, 0, sizeof(bPath));
        sprintf(bPath, "%s:/", inuse->root);
        
        getFiles();
        
        return 1;
}

int updatePath (char *update)
{
        if (matchStr(update, "."))
        {
                return 1;
        }
        else if (matchStr(update, ".."))
        {
                char * lastSlash = strrchr(bPath + strlen(inuse->root) + 2, '/');
                if (lastSlash == NULL)
                {
                        return 0;
                }
                *lastSlash = 0;
        }
        else
        {
                sprintf(bPath, "%s/%s", bPath, update);
        }

        getFiles();
        
        return 1;
}

char *getFullName (item *file)
{
        char ffPath[MAXPATHLEN];
        
        memset(&ffPath, 0, sizeof(ffPath));
        
        sprintf(ffPath, "%s/%s", getCurrentPath(), file->name);

        return strdup(ffPath);
}

u8 *memoryLoad (item *file)
{
        char *ffPath = getFullName(file);
        u8 *memholder;
        
        FILE * fp = fopen(ffPath, "rb");
	free(ffPath);
        
        if (fp == NULL)
        {
                return NULL;
        }

        memholder = malloc(file->size);
        if (!memholder)
	{
	        fclose(fp);
		return NULL;
	}

        if (fread(memholder, 1, file->size, fp) < file->size)
        {
                free(memholder);
	        fclose(fp);
                return NULL;
        }
        
        fclose(fp);
        
        return memholder;
}
