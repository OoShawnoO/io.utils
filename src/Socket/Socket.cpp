/**
  ******************************************************************************
  * @file           : Socket.cpp
  * @author         : huzhida
  * @brief          : None
  * @date           : 2024/4/17
  ******************************************************************************
  */
#ifdef __linux__
#include <fcntl.h>
#include <cstring>
#include <sys/sendfile.h>
#include <sys/stat.h>
#elif _WIN32
#define _CRT_SECURE_NO_WARNINGS
#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include <WS2tcpip.h>
#endif
#include <Mole.h>
#include "Socket.h"
#include <fstream>


namespace hzd {
    
    const std::string io_socket_channel = "io.Socket";
    
#ifdef _WIN32
    std::string GetWASockError() noexcept {
        LPSTR lpBuffer = nullptr;
        DWORD dwFlags = FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS;
        if(FormatMessageA(dwFlags,nullptr,WSAGetLastError(),0,(LPSTR)&lpBuffer,0,nullptr)) {
            std::string err = lpBuffer;
            LocalFree(lpBuffer);
            return err;
        }
        return "ErrorCode:" + std::to_string(WSAGetLastError());
    }
#endif

    Socket::Socket(SocketType type_) {
#ifdef _WIN32
        static bool is_startup = false;
        if(!is_startup) {
            WSADATA wsa_data;
            int err = WSAStartup(MAKEWORD(2,2),&wsa_data);
            if(err != 0) {
                MOLE_ERROR(io_socket_channel,GetWASockError());
                exit(-1);
            }
            is_startup = true;
        }
#endif
        type = type_;
    }

    Socket::~Socket() {
        Socket::Close();
    }

    bool Socket::Close() {
        if(sock == BAD_SOCKET) return true;
        int ret;
#ifdef __linux__
        ret = close(sock);
        if(ret == -1) {
            MOLE_ERROR(io_socket_channel,strerror(errno));
            return false;
        }
#elif _WIN32
        ret = closesocket(sock);
        if(ret == SOCKET_ERROR) {
            MOLE_ERROR(io_socket_channel,GetWASockError());
            return false;
        }
#endif
        sock = BAD_SOCKET;
        return true;
    }

    ssize_t Socket::Send(const std::string &data) {
        return Send(data.c_str(),data.size());
    }

    ssize_t Socket::Send(std::string &data) {
        return Send(data.c_str(),data.size());
    }

    void Socket::_init() {
        sock = socket(AF_INET,type,0);
        int reuse = 1;
        setsockopt(sock,SOL_SOCKET,SO_REUSEADDR,(const char*)&reuse,sizeof(reuse));
#ifdef __linux__
        setsockopt(sock,SOL_SOCKET,SO_REUSEPORT,(const char*)&reuse,sizeof(reuse));
#endif
    }

    TcpSocket::TcpSocket(SOCKET sock_, sockaddr_in dest_addr_) : Socket(SOCK_STREAM) {
        sock = sock_;
        dest_addr = dest_addr_;
    }

    ssize_t TcpSocket::sendImpl_(const char *data) {
        size_t need_send_bytes;
        ssize_t had_send_bytes;
        while(send_cursor < send_bytes_count) {
            need_send_bytes = send_bytes_count - send_cursor;
        #ifdef __linux__
            int flag = MSG_NOSIGNAL;
        #elif _WIN32
            int flag = 0;
        #endif
            if((had_send_bytes = send(sock,data + send_cursor,static_cast<int>(need_send_bytes),flag)) <= 0) {
                if(errno == EAGAIN || errno == EWOULDBLOCK) {
                    is_new = false;
                    return 0;
                }
                #ifdef __linux__
                MOLE_ERROR(io_socket_channel,strerror(errno));
                #elif _WIN32
                MOLE_ERROR(io_socket_channel,GetWASockError());
                #endif
                is_new = true;
                return -1;
            }
            send_cursor += had_send_bytes;
        }
        is_new = true;
        return static_cast<ssize_t>(send_bytes_count);
    }

    ssize_t TcpSocket::recvImpl_(std::string &data) {
        ssize_t had_recv_bytes;
        if(data.size() < recv_bytes_count) {
            data.reserve(recv_bytes_count);
        }
        char buffer[4096] = {0};
        while(recv_cursor < recv_bytes_count) {
            memset(buffer,0,sizeof(buffer));
            if((had_recv_bytes = recv(sock,buffer,sizeof(buffer),0)) <= 0) {
                if(errno == EAGAIN || errno == EWOULDBLOCK) {
                    is_new = false;
                    return 0;
                }
                #ifdef __linux__
                    MOLE_ERROR(io_socket_channel,strerror(errno));
                #elif _WIN32
                MOLE_ERROR(io_socket_channel,GetWASockError());
                #endif
                is_new = true;
                return -1;
            }
            recv_cursor += had_recv_bytes;
            data.append(buffer,had_recv_bytes);
        }
        is_new = true;
        return static_cast<ssize_t>(recv_bytes_count);
    }

