#include "mmapfile.h"

#define HEADER_SIZE 4096

char  *mmapfile_init(string mmap_file_path);
string mmapfile_status();
string mmapfile_compute_fingerprints(const char *csv_data, int nlargest);
