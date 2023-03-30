#include<memory>
#include<string>
#include<unistd.h>
#include<iostream>
#include<sys/types.h>
#include<sys/socket.h>
#include<arpa/inet.h>
#include<sys/un.h>
#include<map>
#include<vector>



namespace sylar {


class IPAddress;


class Address {
public:
    typedef std::shared_ptr<Address> ptr;
    virtual ~Address() {}

    static Address::ptr Create(const sockaddr* addr, socklen_t addrlen);


    //Lookup这几个函数，是将域名转化为ip地址，
    static bool Lookup(std::vector<Address::ptr>& result, const std::string& host,
                int family = AF_INET, int type = 0, int protocol = 0);

    static Address::ptr LookupAny(const std::string& host, 
            int family = AF_INET, int type = 0, int protocol = 0);
            
    static IPAddress::ptr LookupAnyIPAddress(const std::string& host,
            int family = AF_INET, int type = 0, int protocol = 0);

    
    //GetINterfaceAddresses这两个函数是查询网卡地址，
    static bool GetInterfaceAddresses(std::multimap<std::string,
                std::pair<Address::ptr, uint32_t> >& result,
                int family = AF_INET);

    static bool GetInterfaceAddresses(std::vector<std::pair<Address::ptr, uint32_t> >& result,
                                    const std::string& iface, int family = AF_INET);

    int getFamliy() const;

public:
    virtual const sockaddr* getAddr() const = 0;

    virtual sockaddr* getAddr() = 0;

    virtual socklen_t getAddrLen() const = 0;

    virtual std::ostream& insert(std::ostream& os) const = 0;

    std::string toString() const;


    bool operator<(const Address& rhs) const;
    bool operator==(const Address& rhs) const;
    bool operator!=(const Address& rhs) const;
};



class IPAddress : public Address{
public:
    typedef std::shared_ptr<IPAddress> ptr;


    static IPAddress::ptr Create(const char* address, uint16_t port = 0);

    virtual IPAddress::ptr broadcastAddress(uint32_t prefix_len) =0;
    virtual IPAddress::ptr networdAddress(uint32_t prefix_len) = 0;
    virtual IPAddress::ptr subnetMask(uint32_t prefix_len) = 0;

   
    virtual uint32_t getPort() const = 0;
    virtual void setPort(uint16_t v) = 0;
};


class IPv4Address : public IPAddress{
public:
    typedef std::shared_ptr<IPv4Address> ptr;


    static IPv4Address::ptr Create(const char* address, uint16_t port = 0);

    IPv4Address(const sockaddr_in& address);
    IPv4Address(uint32_t address = INADDR_ANY, uint16_t port = 0);

    sockaddr* getAddr() override;

    const sockaddr* getAddr() const override;

    socklen_t getAddrLen() const override;

    std::ostream& insert(std::ostream& os) const override;

    IPAddress::ptr broadcastAddress(uint32_t prefix_len) override;
    IPAddress::ptr networdAddress(uint32_t prefix_len) override;
    IPAddress::ptr subnetMask(uint32_t prefix_len) override;
    uint32_t getPort() const override;
    void setPort(uint16_t v) override;

private:
    sockaddr_in m_addr;
};

class IPv6Address : public IPAddress {
public:
    typedef std::shared_ptr<IPv6Address> ptr;
    static IPv6Address::ptr Create(const char* address, uint32_t port = 0);


    IPv6Address();
    IPv6Address(const sockaddr_in6& address);
    IPv6Address(uint8_t address[16], uint32_t port = 0);    


    const sockaddr* getAddr() const override;

    sockaddr* getAddr() override;

    socklen_t getAddrLen() const override;

    std::ostream& insert(std::ostream& os) const override;


    IPAddress::ptr broadcastAddress(uint32_t prefix_len) override;

    IPAddress::ptr networdAddress(uint32_t prefix_len) override;
    
    IPAddress::ptr subnetMask(uint32_t prefix_len) override;
    
    uint32_t getPort() const override;
    
    void setPort(uint16_t v) override;

private:
    sockaddr_in6 m_addr;
};


class UnixAddress : public Address {
public:
    typedef std::shared_ptr<UnixAddress> ptr;
    UnixAddress();
    UnixAddress(const std::string& path);

    const sockaddr* getAddr() const override;

    sockaddr* getAddr() override;

    socklen_t getAddrLen() const override;


    std::ostream& insert(std::ostream& os) const override;

    void setAddrLen(uint32_t v) ;


private:
    struct sockaddr_un m_addr;
    socklen_t m_length;
};


class UnKnowAddress : public Address {
public:
    typedef std::shared_ptr<UnKnowAddress> ptr;

    UnKnowAddress(const sockaddr& addr);
    UnKnowAddress(int family);
    const sockaddr* getAddr() const override ;

    sockaddr* getAddr() override;

    socklen_t getAddrLen()  const override;

    std::ostream& insert(std::ostream& os) const override;
private:
    sockaddr m_addr;
};


std::ostream& operator<<(std::ostream& os, const Address& addr);

}