    ssize_t TcpSocket::Send(const char *data, size_t size) {
        if(is_new) {
            if(size <= 0) return -1;
            send_bytes_count = size;
            send_cursor = 0;
            is_new = false;
        }
        return sendImpl_(data);
    }

    ssize_t TcpSocket::Recv(std::string &data, size_t size, bool is_append) {
        if(is_new) {
            if(!is_append) data.clear();
            recv_bytes_count = size;
            recv_cursor = 0;
            is_new = false;
        }
        return recvImpl_(data);
    }

    bool TcpSocket::SendFile(const std::string &file_path) {
#ifdef __linux__
        if(is_new) {
            fd = open(file_path.c_str(),O_RDONLY);
            if(fd < 0) {
                MOLE_ERROR(io_socket_channel,strerror(errno));
                return false;
            }
            struct stat stat{};
            fstat(fd,&stat);
            send_bytes_count = stat.st_size;
            send_cursor = 0;
        }
        ssize_t had_send_bytes;
        while(send_cursor < send_bytes_count) {
            auto offset = (off_t)send_cursor;
            if((had_send_bytes = sendfile(sock,fd,&offset,send_bytes_count - send_cursor)) < 0) {
                if(errno == EAGAIN || errno == EWOULDBLOCK) {
                    continue;
                }
                MOLE_ERROR(io_socket_channel,strerror(errno));
                close(fd);
                fd = -1;
                return false;
            }
            send_cursor += had_send_bytes;
        }
        close(fd);
        fd = -1;
        is_new = true;
        return true;
#elif _WIN32
        if(is_new) {
            fd = fopen(file_path.c_str(),"rb");
            if(!fd) {
                MOLE_ERROR(io_socket_channel,strerror(errno));
                return false;
            }
            fseek(fd,0,SEEK_END);
            send_bytes_count = ftell(fd);
            fseek(fd,0,SEEK_SET);
            send_cursor = 0;
            is_new = false;
        }
        size_t need_send_bytes;
        ssize_t had_send_bytes;
        char send_buffer[4096] = {0};
        while(send_cursor < send_bytes_count) {
            need_send_bytes = fread(send_buffer,sizeof(char),sizeof(send_buffer),fd);
            if((had_send_bytes = send(sock,send_buffer,static_cast<int>(need_send_bytes),0))<= 0) {
                if(errno == EAGAIN || errno == EWOULDBLOCK) {
                    continue;
                }
                MOLE_ERROR(io_socket_channel,std::to_string(WSAGetLastError()));
                fclose(fd);
                fd = nullptr;
                return false;
            }
            send_cursor += had_send_bytes;
        }
        fclose(fd);
        fd = nullptr;
        is_new = true;
        return true;
#endif
    }

    bool TcpSocket::RecvFile(const std::string &file_path, size_t file_size) {
#ifdef __linux__
        if(is_new){
            recv_cursor = 0;
            recv_bytes_count = file_size;
            is_new = false;
            fd = open(file_path.c_str(),O_CREAT | O_WRONLY,0755);
        }
        ssize_t had_recv_bytes;
        size_t need_recv_bytes;
        char recv_buffer[4096] = {0};
        while(recv_cursor < recv_bytes_count){
            bzero(recv_buffer,sizeof(recv_buffer));
            need_recv_bytes = (recv_bytes_count - recv_cursor) > sizeof(recv_buffer) ? sizeof(recv_buffer) : (recv_bytes_count - recv_cursor);
            if((had_recv_bytes = ::recv(sock,recv_buffer,need_recv_bytes,0)) < 0){
                if(errno == EAGAIN || errno == EWOULDBLOCK){
                    continue;
                }
                close(fd);
                fd = -1;
                return false;
            }
            write(fd,recv_buffer,had_recv_bytes);
            recv_cursor += had_recv_bytes;
        }
        is_new = true;
        close(fd);
        fd = -1;
        return true;
#elif _WIN32
        if(is_new) {
            fd = fopen(file_path.c_str(),"wb");
            if(fd == nullptr) {
                MOLE_ERROR(io_socket_channel,strerror(errno));
                return false;
            }
            recv_bytes_count = file_size;
            recv_cursor = 0;
            is_new = false;
        }
        ssize_t had_recv_bytes;
        char recv_buffer[4096] = {0};
        while(recv_cursor < recv_bytes_count) {
            if((had_recv_bytes = recv(sock,recv_buffer,sizeof(recv_buffer),0)) < 0) {
                if(errno == EAGAIN || errno == EWOULDBLOCK) {
                    continue;
                }
                MOLE_ERROR(io_socket_channel,strerror(errno));
                fclose(fd);
                fd = nullptr;
                return false;
            }
            fwrite(recv_buffer,had_recv_bytes,1,fd);
            recv_cursor += had_recv_bytes;
        }
        fclose(fd);
        fd = nullptr;
        is_new = true;
        return true;
#endif
    }

