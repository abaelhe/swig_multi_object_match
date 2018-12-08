#include "mmapfile.h"

#define HEADER_SIZE 4096

int mmapfile_debug(int debug);
char  *mmapfile_init(string mmap_file_path);
string mmapfile_status();
string mmapfile_compute_fingerprints(char *csv_data, size_t nlargest);
