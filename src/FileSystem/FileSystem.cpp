/**
  ******************************************************************************
  * @file           : FileSystem.cpp
  * @author         : huzhida
  * @brief          : None
  * @date           : 2024/4/18
  ******************************************************************************
  */

#include "FileSystem.h"
#include <fstream>

#ifdef __linux__
#include <sys/stat.h>
#include <unistd.h>
#elif _WIN32
#include <direct.h>
#endif


namespace hzd {
    namespace filesystem {
        bool _exists(const std::string &path,struct stat& st) {
            return stat(path.c_str(),&st) == 0;
        }

        bool exists(const std::string &path) {
            struct stat st{};
            return _exists(path,st);
        }

        bool is_file(const std::string &path) {
            struct stat st{};
            if(!_exists(path,st)) {
                fserr = FileSystemNotExist;
                return false;
            }
            return st.st_mode & S_IFREG;
        }

        bool is_directory(const std::string &path) {
            struct stat st{};
            if(!_exists(path,st)) {
                fserr = FileSystemNotExist;
                return false;
            }
            return st.st_mode & S_IFDIR;
        }

        bool copy(const std::string &src_path, const std::string &dest_path,bool is_overwrite) {
            struct stat src_st{},dest_st{};
            if(!_exists(src_path,src_st)) {
                fserr = FileSystemNotExist;
                return false;
            }

            if(src_st.st_mode & S_IFDIR) {
                fserr = NotFile;
                return false;
            }

            std::string processed_src_path = src_path;
            for(auto& c : processed_src_path) {
                if(c == '\\') c = '/';
            }
            auto sub_start = processed_src_path.find_last_of('/');
            if(sub_start == std::string::npos) sub_start = 0;
            std::string src_file_name = processed_src_path.substr(sub_start);

            std::ifstream src(src_path);
            if(!_exists(dest_path,dest_st)) {
                std::ofstream dest(dest_path);
                dest << src.rdbuf();
                return true;
            }else{
                if(dest_st.st_mode & S_IFDIR) {
                    std::ofstream dest(dest_path + "/" + src_file_name);
                    dest << src.rdbuf();
                    return true;
                }
                else {
                    if(!is_overwrite) {
                        fserr = AlreadyExist;
                        return false;
                    }
                    std::ofstream dest(dest_path,std::ios::out | std::ios::trunc);
                    dest << src.rdbuf();
                    return true;
                }
            }
        }

        std::string pwd() {
            char buffer[4096] = {0};
#ifdef __linux__
            getcwd(buffer,sizeof(buffer));
#elif  _WIN32
            _getcwd(buffer,sizeof(buffer));
#endif
            return buffer;
        }

        bool createdir(const std::string &path) {
#ifdef _WIN32
            return mkdir(path.c_str()) == 0;
#elif __linux__
            return mkdir(path.c_str(),0755) == 0;
#endif
        }

        long long int fsize(const std::string &path) {
            struct stat st{};
            if(!_exists(path,st)) {
                fserr = FileSystemNotExist;
                return -1;
            }
            return st.st_size;
        }

        bool rm(const std::string &path) {
            return remove(path.c_str()) == 0;
        }

        bool mv(const std::string &src_path, const std::string &dest_path) {
            return false;
        }

    }
} // hzd