    TcpSocket::TcpSocket(TcpSocket &&tcp_socket) noexcept : Socket(tcp_socket.type) {
        sock = tcp_socket.sock;
        type = tcp_socket.type;
        self_addr = tcp_socket.self_addr;
        dest_addr = tcp_socket.dest_addr;
        recv_cursor = tcp_socket.recv_cursor;
        recv_bytes_count = tcp_socket.recv_bytes_count;
        send_cursor = tcp_socket.send_cursor;
        send_bytes_count = tcp_socket.send_bytes_count;
        fd = tcp_socket.fd;
        is_new = tcp_socket.is_new;

        tcp_socket.sock = BAD_SOCKET;
    }

    TcpSocket &TcpSocket::operator=(TcpSocket &&tcp_socket) noexcept {
        sock = tcp_socket.sock;
        type = tcp_socket.type;
        self_addr = tcp_socket.self_addr;
        dest_addr = tcp_socket.dest_addr;
        recv_cursor = tcp_socket.recv_cursor;
        recv_bytes_count = tcp_socket.recv_bytes_count;
        send_cursor = tcp_socket.send_cursor;
        send_bytes_count = tcp_socket.send_bytes_count;
        fd = tcp_socket.fd;
        is_new = tcp_socket.is_new;

        tcp_socket.sock = BAD_SOCKET;
        return *this;
    }

    long TcpSocket::Send(const std::string &data) {
        return Socket::Send(data);
    }

    long TcpSocket::Send(std::string &data) {
        return Socket::Send(data);
    }

    TcpListener::TcpListener(const std::string &ip, unsigned short port) {
        self_addr.sin_family = AF_INET;
        self_addr.sin_addr.s_addr = inet_addr(ip.c_str());
        self_addr.sin_port = htons(port);

        Socket::_init();
    }


    bool TcpListener::Bind() {
        if(bind(sock,(sockaddr*)&self_addr,sizeof(self_addr)) < 0) {
            return false;
        }
        return true;
    }

    bool TcpListener::Listen() {
        if(listen(sock,1024) < 0) {
            return false;
        }
        return true;
    }

    bool TcpListener::Accept(TcpSocket &tcp_socket) {
        SOCKET sock_;
        sockaddr_in dest_addr_{};
        socklen_t  dest_addr_len = sizeof(dest_addr_);
        if((sock_ = accept(sock,(sockaddr*)&dest_addr_,&dest_addr_len)) < 0) return false;
        tcp_socket = {sock_,dest_addr_};
        return true;
    }


    bool TcpClient::Connect(const std::string &ip, unsigned short port) {
        if(sock != BAD_SOCKET) Close();
        _init();

        dest_addr.sin_addr.s_addr = inet_addr(ip.c_str());
        dest_addr.sin_port = htons(port);
        dest_addr.sin_family = AF_INET;
#ifdef __linux__
        int option = fcntl(sock,F_GETFL);
        int newOption = option | O_NONBLOCK;
        fcntl(sock,F_SETFL,newOption);

        while(connect(sock,(sockaddr*)&dest_addr,sizeof(dest_addr)) < 0){
            if(errno != EINPROGRESS && errno != EALREADY) {
                MOLE_ERROR(io_socket_channel, strerror(errno));
                return false;
            }
        }

        fcntl(sock,F_SETFL,option);
#elif _WIN32
        if(connect(sock,(sockaddr*)&dest_addr,sizeof(dest_addr))< 0) {
            MOLE_ERROR(io_socket_channel,strerror(errno));
            return false;
        }
#endif
        return true;
    }


