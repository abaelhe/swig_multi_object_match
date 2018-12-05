
#include "driver.h"

using namespace std;
using namespace engine;


typedef unordered_map<uint32_t, std::pair<string, string>> FPMatchDict;
typedef unordered_map<uint16_t, uint32_t> FPDiffsMap;
typedef unordered_map<uint32_t, FPDiffsMap> FPTicketsMap;
typedef std::pair<uint32_t, uint32_t>  FPRankPair;


static  FPMatchDict *BIG_DICT = NULL;
static  MmapFile *mmapFile = NULL;
static char *MMAP_POINTER = NULL;
static uint64_t indexed_hash_ids_num = 0;

bool fingerprint_rank_function(FPRankPair &i, FPRankPair &j){
    return i.first > j.first;
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

    char a, *p = (char *)csv_data, *s;
    uint32_t hash_id, offset_idx, offset_diff, offset_tickets, offsets_num;
    uint16_t offset, offset_val, *offset_ptr;
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
                const string &offsets = got->second.first;
                offsets_num = offsets.size()/2;
                offset_ptr = (uint16_t *)offsets.c_str();
                for(offset_idx=0; offset_idx < offsets_num; offset_idx++){
                    offset_val = offset_ptr[offset_idx];
                    offset_diff = offset_val - offset;
                    offset_tickets = FingerprintTickets[hash_id][offset_diff];
                    FingerprintTickets[hash_id][offset_diff] = offset_tickets + 1;
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


void load_mmap_index(FPMatchDict &fp_dict, char *mmap_start, uint32_t hash_ids_num, uint64_t stored_file_size){
    DEBUG("Loading: hash_ids_num:%ld, file_size:%lld ...\n", hash_ids_num, stored_file_size);
    fp_dict.reserve(hash_ids_num);

    uint32_t hash_id;
    uint32_t offsets_num;
    uint32_t index = 0;
    uint32_t *h = (uint32_t *)(mmap_start + HEADER_SIZE);
    char *p = mmap_start + HEADER_SIZE + hash_ids_num * 4;
    while (index < hash_ids_num){
        hash_id = *h++;
        offsets_num = *((uint32_t *)p);
        p+=4;
        fp_dict[hash_id] = std::make_pair(string(p, 2*offsets_num), string(p + 2*offsets_num, offsets_num * 4));
        p += offsets_num * (2+4);
        index++;
        if(index % 100000 == 0){
            DEBUG("    %ld.\n", index);
        }

    }

    if((p - mmap_start) == stored_file_size){
        DEBUG("You are lucky, this MUST be the right place.\n");
    }
}

char *mmapfile_init(string mmap_file_path){
    if (MMAP_POINTER != NULL)return MMAP_POINTER;

    mmapFile = new MmapFile();
    mmapFile->Initialize(mmap_file_path, 0, false); //Read
    uint64_t size = mmapFile->Size();
    MMAP_POINTER = (char *) mmap(NULL, size, PROT_READ, MAP_FILE | MAP_SHARED, mmapFile->GetFd(), 0);
    if (MMAP_POINTER == MAP_FAILED) {
        MMAP_POINTER = NULL;
        int err = errno;
        FATAL("Unable to mmap: (%d) %s", err, strerror(err));
        return NULL;
    }

    if(size < HEADER_SIZE){
        return NULL;
    }

    uint64_t stored_file_size = *((uint64_t *)MMAP_POINTER);
    uint32_t hash_ids_num = *(uint32_t *)(MMAP_POINTER + 8);

    BIG_DICT  = new FPMatchDict;
    if (BIG_DICT == NULL) {
        int err = errno;
        FATAL("Unable to new class MatchIndexMap & FPMatchDict, %d, %s", err, strerror(err));
        return NULL;
    }

    BIG_DICT->reserve(hash_ids_num);
    load_mmap_index(*BIG_DICT, MMAP_POINTER, hash_ids_num, stored_file_size);

    return MMAP_POINTER;
}

string mmapfile_status(){
    return "";
}
