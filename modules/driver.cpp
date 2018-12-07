
#include "driver.h"

using namespace std;
using namespace engine;


typedef unordered_map<uint32_t, char *> FPMatchDict;
typedef unordered_map<int16_t, uint32_t> FPDiffsMap;
typedef unordered_map<uint32_t, FPDiffsMap> FPTicketsMap;
typedef std::pair<uint32_t, uint32_t>  FPRankPair;

static int debug_mode = 0;
static long PAGE_SIZE = 0;
static uint32_t indexed_hash_ids_num = 0;

static  FPMatchDict *BIG_DICT = NULL;
static  MmapFile *mmapFile = NULL;
static char *MMAP_POINTER = NULL;


int mmapfile_debug(int debug){
    debug_mode = debug;
    return debug_mode;
};

bool fingerprint_rank_function(FPRankPair &i, FPRankPair &j){
    return i.first > j.first;
}

uint32_t get_fingerprint_diff_tickets(FPTicketsMap &fptm, uint32_t songid, int16_t diff){
    uint32_t val = fptm[songid][diff];
    return val;
}

uint32_t get_fingerprint_diffs(FPTicketsMap &fptm, uint32_t songid){
    FPDiffsMap &diffs = fptm[songid];
    DEBUG("songid: %d\n", songid);
    uint32_t max_tickets = 0;
    for(FPDiffsMap::const_iterator it=diffs.begin(); it != diffs.end(); ++it){
        int16_t diff = it->first;
        uint32_t tickets = it->second;
        if(tickets > max_tickets){
            max_tickets = tickets;
        }
        DEBUG("    %d: %d\n", diff, tickets);
    }

    DEBUG("songid: %d, max_tickets:%d\n", songid, max_tickets);
    return max_tickets;
}


void fingerprint_rank(FPTicketsMap &fingerprint_tickets, string &result, int nlargest){
    vector<FPRankPair> fp_ranks;

    uint32_t songid, offset_diff_tickets_max;
    FPTicketsMap::iterator it;
    FPDiffsMap::iterator ti;
    size_t tickets_size = fingerprint_tickets.size(), diffs_size;

    while(tickets_size-- > 0){
        it=fingerprint_tickets.begin();
        songid = it->first;
        FPDiffsMap & tickets = it->second;

        offset_diff_tickets_max = 0;
        diffs_size = tickets.size();
        while(diffs_size-- >0){
            ti = tickets.begin();
            if (ti->second > offset_diff_tickets_max){
                offset_diff_tickets_max = ti->second;
            }
            tickets.erase(ti);
        }

        fingerprint_tickets.erase(it);
        fp_ranks.push_back(std::make_pair(offset_diff_tickets_max, songid));
    }

    nlargest = fp_ranks.size() > nlargest ? nlargest : fp_ranks.size();
    std::partial_sort(fp_ranks.begin(), fp_ranks.begin() + nlargest, fp_ranks.end(), fingerprint_rank_function);

    result.resize(nlargest * 8);
    uint32_t *x = (uint32_t *)result.c_str();

    for(int i=0; i< nlargest; i++){
        FPRankPair &m = fp_ranks[i];
        *x = m.first;
        x[nlargest] = m.second;
        x++;
    }
}


string mmapfile_compute_fingerprints(const char *csv_data, int nlargest=5){
    if (MMAP_POINTER == NULL){
        return "Error: call mmapfile_init to load the database first.";
    }

    FPTicketsMap FingerprintTickets;

    char a, *p = (char *)csv_data, *s, *o;
    uint32_t hash_id, offset_idx, offsets_num, *songid_ptr;
    uint16_t offset, *offset_ptr;
    bool matched_hash_id = false;

    while(1){
        s = p;
        while(*p >= '0' && *p <= '9')p++;

        a = *p;
        if(a == '\0'){
            break;

        }else if(a == ','){
            *p = '\0';
            if(! matched_hash_id){
                hash_id = atoi(s);
                matched_hash_id = true;
            }else{
                matched_hash_id = false;
            }

        }else if(a =='\n'){
            if(matched_hash_id){
                *p = '\0';
                offset =(uint16_t)atoi(s);

                FPMatchDict::const_iterator got = BIG_DICT->find(hash_id);
                if(got == BIG_DICT->end())continue;
                o = got->second;
                offsets_num = *(uint32_t *)o;
                offset_ptr = (uint16_t *)(o + 4);
                songid_ptr = (uint32_t *)(offset_ptr + offsets_num);
                for(offset_idx=0; offset_idx < offsets_num; offset_idx++){
                    FingerprintTickets[songid_ptr[offset_idx]][offset_ptr[offset_idx]-offset]++;
                }
            }
            matched_hash_id = false;
        }else if(a<'0' || a > '9'){
            break;
        }

        p++;
        continue;
    }

    string result;
    fingerprint_rank(FingerprintTickets,  result, nlargest);

    return result;
}