    ssize_t UdpSocket::sendImpl_(const char *data) {
        size_t need_send_bytes;
        ssize_t had_send_bytes;
        while(send_cursor < send_bytes_count) {
            need_send_bytes = send_bytes_count - send_cursor;
#ifdef __linux__
            int flag = MSG_NOSIGNAL;
#elif _WIN32
            int flag = 0;
#endif
            if((had_send_bytes = sendto(sock,data + send_cursor,static_cast<int>(need_send_bytes),flag,(const sockaddr*)&dest_addr,sizeof(dest_addr))) <= 0) {
                if(errno == EAGAIN || errno == EWOULDBLOCK) {
                    is_new = false;
                    return 0;
                }
#ifdef __linux__
                    MOLE_ERROR(io_socket_channel,strerror(errno));
#elif _WIN32
                MOLE_ERROR(io_socket_channel,GetWASockError());
#endif
                is_new = true;
                return -1;
            }
            send_cursor += had_send_bytes;
        }
        is_new = true;
        return static_cast<ssize_t>(send_bytes_count);
    }

    ssize_t UdpSocket::recvImpl_(std::string &data) {
        ssize_t had_recv_bytes;
        if(data.size() < recv_bytes_count) {
            data.reserve(recv_bytes_count);
        }
        char buffer[4096] = {0};
        while(recv_cursor < recv_bytes_count) {
            if((had_recv_bytes = recvfrom(sock,buffer,sizeof(buffer),0,(struct sockaddr*)&from_addr,(socklen_t*)&from_addr_size)) <= 0) {
                if(errno == EAGAIN || errno == EWOULDBLOCK) {
                    is_new = false;
                    return 0;
                }
#ifdef __linux__
                MOLE_ERROR(io_socket_channel,strerror(errno));
#elif _WIN32
                MOLE_ERROR(io_socket_channel,std::to_string(WSAGetLastError()));
#endif
                is_new = true;
                return -1;
            }
            recv_cursor += had_recv_bytes;
            data.append(buffer,had_recv_bytes);
        }
        is_new = true;
        return static_cast<ssize_t>(recv_bytes_count);
    }

    ssize_t UdpSocket::Send(const char *data, size_t size) {
        if(is_new) {
            if(size <= 0) return -1;
            send_bytes_count = size;
            send_cursor = 0;
            is_new = false;
        }
        return sendImpl_(data);
    }

    ssize_t UdpSocket::Recv(std::string &data, size_t size, bool is_append) {
        if(is_new) {
            if(!is_append) data.clear();
            recv_bytes_count = size;
            recv_cursor = 0;
            is_new = false;
        }
        return recvImpl_(data);
    }

    bool UdpSocket::SendFile(const std::string &file_path) {
#ifdef __linux__
        if(is_new) {
            fd = open(file_path.c_str(),O_RDONLY);
            if(fd < 0) {
                MOLE_ERROR(io_socket_channel,strerror(errno));
                return false;
            }
            struct stat stat{};
            fstat(fd,&stat);
            send_bytes_count = stat.st_size;
            send_cursor = 0;
        }
        ssize_t need_send_bytes;
        ssize_t had_send_bytes;
        char buffer[4096] = {0};
        while(send_cursor < send_bytes_count) {
            need_send_bytes = read(fd,buffer,sizeof(buffer));
            if((had_send_bytes = sendto(sock,buffer,need_send_bytes,0,(sockaddr*)&dest_addr,sizeof(dest_addr))) < 0) {
                if(errno == EAGAIN || errno == EWOULDBLOCK) {
                    continue;
                }
                MOLE_ERROR(io_socket_channel,strerror(errno));
                close(fd);
                fd = -1;
                return false;
            }
            send_cursor += had_send_bytes;
        }
        close(fd);
        fd = -1;
        is_new = true;
        return true;
#elif _WIN32
        if(is_new) {
            fd = fopen(file_path.c_str(),"rb");
            if(fd == nullptr) {
                MOLE_ERROR(io_socket_channel,strerror(errno));
                return false;
            }
            fseek(fd,0,SEEK_END);
            send_bytes_count = ftell(fd);
            fseek(fd,0,SEEK_SET);
            send_cursor = 0;
            is_new = false;
        }
        size_t need_send_bytes;
        ssize_t had_send_bytes;
        char send_buffer[4096] = {0};
        while(send_cursor < send_bytes_count) {
            need_send_bytes = fread(send_buffer,1,sizeof(send_buffer),fd);
            if((had_send_bytes = sendto(sock,send_buffer,static_cast<int>(need_send_bytes),0,(sockaddr*)&dest_addr,sizeof(dest_addr))) < 0) {
                if(errno == EAGAIN || errno == EWOULDBLOCK) {
                    continue;
                }
                MOLE_ERROR(io_socket_channel,strerror(errno));
                fclose(fd);
                fd = nullptr;
                return false;
            }
            send_cursor += had_send_bytes;
        }
        fclose(fd);
        fd = nullptr;
        is_new = true;
        return true;
#endif
    }

