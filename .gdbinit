
symbol-file _mmapfile.so
#add-symbol-file _mmapfile.so
set  debug-file-directory .
file /usr/bin/python2.7

set breakpoint pending on
#br _PyImport_LoadDynamicModule
br load_mmap_index
br fingerprint_rank
br mmapfile_compute_fingerprints
r -i -c 'import _mmapfile as mmapfile;mmapfile.mmapfile_init("../data/index.mmap");mmapfile.mmapfile_compute_fingerprints(open("/home/heyijun/pglib/g/0588/152385.csv").read(), 5)'
