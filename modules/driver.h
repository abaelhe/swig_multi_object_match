#include "mmapfile.h"

#define HEADER_SIZE 4096

int mmapfile_debug(int debug);
char  *mmapfile_init(string mmap_file_path);
string mmapfile_status();
string mmapfile_compute_fingerprints(const char *csv_data, int nlargest);
