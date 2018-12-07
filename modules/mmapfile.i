/* File : mmap_engine.i */

%module mmapfile
%include <std_string.i>
%include <std_vector.i>
%include <exception.i>
using namespace std;

%apply const std::string& {std::string* foo};

struct cpp_string{
  std::string val;
};

#define HEADER_SIZE 4096


%{
/* Put headers and other declarations here */
struct cpp_string{
  std::string val;
};

extern int         mmapfile_debug(int debug);
extern std::string mmapfile_status();
extern char       *mmapfile_init(std::string mmap_file_path);
extern std::string mmapfile_compute_fingerprints(const char *csv_data, int nlargest);

%}

extern int         mmapfile_debug(int debug);
extern char       *mmapfile_init(std::string mmap_file_path);
extern std::string mmapfile_status();
extern std::string mmapfile_compute_fingerprints(const char *csv_data, int nlargest);

