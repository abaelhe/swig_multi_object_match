
#include "driver.h"

using namespace std;
using namespace engine;


typedef vector<uint64_t> FPQueryVector;
typedef unordered_map<uint32_t, char *> FPMatchDict;
typedef unordered_map<int16_t, uint32_t> FPDiffsMap;
typedef unordered_map<uint32_t, FPDiffsMap> FPTicketsMap;
typedef std::unordered_map<uint32_t, uint32_t> FPRankMap;
typedef std::pair<uint32_t, uint32_t> FPRankPair;
static int debug_mode = 0;
static long PAGE_SIZE = 0;
static uint32_t SONG_TICKETS_HIGH_WATER = 1000;
static uint32_t indexed_hash_ids_num = 0;

static  FPMatchDict *BIG_DICT = NULL;
static  MmapFile *mmapFile = NULL;
static char *MMAP_POINTER = NULL;


int mmapfile_debug(int debug){
    debug_mode = debug;
    return debug_mode;
};

uint32_t dbg_fingerprint_diff_tickets(FPTicketsMap &fptm, uint32_t songid, int16_t diff){
    uint32_t val = fptm[songid][diff];
    return val;
}

uint32_t dbg_fingerprint_diffs(FPTicketsMap &fptm, uint32_t songid){
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

static inline bool FPRankPartial(FPRankPair &x, FPRankPair &y){
    return x.first  > y.first;
}

static inline bool fingerprint_parse_csv_query(char *csv_data, FPQueryVector &fpQuery){
# define CSV_DATA_FIELDS_NUM 2
    size_t field_index;
    const char *line_sep = "\r\n", *field_sep = ",";
    char *line, *_line_ctx, *field, *_field_ctx;
    char *fields_ptr[CSV_DATA_FIELDS_NUM];

    uint32_t hash_id;
    uint16_t offset;

    for (line = strtok_r(csv_data, line_sep, &_line_ctx); line; line = strtok_r(NULL, line_sep, &_line_ctx)){
        field_index = 0;
        for (field = strtok_r(line, field_sep, &_field_ctx); field; field = strtok_r(NULL, field_sep, &_field_ctx)){
            fields_ptr[field_index%CSV_DATA_FIELDS_NUM] = field;
            field_index++;
            if (field_index >= CSV_DATA_FIELDS_NUM)break;
        }

        hash_id = atoi(fields_ptr[0]);
        offset = atoi(fields_ptr[1]);

        fpQuery.push_back((uint64_t)offset<<32 | (uint64_t)hash_id);
    }

    return true;
}

static inline bool fingerprint_parse_hashid_offsets(FPQueryVector &fpQuery, FPRankMap &fpsmtm){
    uint32_t hash_id;
    uint16_t offset, *offset_ptr;
    uint64_t hash_id_offset;

    uint64_t rank_high_tickets_total=0;
    FPRankMap rank_high_tickets;
    FPTicketsMap fpt;
    char *o;

    uint32_t ticket_high_water = SONG_TICKETS_HIGH_WATER;
    uint32_t offsets_num, offset_idx, *songid_ptr, songid, tickets, *songid_max_tickets;
    for(FPQueryVector::iterator iter= fpQuery.begin(); iter != fpQuery.end(); iter++){
        hash_id_offset = *iter;
        hash_id =((uint32_t *)&hash_id_offset)[0];
        offset = ((uint32_t *)&hash_id_offset)[1];
        FPMatchDict::const_iterator got = BIG_DICT->find(hash_id);
        if(got == BIG_DICT->end())continue;

        o = got->second;
        offsets_num = *(uint32_t *)o;
        offset_ptr = (uint16_t *)(o + 4);
        songid_ptr = (uint32_t *)(offset_ptr + offsets_num);
        for(offset_idx=0; offset_idx < offsets_num; offset_idx++){
            songid = songid_ptr[offset_idx];
            if( rank_high_tickets.find(songid) != rank_high_tickets.end()){
                rank_high_tickets[songid]++;
                rank_high_tickets_total ++;
                continue;
            }

            tickets = ++(fpt[songid][offset_ptr[offset_idx] - offset]);
            songid_max_tickets = & fpsmtm[songid];
            if (tickets > *songid_max_tickets){
                *songid_max_tickets = tickets;
            }

            if(tickets >= ticket_high_water){
                rank_high_tickets[songid] = tickets;
                rank_high_tickets_total += tickets;
            }
        }
    }

    fpQuery.clear();
    fpt.clear();

    size_t normal_num = fpsmtm.size();
    size_t rank_high_tickets_num = rank_high_tickets.size();
    uint32_t avg_high_score = rank_high_tickets_total / rank_high_tickets.size();
    for(FPRankMap::iterator riter=rank_high_tickets.begin(); riter != rank_high_tickets.end(); riter++){
        songid = riter->first;
        if(rank_high_tickets_num >= avg_high_score){
            fpsmtm[songid] = riter->second;
            continue;
        }

        if(normal_num != 0){
            fpsmtm[songid] = riter->second;
        }
    }
    return true;
}

static inline bool fingerprint_songs_rank(string &result, FPRankMap &fpsmtm, size_t nlargest){
    std::vector<FPRankPair> FPRankVector;
    size_t song_size = fpsmtm.size();
    FPRankMap::iterator iter;
    while(song_size-- > 0){
        iter = fpsmtm.begin();
        FPRankVector.push_back(std::make_pair(iter->second, iter->first));
        fpsmtm.erase(iter);
    }

    nlargest = FPRankVector.size() > nlargest ? nlargest : FPRankVector.size();
    std::partial_sort(FPRankVector.begin(), FPRankVector.begin() + nlargest, FPRankVector.end(), FPRankPartial);

    uint32_t *rank_result_ptr = (uint32_t *)result.c_str();
    for(size_t i=0; i< nlargest; i++){
        *rank_result_ptr = FPRankVector[i].first;
        rank_result_ptr[nlargest] = FPRankVector[i].second;
        rank_result_ptr++;
    }

    return true;
}

string mmapfile_compute_fingerprints(char *csv_data, size_t nlargest=5){
    if (MMAP_POINTER == NULL){
        return "Error: call mmapfile_init to load the database first.";
    }
    double time_start;
    GETCLOCKS(time_start);

    FPRankMap FPSongMaxTicketsMap;
    FPQueryVector fpQuery;

    fingerprint_parse_csv_query(csv_data, fpQuery);
    size_t qsize = fpQuery.size();
    double time_csv;
    GETCLOCKS(time_csv);

    fingerprint_parse_hashid_offsets(fpQuery, FPSongMaxTicketsMap);
    size_t song_size = FPSongMaxTicketsMap.size();
    double time_hashids;
    GETCLOCKS(time_hashids);

    string result(nlargest * 8, 'A');
    fingerprint_songs_rank(result, FPSongMaxTicketsMap, nlargest);
    double time_rank;
    GETCLOCKS(time_rank);

    DEBUG("qsize:%lu, song_size:%lu, csv:%0.5f,  hashids_dict_time:%0.5f,  rank_time:%0.5f\n", \
        qsize, song_size, time_csv-time_start, time_hashids - time_csv, time_rank - time_hashids);
    return result;
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
