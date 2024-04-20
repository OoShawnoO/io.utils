/**
  ******************************************************************************
  * @file           : FileSystem.h
  * @author         : huzhida
  * @brief          : None
  * @date           : 2024/4/18
  ******************************************************************************
  */

#ifndef IO_UTILS_FILESYSTEM_H
#define IO_UTILS_FILESYSTEM_H

#include <string>
#include <vector>

namespace hzd {

    namespace filesystem {

        /**
         * 判断是否存在文件或目录
         * @return true表示存在,false表示不存在 & true for exist,false for not exist
         */
        bool exists(const std::string &path);
        /**
         * 是否为文件 & whether file or not
         * @return true表示是文件,false表示不是文件 & true for file type,false for not or not exist
         */
        bool is_file(const std::string &path);

        /**
         * 是否为目录 & whether directory or not
         * @return true表示是目录,false表示不是目录 & true for directory type,false for not or not exist
         */
        bool is_directory(const std::string &path);

        /**
         * 复制源文件到目标路径下 & copy src file to destination path
         * @param src_path 源路径 & source path
         * @param dest_path 目标路径 & destination path
         * @return true表示成功,false表示失败 & true for success,false for failed
         */
        bool copy(const std::string &src_path, const std::string &dest_path, bool is_overwrite = false);

        /**
         * 获取当前工作目录的绝对路径 & get current work dir absolute path
         * @return 当前工作目录的绝对路径 & current work dir absolute path
         */
        std::string pwd();

        /**
         * 创建目录 & create directory          \n
         * 若中间目录不存在，将迭代创建直至目的目录 & If the intermediate directory does not exist, it will be created iteratively until the destination directory is reached
         * @return true表示成功,false表示失败 & true for success,false for failed
         */
        bool createdir(const std::string &path);

        /**
         *  文件大小 & file size
         *  @return 文件大小,-1表示文件不存在 & file size,-1 for file not exist
         */
        long long int fsize(const std::string &path);

        /**
         * 删除文件或目录 & delete file or directory
         * @return true表示成功,false表示失败 & true for success,false for failed
         */
        bool remove(const std::string &path);

        /**
         * 移动文件或重命名 & move file or directory,or rename
         * @return true表示成功,false表示失败 & true for success,false for failed
         */
        bool move(const std::string &src_path, const std::string &dest_path);

        /**
         * 重命名文件 & rename file
         * @return true表示成功,false表示失败 & true for success,false for failed
         */
        bool rename(const std::string& old_name,const std::string& new_name);

        /**
         * 列表目录下文件或目录名 & list files or dirs in path
         * @return true表示成功,false表示失败 & true for success,false for failed
         */
        bool listdir(const std::string& path,std::vector<std::string>& dirs_name,std::vector<std::string>& files_name);

        /**
         * 获取文件或目录的绝对路径 & get absolute path for file or dir
         * @return true表示成功,false表示失败 & true for success,false for failed
         */
        bool absolute(const std::string& path,std::string& absolute_path);
    }

} // hzd

#endif //IO_UTILS_FILESYSTEM_H
