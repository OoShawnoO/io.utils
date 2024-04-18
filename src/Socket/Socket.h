/**
  ******************************************************************************
  * @file           : Socket.h
  * @author         : huzhida
  * @brief          : None
  * @date           : 2024/4/17
  ******************************************************************************
  */

#ifndef IO_UTILS_SOCKET_H
#define IO_UTILS_SOCKET_H

#include <climits>
#include <string>

#ifdef __linux__

#include <unistd.h>
#include <arpa/inet.h>

#define BAD_SOCKET (-1)

using SOCKET = int;
using FD = int;
#elif _WIN32

#include <winsock2.h>

#pragma comment(lib,"ws2_32.lib")

#define BAD_SOCKET (ULLONG_MAX)

namespace hzd {
    std::string GetWASockError() noexcept;
}

using FD = FILE*;
#endif


using SocketType = int;
#define BAD_SOCKET_TYPE (-1)

namespace hzd {
    // 抽象套接字
    // abstract socket
    class Socket {
    protected:
        // 套接字文件描述符
        // socket file descriptor
        SOCKET          sock{BAD_SOCKET};
        // 套接字类型
        // socket type
        SocketType      type{BAD_SOCKET_TYPE};
        // 本套接字地址
        // self socket addr
        sockaddr_in     self_addr{};
        // 目标套接字地址
        // destination socket addr
        sockaddr_in     dest_addr{};
        // 接收游标
        // recv cursor
        size_t          recv_cursor{0};
        // 发送游标
        // send cursor
        size_t          send_cursor{0};
        // 接收总字节数
        // recv bytes count
        size_t          recv_bytes_count{0};
        // 发送总字节数
        // send bytes count
        size_t          send_bytes_count{0};
        // 待发送文件描述符
        // ready send/recv file descriptor
        FD             fd{};
        // 是否为新的操作
        // whether new operate or not
        bool            is_new{true};

        void _init();
        /**
         * 发送数据 & send data
         * @param data 数据地址 & data address
         * @return >0 表示成功发送字节数,0表示需要稍后再次调用,-1表示失败 & return >0 for success send bytes count,0 for again,-1 for failed
         */
        virtual long sendImpl_(const char* data) = 0;
        /**
         * 接收数据 & recv data
         * @param data 保存数据的字符串数据 & string for data-save
         * @return >0 表示成功接收的字节数,0表示需要稍后再次调用,-1表示失败 & return >0 for success recv bytes count,0 for again,-1 for failed
         */
        virtual long recvImpl_(std::string& data) = 0;
    public:
        /**
         * 构造函数
         * constructor
         * @param type 套接字类型 & socket type
         */
        explicit Socket(SocketType type);
        /**
         *  析构函数
         *  destructor
         */
        virtual ~Socket();
        /**
         * 获取套接字文件描述符 & get socket file descriptor
         * @return 套接字文件描述符 & socket file descriptor
         */
        inline SOCKET Sock() const { return sock; };
        /**
         * 获取套接字本地地址 & get socket self address
         * @return 套接字本地地址 & socket self address
         */
        inline const sockaddr_in&  Addr() const { return self_addr; };
        /**
         * 获取套接字目标地址 & get socket destination address
         * @return 套接字目的地址 & socket destination address
         */
        inline const sockaddr_in& DestAddr() const { return dest_addr; };
        /**
         * 关闭套接字 & close socket
         * @return true表示成功,false表示失败 & true for success,false for failed
         */
        virtual bool Close();
        /**
         * 发送数据 & send data
         * @param data 数据地址 & data address
         * @param size 需要发送的数据大小 & size of data need send
         * @return >0 表示成功发送字节数,0表示需要稍后再次调用,-1表示失败 & return >0 for success send bytes count,0 for again,-1 for failed
         */
        virtual long Send(const char* data,size_t size) = 0;
        /**
         * 发送数据 & send data
         * @param data 数据地址 & data address
         * @return >0 表示成功发送字节数,0表示需要稍后再次调用,-1表示失败 & return >0 for success send bytes count,0 for again,-1 for failed
         */
        virtual long Send(const std::string& data);
        /**
         * 发送数据 & send data
         * @param data 数据地址 & data address
         * @return >0 表示成功发送字节数,0表示需要稍后再次调用,-1表示失败 & return >0 for success send bytes count,0 for again,-1 for failed
         */
        virtual long Send(std::string& data);
        /**
         * 接收数据 & recv data
         * @param data 保存数据的字符串数据 & string for data-save
         * @param size 需要接收数据大小 & size of data-save need
         * @param is_append 是否在字符串基础上添加 & whether append to raw string
         * @return >0 表示成功接收的字节数,0表示需要稍后再次调用,-1表示失败 & return >0 for success recv bytes count,0 for again,-1 for failed
         */
        virtual long Recv(std::string& data,size_t size,bool is_append) = 0;
        /**
         * 发送文件 & send file
         * @param file_path 文件路径 & file path
         * @return true 成功, false 失败 & true for success,false for failed
         */
        virtual bool SendFile(const std::string& file_path) = 0;
        /**
         * 接收文件 & recv file
         * @param file_path 文件路径 & file path
         * @param file_size 文件大小 & file size
         * @return true 成功, false 失败 & true for success,false for failed
         */
        virtual bool RecvFile(const std::string& file_path,size_t file_size) = 0;
    };

