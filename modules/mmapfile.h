#ifndef _MMAPFILE_H
#define	_MMAPFILE_H

#include "linuxplatform.h"

namespace engine{

struct __FileInfo__ {
    int fd;
    uint64_t size;
    uint32_t useCount;
};

class MmapPointer {
public:
    uint8_t *_pData;
    uint64_t _size;
    uint64_t _cursor;
    uint32_t _bytesRead;

    MmapPointer();
    virtual ~MmapPointer();
    uint8_t *GetPointer();
    bool Allocate(int fd, uint64_t cursor, uint32_t windowSize, uint64_t size);
    bool Free();
    uint64_t Copy(void *pBuffer, uint64_t cursor, uint64_t delta, uint64_t count);
    uint8_t GetState(MmapPointer & backBuffer);
    bool HasRange(uint64_t cursor, uint64_t count);
    operator string();
};

class MmapFile {
private:
    static map<string, __FileInfo__ > _fds;
    uint64_t _cursor;
    int _fd;
    string _path;
    uint64_t _size;
    bool _failed;

    uint32_t _windowSize;

    MmapPointer _pointer1;
    MmapPointer _pointer2;
public:
    static int32_t _pageSize;
public:
    MmapFile();
    virtual ~MmapFile();

    //init
    bool Initialize(string path, uint32_t windowSize, bool exclusive);

    //info
    int GetFd();
    uint64_t Size();
    uint64_t Cursor();
    bool IsEOF();
    string GetPath();
    bool Failed();
    bool IsOpen();

    //seeking
    bool SeekBegin();
    bool SeekEnd();
    bool SeekAhead(int64_t count);
    bool SeekBehind(int64_t count);
    bool SeekTo(uint64_t position);

    //read data
    bool ReadI8(int8_t *pValue);
    bool ReadI16(int16_t *pValue, bool networkOrder = true);
    bool ReadI24(int32_t *pValue, bool networkOrder = true);
    bool ReadI32(int32_t *pValue, bool networkOrder = true);
    bool ReadSI32(int32_t *pValue);
    bool ReadI64(int64_t *pValue, bool networkOrder = true);
    bool ReadUI8(uint8_t *pValue);
    bool ReadUI16(uint16_t *pValue, bool networkOrder = true);
    bool ReadUI24(uint32_t *pValue, bool networkOrder = true);
    bool ReadUI32(uint32_t *pValue, bool networkOrder = true);
    bool ReadSUI32(uint32_t *pValue);
    bool ReadUI64(uint64_t *pValue, bool networkOrder = true);
    bool ReadBuffer(uint8_t *pBuffer, uint64_t count);

    //peek data
    bool PeekI8(int8_t *pValue);
    bool PeekI16(int16_t *pValue, bool networkOrder = true);
    bool PeekI24(int32_t *pValue, bool networkOrder = true);
    bool PeekI32(int32_t *pValue, bool networkOrder = true);
    bool PeekSI32(int32_t *pValue);
    bool PeekI64(int64_t *pValue, bool networkOrder = true);
    bool PeekUI8(uint8_t *pValue);
    bool PeekUI16(uint16_t *pValue, bool networkOrder = true);
    bool PeekUI24(uint32_t *pValue, bool networkOrder = true);
    bool PeekUI32(uint32_t *pValue, bool networkOrder = true);
    bool PeekSUI32(uint32_t *pValue);
    bool PeekUI64(uint64_t *pValue, bool networkOrder = true);
    bool PeekBuffer(uint8_t *pBuffer, uint64_t count);
};

}
#endif	/* _MMAPFILE_H */



