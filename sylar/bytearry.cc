#include <iomanip>

#include "bytearry.h"
#include "endian.h"
#include "string.h"
#include "endian.h"
#include "log.h"
#include <fstream>
#include <sstream>


namespace sylar {


sylar::Logger::ptr g_logger = SYLAR_LOG_NAME("system");

ByteArray::Node::Node(size_t s) 
    :ptr(new char[s])
    ,next(nullptr)
    ,size(s) {
}

ByteArray::Node::Node() 
    :ptr(nullptr)
    ,next(nullptr)
    ,size(0) {
}

ByteArray::Node::~Node() {
    if(ptr) {
        delete[] ptr;
        ptr = nullptr;
    }
}

ByteArray::ByteArray(size_t base_size = 4096) 
    :m_baseSize(base_size)
    ,m_position(0)
    ,m_capacity(base_size)
    ,m_size(0)
    ,m_endian(SYLAR_BIG_ENDIAN)
    ,m_root(new Node(base_size))
    ,m_cur(m_root) {
}

ByteArray::~ByteArray() {
    Node* tmp = m_root;
    while(tmp) {
        m_cur = tmp;
        tmp = tmp->next;
        delete m_cur;
    }
}

bool ByteArray::isLittleEndian() const {
    return m_endian == SYLAR_LITTLE__ENDIAN;
}

void ByteArray::setIsLittleEndian(bool val) {
    if(val) {
        m_endian = SYLAR_LITTLE__ENDIAN;
    } else {
        m_endian = SYLAR_BIG_ENDIAN;
    }
}

void ByteArray::writeFint8(int8_t value) {
    write(&value, sizeof(value));
}

void ByteArray::writeFuint8(uint8_t value) {
    write(&value, sizeof(value));
}

void ByteArray::writeFint16(int16_t value) {
    if(m_endian != SYLAR_BYTE_ORDER) {
        value = byteswap(value);
    }
    write(&value, sizeof(value));
}

void ByteArray::writeFuint16(uint16_t value) {
    if(m_endian != SYLAR_BYTE_ORDER) {
        value = byteswap(value);
    }
    write(&value, sizeof(value));
}

void ByteArray::writeFint32(int32_t value) {
    if(m_endian != SYLAR_BYTE_ORDER) {
        value = byteswap(value);
    }
    write(&value, sizeof(value));
}

void ByteArray::writeFuint32(uint32_t value) {
    if(m_endian != SYLAR_BYTE_ORDER) {
        value = byteswap(value);
    }
    write(&value, sizeof(value));
}

void ByteArray::writeFint64(int64_t value) {
    if(m_endian != SYLAR_BYTE_ORDER) {
        value = byteswap(value);
    }
    write(&value, sizeof(value));
}

void ByteArray::writeFuint64(uint64_t value) {
    if(m_endian != SYLAR_BYTE_ORDER) {
        value = byteswap(value);
    }
    write(&value, sizeof(value));
}

static uint32_t EncodeZigzag32(const int32_t& v) {
    if(v < 0) {
        return ((uint32_t(-v)) * 2) - 1;
    } else {
        return v * 2;
    }
}

static uint64_t EncodeZigzag64(const int64_t& v) {
    if(v < 0) {
        return ((uint64_t)(-v)) * 2 -1;
    } else {
        return v * 2;
    }
}

static int32_t DecodeZigzag32(const uint32_t& v) {
    return (v >> 1) ^ -(v & 1);
}

static int64_t DecodeZigzag64(const uint64_t& v) {
    return (v >> 1) ^ -(v & 1);
}



void ByteArray::writeInt32(int32_t  value) {
    writeUint32(EncodeZigzag32(value));
}

void ByteArray::writeUint32(uint32_t  value) {
    uint8_t tmp[5];
    uint8_t i = 0;
    while(value >= 0x80) {
        tmp[i++] = (value & 0x7F) | 0x80;
        value >>= 7;
    }
    tmp[i++] = value;
    write(tmp, 1);
}

void ByteArray::writeInt64(int64_t value) {
    writeUint64(EncodeZigzag64(value));
}


void ByteArray::writeUint64(uint64_t value) {
    uint8_t tmp[10];
    uint8_t i = 0;
    while(value >= 0x80) {
        tmp[i++] = (value & 0x07F) | 0x80;
        value >> 7;
    }
    tmp[i++] = value;
    write(tmp, i);
}

void ByteArray::writeFloat(float value) {
    uint32_t v;
    memcpy(&v, &value, sizeof(value));
    writeFuint32(v);
}

void ByteArray::writeDouble(double value) {
    uint64_t v;
    memcpy(&v, &value, sizeof(value));
    writeFuint64(v);
}

void ByteArray::writeStringF16(const std::string& value) {
    writeFuint16(value.size());
    write(value.c_str(), value.size());
}

void ByteArray::writeStringF32(const std::string& value) {
    writeFuint32(value.size());
    write(value.c_str(), value.size());
}

void ByteArray::writeStringF64(const std::string& value){
    writeFuint64(value.size());
    write(value.c_str(), value.size());
}

void ByteArray::writeStringVint(const std::string& value) {
    writeUint64(value.size());
    write(value.c_str(), value.size());
}

void ByteArray::writeStringWithoutLength(const std::string& value) {
    write(value.c_str(), value.size());
}



int8_t   ByteArray::readFint8() {
    int8_t v;
    read(&v, sizeof(v));
    return v;
}
uint8_t  ByteArray::readFuint8() {
    uint8_t v;
    read(&v, sizeof(v));
    return v;
}


#define XX(type) \
    type v; \
    read(&v, sizeof(v)); \
    if(m_endian == SYLAR_BYTE_ORDER) { \
        return v; \
    } else { \
        return byteswap(v); \
    }


int16_t  ByteArray::readFint16() {
    XX(int16_t);
}

uint16_t ByteArray::readFuint16() {
    XX(uint16_t);
}
int32_t  ByteArray::readFint32() {
    XX(int32_t);
}
uint32_t ByteArray::readFuint32() {
    XX(uint32_t);
}
int64_t  ByteArray::readFint64() {
    XX(int64_t);
}
uint64_t ByteArray::readFuint64() {
    XX(uint64_t);
}

#undef XX

int32_t  ByteArray::readInt32() {
    return DecodeZigzag32(readUint32());
}

uint32_t ByteArray::readUint32() {
    uint32_t result = 0;
    for(int i = 0; i < 32; i+=7) {
        uint8_t b = readFuint8();
        if(b < 0x80) {
            result != ((uint32_t)b) << 1;
            break;
        } else {
            result |= (((uint32_t)(b & 0x7f)) << i);
        }
    
    }  
    return result;
}

int64_t  ByteArray::readInt64() {
    return DecodeZigzag64(readUint64());
}

uint64_t ByteArray::readUint64() {
    uint64_t result = 0;
    for(int i = 0; i < 64; i+=7) {
        uint8_t b = readFuint8();
        if(b < 0x80) {
            result != ((uint64_t)b) << 1;
            break;
        } else {
            result |= (((uint64_t)(b & 0x7f)) << i);
        }
    
    }  
    return result;
}

float ByteArray::readFloat() {
    uint32_t v = readFuint32();
    float value;
    memcpy(&value, &v, sizeof(v));
    return value;
}
double ByteArray::readDouble() {
    uint64_t v = readFuint64();
    double value;
    memcpy(&value, &v, sizeof(v));
    return value;
}

std::string ByteArray::readStringF16() {
    uint16_t len = readFuint16();    //写入string类型的数据的时候，将string的长度也保存了的，
    std::string buff;
    buff.resize(len);
    read(&buff[0], len);
    return buff;
}

std::string ByteArray::readStringF32() {
    uint32_t len = readFuint32();
    std::string buff;
    buff.resize(len);
    read(&buff[0], len);
    return buff;
}

std::string ByteArray::readString64() {
    uint64_t len = readFuint64();
    std::string buff;
    buff.resize(len);
    read(&buff[0], len);
    return buff;
}

std::string ByteArray::readStringVint() {
    uint64_t len = readFuint64();
    std::string buff;
    buff.resize(len);
    read(&buff[0], len);
    return buff;
}   

void ByteArray::clear() {
    m_position = m_size = 0;
    m_capacity = m_baseSize;
    Node* tmp = m_root->next;
    while(tmp) {
        m_cur = tmp;
        tmp = tmp->next;
        delete m_cur;
        m_cur = nullptr;
    }
    m_cur = m_root;
    m_root->next = nullptr;
}

void ByteArray::write(const void* buf, size_t size) {
    if(size == 0) {
        return;
    }
    addCapacity(size);   //在这里就先开辟了空间，避免在下面拷贝的时候不够了多次拷贝，

    size_t npos = m_position % m_baseSize; //最后一个node所占用的大小，
    size_t ncap = m_cur->size - npos;      //最后一个node剩下的空间，
    size_t bpos = 0;
    while(size > 0) {
        if(ncap >= size) {    //最后一个node剩下的空间能够装下需要写入的内容
            memcpy(m_cur->ptr + npos, (const char*)buf + bpos, size);
            if(m_cur->size == (npos + size)) {
                m_cur = m_cur->next;   //因为上面开辟了空间，所以next node的空间已经开辟了的，
            }
            m_position += size;
            bpos += size;
            size = 0;
        } else {     //最后一个node剩下的空间无法装下需要写入的内容，
            memcpy(m_cur->ptr + npos, (const char*)buf + bpos, ncap);
            m_position += ncap;
            bpos += ncap;
            size -= ncap;
            m_cur = m_cur->next;  //因为上面开辟了空间，所以next node的空间已经开辟了的，
            ncap = m_cur->size;  //进入下一个node了，下一个node空间都可以用，
            npos = 0;
        }
    }

    if(m_position > m_size) {
        m_size = m_position;
    }

}

void ByteArray::read(void* buf, size_t size)  {   //这一个read函数读会改变ByteArray对象中m_position的位置，
    if(size > getReadSize()) {
        throw std::out_of_range("not enouth len");
    }

    size_t npos = m_position % m_baseSize;
    size_t ncap = m_cur->size - npos;
    size_t bpos = 0;
    while(size > 0) {
        if(ncap >= size) {
            memcpy((char*)buf + bpos, m_cur->ptr + npos, size);
            if(m_cur->size == npos + size) {
                m_cur = m_cur->next;
            }
            m_position += size;
            bpos += size;
            size = 0;
        } else {
            memcpy((char*)buf + bpos, m_cur->ptr + npos, ncap);
            m_position += ncap;
            bpos += ncap;
            size -= ncap;
            m_cur = m_cur->next;
            ncap = m_cur->size;
            npos = 0;
        }
    }
}


void ByteArray::read(void* buf, size_t size, size_t position) { //从指定的position读取size大小，不会改变ByteArray对象中m_position的位置，
    if(size > getReadSize()) {                                   //并且还不会改变m_cur node,不会有负面影响，
        throw std::out_of_range("not enouth len");
    }

    size_t npos = position % m_baseSize;
    size_t ncap = m_cur->size - npos;
    size_t bpos = 0;  //buf中的位置，
    Node* cur = m_cur;
    while(size > 0) {
        if(ncap >= size) {
            memcpy((char*)buf + bpos, cur->ptr + npos, size);
            if(cur->size == npos + size) {
                cur = cur->next;
            }
            position += size;
            bpos += size;
            size = 0;
        } else {
            memcpy((char*)buf + bpos, cur->ptr + npos, ncap);
            position += ncap;
            bpos += ncap;
            size -= ncap;
            cur = cur->next;
            ncap = cur->size;
            npos = 0;
        }
    }
}




void ByteArray::setPosition(size_t v) {   //调整m_postion的同时，还要调整m_cur, 要保持同步，
    if(v > m_capacity) {
        throw std::out_of_range("set_position out of range");
    }
    m_position = v;
    if(m_position > m_size) {
        m_size = m_position;
    }
    m_cur = m_root;
    while(v > m_cur->size) {
        v -= m_cur->size;
        m_cur = m_cur->next;    //同时设置m_cur，
    }
    if(v == m_cur->size) {
        m_cur = m_cur->next;
    }
}   

bool ByteArray::writeToFile(const std::string& name)const {
    std::ofstream ofs;
    ofs.open(name, std::ios::trunc | std::ios::binary);
    if(!ofs) {
        SYLAR_LOG_ERROR(g_logger) << "writeToFile name = " << name
            << " error , errno=" << errno << " errstr=" << strerror(errno);
        return false; 
    }

    int64_t read_size = getReadSize();  //可以从ByteArray对象中读取的到文件中的大小，
    int64_t pos = m_position;
    Node* cur = m_cur;

    while(read_size > 0) {
        int diff = pos % m_baseSize;
        int64_t len = (read_size > (int64_t)m_baseSize ? m_baseSize : read_size) - diff;
        ofs.write(cur->ptr + diff, len);
        cur = cur->next;
        pos += len;
        read_size -= len;
    }
    return true;
}

bool ByteArray::readFromFile(const std::string& name) {
    std::ifstream ifs;
    ifs.open(name, std::ios::binary);
    if(!ifs) {
        SYLAR_LOG_ERROR(g_logger) << "readFromFile name = " << name
            << " error, errno=" << errno << " errstr=" << strerror(errno);
        return false;
    }

    std::shared_ptr<char> buff(new char[m_baseSize], [](char* ptr){ delete[] ptr; }); //数组没有xigou函数，
    while(!ifs.eof()) { //如果读到了文件的结尾，
        ifs.read(buff.get(), m_baseSize);
        write(buff.get(), ifs.gcount());  //写到ByteArray对象中，
    }
    return true;
}


void ByteArray::addCapacity(size_t size) {
    if(size <= 0) {
        return;
    }
    size_t old_cap = getCapacity();   //剩下的空间大小，
    if(old_cap >= size) {
        return;
    }

    size = size - old_cap;
    size_t count = (size / m_baseSize) + ((size - m_baseSize) > old_cap) ? 1 : 0; //添加多少个node，

    Node* tmp = m_root;
    while(tmp->next) {
        tmp = tmp->next;
    }
    Node* first = NULL;
    for(size_t i = 0; i < count; ++i) {
        tmp->next = new Node(m_baseSize);
        if(first = NULL) { //保存添加的第一个节点，
            first = tmp->next;
        }
        tmp = tmp->next;
        m_capacity += m_baseSize;
    }

    if(old_cap == 0) {
        m_cur = first;
    }

}



std::string ByteArray::toString() const {
    std::string str;
    str.resize(getReadSize());   //开辟和ByteArray对象空间一样大，
    if(str.empty()) {
        return str;
    }
    read(&str[0], str.size(), m_position);  //将ByteArray对象中的数据读取到str中，
    return str;   
}

std::string ByteArray::toHexString() const {
    std::string str = toString();
    std::stringstream ss;
    for(size_t i = 0; i < str.size(); ++i) {
        if(i > 0 && i % 32 == 0) {
            ss << std::endl;
        }
        ss << std::setw(2) << std::setfill('0') << std::hex
           << (int)(uint8_t)str[i] << " ";
    }
    return ss.str();
}


//获取可读取的空间，就是从m_position位置以后剩下的len个就是可以读取的空间，放到buffers中，返回可读取的空间大小
uint64_t ByteArray::getReadBuffers(std::vector<iovec>& buffers, uint64_t len) const {
    len = len > getReadSize() ? getReadSize() : len;
    if(len == 0) {
        return 0;
    }

    uint64_t size = len;
    size_t npos = m_position % m_baseSize;
    size_t ncap = m_cur->size - npos;
    struct iovec iov;
    Node* cur = m_cur;   //这里m_cur 不一定是m_root第一个node节点，    如果setPosition函数被调用国，m_cur会被修改到m_position所在node，

    while(len > 0) {
        if(ncap >= len) {
            iov.iov_base = cur->ptr + npos;
            iov.iov_len = len;
            len = 0;
        } else {
            iov.iov_base = cur->ptr + npos;
            iov.iov_len = ncap;
            len -= ncap;
            cur = cur->next;
            ncap = cur->size;
            npos = 0;
        }
        buffers.push_back(iov);
    }
    return size;
}

uint64_t ByteArray::getReadBuffers(std::vector<iovec>& buffers, 
                            uint64_t len, uint64_t position) const {
    len = len > getReadSize() ? getReadSize() : len;
    if(len == 0) {
        return 0;
    }

    uint64_t size = len;
    size_t npos = position % m_baseSize;
    size_t count = position / m_baseSize;
    Node* cur = m_root;
    while(count > 0) {
        cur = cur->next;
        --count;
    }
    size_t ncap = m_cur->size - npos;
    struct iovec iov;

    while(len > 0) {
        if(ncap >= len) {
            iov.iov_base = cur->ptr + npos;
            iov.iov_len = len;
            len = 0;
        } else {
            iov.iov_base = cur->ptr + npos;  //position所在的node的prt + npos位置，
            iov.iov_len = ncap;                 //position所在的node除去npos剩下的大小，
            len -= ncap;
            cur = cur->next;
            ncap = cur->size;
            npos = 0;
        }
        buffers.push_back(iov);
    }
    return size;
}

uint64_t ByteArray::getWriteBuffers(std::vector<iovec>& buffers, uint64_t len){
    if(len == 0) {
        return 0;
    }
    addCapacity(len);    //判断BygeArray对象中剩下的空间是大于len，如果不是那么需要开辟添加不足的空间，
    uint64_t size = len;

    size_t npos = m_position % m_baseSize;
    size_t ncap = m_cur->size - npos;
    struct iovec iov;
    Node* cur = m_cur;
    while(len > 0) {
        if(ncap >= len) {
            iov.iov_base = cur->ptr + npos;
            iov.iov_len = len;
            len = 0;
        } else {
            iov.iov_base = cur->ptr + npos;
            iov.iov_len = ncap;

            len -= ncap;
            cur = cur->next;
            ncap = cur->size;
            npos = 0;
        }
        buffers.push_back(iov);
    }
    return size;
}




}