    class TcpSocket : public Socket {
    public:
        TcpSocket() : Socket(SOCK_STREAM) {}

        TcpSocket(SOCKET sock,sockaddr_in dest_addr);

        TcpSocket(const TcpSocket&) = delete;
        TcpSocket& operator=(TcpSocket&) = delete;
        TcpSocket(TcpSocket&& tcp_socket) noexcept;
        TcpSocket& operator=(TcpSocket&& tcp_socket) noexcept;

        long Send(const char *data, size_t size) override;

        long Send(const std::string& data) override;

        long Send(std::string& data) override;

        long Recv(std::string &data, size_t size, bool is_append) override;

        bool SendFile(const std::string &file_path) override;

        bool RecvFile(const std::string &file_path, size_t file_size) override;

    protected:
        long sendImpl_(const char *data) override;

        long recvImpl_(std::string &data) override;
    };

    class TcpListener : public TcpSocket {
    public:
        /**
         * 构造函数 & constructor
         * @param ip 绑定ip & bind ip
         * @param port 绑定端口 & bind port
         */
        TcpListener(const std::string& ip,unsigned short port);
        /**
         * bind address
         * @return true表示成功,false表示失败 & true for success,false for failed
         */
        bool Bind();
        /**
         * listen tcp socket
         * @return true表示成功,false表示失败 & true for success,false for failed
         */
        bool Listen();
        /**
         * accept new tcp socket
         * @param: tcp_socket 新连接对象返回值 & new tcp socket return
         * @return true表示成功,false表示失败 & true for success,false for failed
         */
        bool Accept(TcpSocket& tcp_socket);
    };

    class TcpClient : public TcpSocket {
    public:
        /**
         * 连接到目标套接字 & connect to destination socket
         * @param ip 目标IP & destination ip
         * @param port 目标端口 & destination port
         * @return true表示成功,false表示失败 & true for success,false for failed
         */
        bool Connect(const std::string& ip,unsigned short port);
    };

    class UdpSocket : public Socket {
        sockaddr_in             from_addr{};
        int                     from_addr_size{sizeof(sockaddr_in)};
    private:
        long Send(const char *data, size_t size) override;

        long Send(const std::string& data) override;

        long Send(std::string& data) override;

        bool SendFile(const std::string &file_path) override;

        long sendImpl_(const char *data) override;

        long recvImpl_(std::string &data) override;
    public:
        UdpSocket() : Socket(SOCK_DGRAM) {_init();}

        bool Bind(const std::string& ip,unsigned short port);
        /**
         * 发送数据到ip:port & send data to ip:port
         * @param ip 目标ip & destination ip
         * @param port 目标端口 & destination port
         * @param data 数据地址 & data address
         * @param size 数据大小 & data size
         * @return >0 表示成功接收的字节数,0表示需要稍后再次调用,-1表示失败 & return >0 for success recv bytes count,0 for again,-1 for failed
         */
        long SendTo(const std::string& ip,unsigned short port,const char* data,size_t size);
        /**
         * 发送数据到ip:port & send data to ip:port
         * @param ip 目标ip & destination ip
         * @param port 目标端口 & destination port
         * @param data 数据地址 & data address
         * @return >0 表示成功接收的字节数,0表示需要稍后再次调用,-1表示失败 & return >0 for success recv bytes count,0 for again,-1 for failed
         */
        long SendTo(const std::string& ip,unsigned short port,const std::string& data);
        /**
         * 发送数据到ip:port & send data to ip:port
         * @param ip 目标ip & destination ip
         * @param port 目标端口 & destination port
         * @param data 数据地址 & data address
         * @return >0 表示成功接收的字节数,0表示需要稍后再次调用,-1表示失败 & return >0 for success recv bytes count,0 for again,-1 for failed
         */
        long SendTo(const std::string& ip,unsigned short port,std::string& data);
        /**
         * 发送文件到ip:port & send file to ip:port
         * @param ip 目标ip & destination ip
         * @param port 目标端口 & destination port
         * @param file_path 文件路径 & file path
         * @return
         */
        bool SendFileTo(const std::string& ip,unsigned short port,const std::string& file_path);

        long Recv(std::string &data, size_t size, bool is_append) override;

        bool RecvFile(const std::string &file_path, size_t file_size) override;
    };
} // hzd

#endif //IO_UTILS_SOCKET_H
