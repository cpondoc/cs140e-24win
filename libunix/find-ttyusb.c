// engler, cs140e: your code to find the tty-usb device on your laptop.
#include <assert.h>
#include <fcntl.h>
#include <string.h>

#include "libunix.h"

#define _SVID_SOURCE
#include <dirent.h>
static const char *ttyusb_prefixes[] = {
    "ttyUSB",	// linux
    "ttyACM",   // linux
    "cu.SLAB_USB", // mac os
    "cu.usbserial", // mac os
    // if your system uses another name, add it.
	0
};

static int filter(const struct dirent *d) {
    // scan through the prefixes, returning 1 when you find a match.
    // 0 if there is no match.
    for (int i = 0; i < 4; i++) {
        // Check through the two strings, check
        // https://stackoverflow.com/questions/4770985/how-to-check-if-a-string-starts-with-another-string-in-c
        if (strncmp(ttyusb_prefixes[i], d->d_name, strlen(ttyusb_prefixes[i])) == 0) {
            return 1;
        }
    }
    return 0;
}

// find the TTY-usb device (if any) by using <scandir> to search for
// a device with a prefix given by <ttyusb_prefixes> in /dev
// returns:
//  - device name.
// error: panic's if 0 or more than 1 devices.
char *find_ttyusb(void) {
    // use <alphasort> in <scandir>
    // return a malloc'd name so doesn't corrupt.
    // Use <scandir> to search for devices
    char *name = "";
    struct dirent **file_list;
    int num_files = scandir("/dev", &file_list, filter, alphasort);
    if (num_files > 0) {
        // Malloc a string name
        const char* directory_name = file_list[0]->d_name;
        char *prefix = "/dev/";
        name = (char*)malloc(7 + strlen(directory_name));

        // Copy over text
        strcpy(name, prefix);
        strcat(name, directory_name);
        return name;
    }
    
    return name;
}

// return the most recently mounted ttyusb (the one
// mounted last).  use the modification time 
// returned by state.
char *find_ttyusb_last(void) {
    unimplemented();
}

// return the oldest mounted ttyusb (the one mounted
// "first") --- use the modification returned by
// stat()
char *find_ttyusb_first(void) {
    unimplemented();
}
