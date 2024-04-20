/**
  ******************************************************************************
  * @file           : main.cpp
  * @author         : huzhida
  * @brief          : None
  * @date           : 2024/4/17
  ******************************************************************************
  */

#include "../src/Socket/Socket.h"
#include "../src/FileSystem/FileSystem.h"
#include <Mole.h>
#include <gtest/gtest.h>
#include <thread>

#ifdef __linux__
#define __sleep(x) usleep(1000*x)
#elif _WIN32
#define __sleep(x) _sleep(x)
#endif

TEST(TEST_TCP,LISTEN) {
    hzd::TcpListener listener("127.0.0.1",9999);
    ASSERT_EQ(listener.Bind(),true);
    ASSERT_EQ(listener.Listen(),true);
    listener.Close();
}

TEST(TEST_TCP,CONNECT) {
    hzd::TcpListener listener("127.0.0.1",9999);
    listener.Bind();
    listener.Listen();
    hzd::TcpSocket tcp;

    std::thread t([] {
        hzd::TcpClient client;
        __sleep(1);
        ASSERT_EQ(client.Connect("127.0.0.1",9999),true);
        __sleep(1);
        client.Close();
    });

    ASSERT_EQ(listener.Accept(tcp),true);
    t.join();

    tcp.Close();
    listener.Close();
}

TEST(TEST_TCP,SEND_RECV) {
    hzd::TcpListener listener("127.0.0.1",9999);
    ASSERT_EQ(listener.Bind(),true);
    ASSERT_EQ(listener.Listen(),true);
    hzd::TcpSocket tcp;
    bool is_end = false;
    std::thread t([&] {
        hzd::TcpClient client;
        __sleep(1);
        ASSERT_EQ(client.Connect("127.0.0.1",9999),true);
        client.Send("123456");
        while(!is_end) { }
    });

    ASSERT_EQ(listener.Accept(tcp),true);
    std::string str;
    tcp.Recv(str,6,false);
    ASSERT_EQ(str,"123456");
    is_end = true;
    t.join();
}

TEST(TEST_TCP,SEND_RECV_FILE) {
    hzd::TcpListener listener("127.0.0.1",9999);
    ASSERT_EQ(listener.Bind(),true);
    ASSERT_EQ(listener.Listen(),true);
    hzd::TcpSocket tcp;
    bool is_end = false;

    auto fp = fopen("../test/main.cpp","rb");
    ASSERT_NE(fp,nullptr);
    struct stat stat{};
    fstat(fileno(fp),&stat);

    std::thread t([&] {
        hzd::TcpClient client;
        __sleep(1);
        ASSERT_EQ(client.Connect("127.0.0.1",9999),true);
        __sleep(1);
        client.SendFile("../test/main.cpp");
        while(!is_end) { }
    });

    ASSERT_EQ(listener.Accept(tcp),true);
    std::string str;
    tcp.RecvFile("../test/temp_main.cpp",stat.st_size);
    auto fp_ = fopen("../test/temp_main.cpp","rb");
    ASSERT_NE(fp_,nullptr);
    struct stat stat_{};
    fstat(fileno(fp_),&stat_);
    ASSERT_EQ(stat.st_size,stat_.st_size);
    is_end = true;
    t.join();
    remove("../test/temp_main.cpp");
}

TEST(TEST_UDP,BIND) {
    hzd::UdpSocket socket;
    ASSERT_EQ(socket.Bind("127.0.0.1",9999),true);
}

TEST(TEST_UDP,SEND_RECV) {
    hzd::UdpSocket listener;
    ASSERT_EQ(listener.Bind("127.0.0.1",9999),true);
    bool is_end = false;
    std::thread t([&] {
        hzd::UdpSocket client;
        __sleep(1);
        ASSERT_EQ(client.SendTo("127.0.0.1",9999,"123456",6),6);
        while(!is_end) { }
    });

    std::string str;
    ASSERT_EQ(listener.Recv(str,6,false),6);
    ASSERT_EQ(str,"123456");
    is_end = true;
    t.join();
}

TEST(TEST_UDP,SEND_RECV_FILE) {
    hzd::UdpSocket listener;
    ASSERT_EQ(listener.Bind("127.0.0.1",9999),true);
    bool is_end = false;

    auto fp = fopen("../test/main.cpp","rb");
    ASSERT_NE(fp,nullptr);
    struct stat stat{};
    fstat(fileno(fp),&stat);

    std::thread t([&] {
        hzd::UdpSocket client;
        __sleep(1);
        client.SendFileTo("127.0.0.1",9999,"../test/main.cpp");
        while(!is_end) { }
    });

    listener.RecvFile("../test/temp_main.cpp",stat.st_size);
    auto fp_ = fopen("../test/temp_main.cpp","rb");
    ASSERT_NE(fp_,nullptr);
    struct stat stat_{};
    fstat(fileno(fp_),&stat_);
    ASSERT_EQ(stat.st_size,stat_.st_size);
    is_end = true;
    t.join();
    remove("../test/temp_main.cpp");
}

TEST(TEST_FILESYSTEM,COPY) {
    ASSERT_EQ(hzd::filesystem::copy("../test/main.cpp","../test/copy_main.cpp"),true);
    ASSERT_EQ(hzd::filesystem::exists("../test/copy_main.cpp"),true);
    ASSERT_EQ(hzd::filesystem::remove("../test/copy_main.cpp"),true);

    ASSERT_EQ(hzd::filesystem::copy("../test/main.cpp","copy_main.cpp"),true);
    ASSERT_EQ(hzd::filesystem::exists("copy_main.cpp"),true);
    ASSERT_EQ(hzd::filesystem::remove("copy_main.cpp"),true);

    ASSERT_EQ(hzd::filesystem::copy("../test/main.cpp","./"),true);
    ASSERT_EQ(hzd::filesystem::exists("./main.cpp"),true);
    ASSERT_EQ(hzd::filesystem::remove("./main.cpp"),true);

    ASSERT_EQ(hzd::filesystem::copy("../test/_.cpp","./"),false);
    ASSERT_EQ(hzd::filesystem::copy("../test/main.cpp","../test/1/1.cpp"),false);

}

TEST(TEST_FILESYSTEM,MOVE) {
    ASSERT_EQ(hzd::filesystem::move("../test/test_move","./"),true);
    ASSERT_EQ(hzd::filesystem::exists("../test/test_move"),false);
    ASSERT_EQ(hzd::filesystem::exists("./test_move"),true);
    ASSERT_EQ(hzd::filesystem::move("./test_move","../test/test_move"),true);
    ASSERT_EQ(hzd::filesystem::exists("../test/test_move"),true);
    ASSERT_EQ(hzd::filesystem::exists("./test_move"),false);

}

TEST(TEST_FILESYSTEM,LIST_DIR_AND_ABS) {
    std::vector<std::string> files,dirs;
    ASSERT_EQ(hzd::filesystem::listdir("./",dirs,files),true);
    std::string abs_;
    ASSERT_EQ(hzd::filesystem::absolute(files[0],abs_),true);
    ASSERT_EQ(hzd::filesystem::absolute("123.txt",abs_),false);
}

int main() {
    testing::InitGoogleTest();
    return RUN_ALL_TESTS();
}