size_t read_nbytes(int fd, char *p, size_t n){
    ssize_t num;
    size_t readed_num = 0;

    while (readed_num < n){
        num = read(fd, p + readed_num, n - readed_num);
        if(-1 == num){
            int _err = errno;
            FATAL("Error, read_nbytes, fd:%d, errno:%d, %s\n", fd, _err, strerror(_err));
            return -1;
        }

        if(0 == num)break;
        readed_num += num;
    }

    return readed_num;
}

char *mmapfile_init(string mmap_file_path){
    if (MMAP_POINTER != NULL)return MMAP_POINTER;

#define ERROR_IF(_a, _err_str) \
    if(_a){ \
        int _err = errno; \
        FATAL("Error, %s, file:%s, errno:%d, %s\n", (_err_str), mmap_file_path.c_str(), _err, strerror(_err)); \
        return NULL; \
    }

    ERROR_IF(((PAGE_SIZE=sysconf(_SC_PAGE_SIZE)) == -1), "sysconf(_SC_PAGE_SIZE)");
    ERROR_IF((!fileExists(mmap_file_path)), "file not exist!");
    mmapFile = new MmapFile();
    mmapFile->Initialize(mmap_file_path, 0, false); //Read
    uint64_t size = mmapFile->Size();
    ERROR_IF((size < HEADER_SIZE), "index size less than HEADER_SIZE.");
    MMAP_POINTER = (char *) mmap(NULL, size, PROT_READ, MAP_FILE | MAP_PRIVATE | MAP_NOCACHE | MAP_NOEXTEND, mmapFile->GetFd(), 0);
    ERROR_IF((MMAP_POINTER == MAP_FAILED), "mmap.");

    uint64_t stored_file_size = *((uint64_t *)MMAP_POINTER);
    uint32_t hash_ids_num  = indexed_hash_ids_num = *(uint32_t *)((char *)MMAP_POINTER + 8);

    BIG_DICT  = new FPMatchDict;
    ERROR_IF((BIG_DICT == NULL), "new FPMatchDict");
    BIG_DICT->reserve(hash_ids_num);
    FPMatchDict &fp_dict = *BIG_DICT;

    DEBUG("Loading: hash_ids_num:%u, file_size:%lu ...\n", hash_ids_num, stored_file_size);
    uint32_t index = 0, *hashid_array = (uint32_t*)(MMAP_POINTER + HEADER_SIZE);
    char *p = MMAP_POINTER + HEADER_SIZE + hash_ids_num * 4;
    while (index < hash_ids_num){
        if(index % int(hash_ids_num / 10) == 0){DEBUG("   %0.2f%%.\n", index*100.0/hash_ids_num);}
        fp_dict[hashid_array[index]] = p;
        p += (4 + *((uint32_t *)p) * (2+4));
        index++;
    }

    if((uint64_t)(p - MMAP_POINTER) != stored_file_size){
        FATAL("Error read index size:%ld", stored_file_size);
        munmap(MMAP_POINTER, size);
        return NULL;
    }

    ERROR_IF((-1 == munmap(MMAP_POINTER, PAGE_SIZE * ((HEADER_SIZE + hash_ids_num * 4) / PAGE_SIZE))), "munmap hashids segment.");
    MMAP_POINTER += (PAGE_SIZE * ((HEADER_SIZE + hash_ids_num * 4) / PAGE_SIZE));
    DEBUG("You are lucky, this MUST be the right place.\n");
    return MMAP_POINTER;
#undef ERROR_IF
}

string mmapfile_status(){
    return "";
}
