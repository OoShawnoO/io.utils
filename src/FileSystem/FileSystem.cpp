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


#ifdef __linux__
#include <sys/stat.h>
#include <unistd.h>
#include <cstring>
#include <dirent.h>
#elif _WIN32

#ifdef _M_AMD64
#define _AMD64_
#elif _M_IX86
#define _X86_
#elif _M_ARM64
#define _ARM64EC_
#elif _M_ARM
#define _ARM_
#else
#endif
#include <windef.h>
#include <direct.h>
#include <io.h>
#include <fileapi.h>
#include <errhandlingapi.h>
#include <WinBase.h>

namespace hzd {
    std::string GetLastError_() noexcept {
        LPSTR lpBuffer = nullptr;
        DWORD dwFlags = FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS;
        if(FormatMessageA(dwFlags,nullptr,GetLastError(),0,(LPSTR)&lpBuffer,0,nullptr)) {
            std::string err = lpBuffer;
            LocalFree(lpBuffer);
            return err;
        }
        return "ErrorCode:" + std::to_string(GetLastError());
    }
}

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
            if(mkdir(path.c_str()) != 0) {
                MOLE_ERROR(io.FileSystem,strerror(errno));
                return false;
            }
            return true;
#elif __linux__
            if(mkdir(path.c_str(),0755) != 0) {
                MOLE_ERROR(io.FileSystem,strerror(errno));
                return false;
            }
            return true;
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
            struct stat st{};
            if(!_exists(path,st)) {
                MOLE_ERROR(io.FileSystem,"file or dir not exist",{MOLE_VAR(path)});
                return false;
            }
            if(st.st_mode & S_IFDIR) {
#ifdef __linux__
                return rmdir(path.c_str()) == 0;
#elif _WIN32
                return _rmdir(path.c_str()) == 0;
#endif
            }
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
#ifdef __linux__
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
#elif _WIN32
            _finddata_t file_data;
            intptr_t handle;
            handle = _findfirst((path + "\\*").c_str(),&file_data);
            if(handle == -1L) {
                MOLE_ERROR(io.FileSystem,"can't match path",{ MOLE_VAR(path )});
                return false;
            }
            do {

                if(file_data.attrib & _A_SUBDIR) {
                    if(strcmp(file_data.name,".") == 0 || strcmp(file_data.name,"..") == 0) continue;
                    dirs_name.emplace_back(file_data.name);
                }else {
                    files_name.emplace_back(file_data.name);
                }

            }while(_findnext(handle,&file_data) == 0);
            return true;
#endif
        }

        bool absolute(const std::string& path,std::string& absolute_path) {
            if(!exists(path)) {
                MOLE_ERROR(io.FileSystem,"no such file or dir");
                return false;
            }
#ifdef __linux__
            char result[4096] = {0};
            if(!realpath(path.c_str(),result)) {
                MOLE_ERROR(io.FileSystem,strerror(errno));
                return false;
            }
#elif _WIN32
            char result[4096] = {0};
            auto ret = GetFullPathName(path.c_str(),sizeof(result),result,nullptr);
            if(ret == 0) {
                MOLE_ERROR(io.FileSystem,GetLastError_());
                return false;
            }
#endif
            absolute_path = result;
            return true;
        }
    }
} // hzd