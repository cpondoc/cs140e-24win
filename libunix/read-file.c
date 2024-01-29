#include <assert.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>

#include "libunix.h"

// allocate buffer, read entire file into it, return it.   
// buffer is zero padded to a multiple of 4.
//
//  - <size> = exact nbytes of file.
//  - for allocation: round up allocated size to 4-byte multiple, pad
//    buffer with 0s. 
//
// fatal error: open/read of <name> fails.
//   - make sure to check all system calls for errors.
//   - make sure to close the file descriptor (this will
//     matter for later labs).
// 
void *read_file(unsigned *size, const char *name) {
    // Set up helper variables
    struct stat stats;
    void *buffer = NULL;

    // Use stat to get results on files
    int result = stat(name, &stats);
    if (result == 0) {
        int file_size = stats.st_size;
        
        // Round up to a multiple of 4
        int original_file_size = file_size;
        file_size += 4;
        file_size -= (file_size % 4);

        // Allocate a buffer, zero pad to the next multiple of 4
        buffer = calloc(file_size + 4, sizeof(int));
        if (original_file_size == 0) {
            *size = file_size;
            return buffer;
        }

        // Read in file
        FILE* file = fopen(name, "r");
        int fd = fileno(file);
        printf("\nOriginal File Size: %d\n", file_size);
        int reading = read_exact(fd, buffer, original_file_size);

        // Close file descriptor, set size
        fclose(file);
        *size = reading;
        return buffer;
    }

    // Dummy return
    return buffer;
}