    bool UdpSocket::RecvFile(const std::string &file_path, size_t file_size) {
#ifdef __linux__
        if(is_new){
            recv_cursor = 0;
            recv_bytes_count = file_size;
            is_new = false;
            fd = open(file_path.c_str(),O_CREAT | O_WRONLY,0755);
        }
        ssize_t had_recv_bytes;
        size_t need_recv_bytes;
        char recv_buffer[4096] = {0};
        while(recv_cursor < recv_bytes_count){
            bzero(recv_buffer,sizeof(recv_buffer));
            need_recv_bytes = (recv_bytes_count - recv_cursor) > sizeof(recv_buffer) ? sizeof(recv_buffer) : (recv_bytes_count - recv_cursor);
            if((had_recv_bytes = ::recvfrom(sock,recv_buffer,need_recv_bytes,0,(sockaddr*)&from_addr,(socklen_t*)&from_addr_size)) < 0){
                if(errno == EAGAIN || errno == EWOULDBLOCK){
                    continue;
                }
                close(fd);
                fd = -1;
                return false;
            }
            write(fd,recv_buffer,had_recv_bytes);
            recv_cursor += had_recv_bytes;
        }
        is_new = true;
        close(fd);
        fd = -1;
        return true;
#elif _WIN32
        if(is_new) {
            fd = fopen(file_path.c_str(),"wb");
            if(fd == nullptr) {
                MOLE_ERROR(io_socket_channel,strerror(errno));
                return false;
            }
            recv_bytes_count = file_size;
            recv_cursor = 0;
            is_new = false;
        }
        ssize_t had_recv_bytes;
        char recv_buffer[4096] = {0};
        while(recv_cursor < recv_bytes_count) {
            if((had_recv_bytes = recvfrom(sock,recv_buffer,sizeof(recv_buffer),0,(sockaddr*)&from_addr,&from_addr_size)) < 0 ) {
                if(errno == EAGAIN || errno == EWOULDBLOCK) {
                    continue;
                }
                MOLE_ERROR(io_socket_channel,strerror(errno));
                fclose(fd);
                fd = nullptr;
                return false;
            }
            fwrite(recv_buffer,had_recv_bytes,1,fd);
            recv_cursor += had_recv_bytes;
        }
        fclose(fd);
        fd = nullptr;
        is_new = true;
        return true;
#endif
    }

    ssize_t UdpSocket::SendTo(const std::string &ip, unsigned short port, const char *data, size_t size) {
        dest_addr.sin_family = AF_INET;
        dest_addr.sin_port = htons(port);
        dest_addr.sin_addr.s_addr = inet_addr(ip.c_str());
        return Send(data,size);
    }

    bool UdpSocket::SendFileTo(const std::string &ip, unsigned short port, const std::string &file_path) {
        dest_addr.sin_family = AF_INET;
        dest_addr.sin_port = htons(port);
        dest_addr.sin_addr.s_addr = inet_addr(ip.c_str());
        return SendFile(file_path);
    }

    bool UdpSocket::Bind(const std::string& ip, unsigned short port) {
        self_addr.sin_family = AF_INET;
        self_addr.sin_addr.s_addr = inet_addr(ip.c_str());
        self_addr.sin_port = htons(port);

        return bind(sock,(sockaddr*)&self_addr,sizeof(self_addr)) >= 0;
    }

    long UdpSocket::Send(const std::string &data) {
        return Socket::Send(data);
    }

    long UdpSocket::Send(std::string &data) {
        return Socket::Send(data);
    }

    long UdpSocket::SendTo(const std::string &ip, unsigned short port, const std::string &data) {
        dest_addr.sin_family = AF_INET;
        dest_addr.sin_port = htons(port);
        dest_addr.sin_addr.s_addr = inet_addr(ip.c_str());
        return Send(data);
    }

    long UdpSocket::SendTo(const std::string &ip, unsigned short port, std::string &data) {
        dest_addr.sin_family = AF_INET;
        dest_addr.sin_port = htons(port);
        dest_addr.sin_addr.s_addr = inet_addr(ip.c_str());
        return Send(data);
    }

    sockaddr_in UdpSocket::FromAddr() const {
        return from_addr;
    }
} // hzd