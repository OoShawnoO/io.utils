/**
  ******************************************************************************
  * @file           : FileSystem.cpp
  * @author         : huzhida
  * @brief          : None
  * @date           : 2024/4/18
  ******************************************************************************
  */
#include <Mole.h>
#include "FileSystem.h"
#include <fstream>
#include <dirent.h>

#ifdef __linux__
#include <sys/stat.h>
#include <unistd.h>
#include <cstring>

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
                MOLE_ERROR(io.FileSystem,"file not exist",{ MOLE_VAR(path) });
                return false;
            }
            return st.st_mode & S_IFREG;
        }

        bool is_directory(const std::string &path) {
            struct stat st{};
            if(!_exists(path,st)) {
                MOLE_ERROR(io.FileSystem,"dir not exist",{ MOLE_VAR(path) });
                return false;
            }
            return st.st_mode & S_IFDIR;
        }

        bool copy(const std::string &src_path, const std::string &dest_path,bool is_overwrite) {
            struct stat src_st{},dest_st{};
            if(!_exists(src_path,src_st)) {
                MOLE_ERROR(io.FileSystem,"source file not exist",{ MOLE_VAR(src_path) });
                return false;
            }

            if(src_st.st_mode & S_IFDIR) {
                MOLE_ERROR(io.FileSystem,"source path is dir,not file",{ MOLE_VAR(src_path)});
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
                if(!dest.is_open()) {
                    MOLE_ERROR(io.FileSystem,"destination can't access.",{MOLE_VAR(dest_path)});
                    return false;
                }
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
                        MOLE_ERROR(io.FileSystem,"destination file already exist,maybe set is_overwrite = true?",{ MOLE_VAR(dest_path) });
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
                MOLE_ERROR(io.FileSystem,"file not exist",{ MOLE_VAR(path) });
                return -1;
            }
            if(st.st_mode & S_IFDIR) {
                MOLE_ERROR(io.FileSystem,"path to dir,not file",{ MOLE_VAR(path) });
                return -1;
            }
            return st.st_size;
        }

        bool remove(const std::string &path) {
            return ::remove(path.c_str()) == 0;
        }

        bool move(const std::string &src_path, const std::string &dest_path) {
            bool ret = copy(src_path,dest_path);
            if(!ret) return false;
            if(!remove(src_path)) {
                MOLE_ERROR(io.FileSystem,strerror(errno));
                return false;
            }
            return true;
        }

        bool rename(const std::string& old_name,const std::string& new_name) {
            if(::rename(old_name.c_str(),new_name.c_str()) == 0) return true;
            else {
                MOLE_ERROR(io.FileSystem,strerror(errno));
                return false;
            }
        }

        bool listdir(const std::string& path,std::vector<std::string>& dirs_name,std::vector<std::string>& files_name) {
            DIR* dir = opendir(path.c_str());
            if(!dir) {
                MOLE_ERROR(io.FileSystem,strerror(errno));
                return false;
            }
            struct dirent* entry;
            while((entry = readdir(dir))) {
                if(entry->d_type == DT_DIR) {
                    if(strcmp(".",entry->d_name) == 0 || strcmp("..",entry->d_name) == 0) continue;
                    dirs_name.emplace_back(entry->d_name);
                }else{
                    files_name.emplace_back(entry->d_name);
                }
            }
            closedir(dir);
            return true;
        }

        bool absolute(const std::string& path,std::string& absolute_path) {
            char result[4096] = {0};
            if(!realpath(path.c_str(),result)) {
                MOLE_ERROR(io.FileSystem,strerror(errno));
                return false;
            }
            absolute_path = result;
            return true;
        }
    }
} // hzd