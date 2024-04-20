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
         * �ж��Ƿ�����ļ���Ŀ¼
         * @return true��ʾ����,false��ʾ������ & true for exist,false for not exist
         */
        bool exists(const std::string &path);
        /**
         * �Ƿ�Ϊ�ļ� & whether file or not
         * @return true��ʾ���ļ�,false��ʾ�����ļ� & true for file type,false for not or not exist
         */
        bool is_file(const std::string &path);

        /**
         * �Ƿ�ΪĿ¼ & whether directory or not
         * @return true��ʾ��Ŀ¼,false��ʾ����Ŀ¼ & true for directory type,false for not or not exist
         */
        bool is_directory(const std::string &path);

        /**
         * ����Դ�ļ���Ŀ��·���� & copy src file to destination path
         * @param src_path Դ·�� & source path
         * @param dest_path Ŀ��·�� & destination path
         * @return true��ʾ�ɹ�,false��ʾʧ�� & true for success,false for failed
         */
        bool copy(const std::string &src_path, const std::string &dest_path, bool is_overwrite = false);

        /**
         * ��ȡ��ǰ����Ŀ¼�ľ���·�� & get current work dir absolute path
         * @return ��ǰ����Ŀ¼�ľ���·�� & current work dir absolute path
         */
        std::string pwd();

        /**
         * ����Ŀ¼ & create directory          \n
         * ���м�Ŀ¼�����ڣ�����������ֱ��Ŀ��Ŀ¼ & If the intermediate directory does not exist, it will be created iteratively until the destination directory is reached
         * @return true��ʾ�ɹ�,false��ʾʧ�� & true for success,false for failed
         */
        bool createdir(const std::string &path);

        /**
         *  �ļ���С & file size
         *  @return �ļ���С,-1��ʾ�ļ������� & file size,-1 for file not exist
         */
        long long int fsize(const std::string &path);

        /**
         * ɾ���ļ���Ŀ¼ & delete file or directory
         * @return true��ʾ�ɹ�,false��ʾʧ�� & true for success,false for failed
         */
        bool remove(const std::string &path);

        /**
         * �ƶ��ļ��������� & move file or directory,or rename
         * @return true��ʾ�ɹ�,false��ʾʧ�� & true for success,false for failed
         */
        bool move(const std::string &src_path, const std::string &dest_path);

        /**
         * �������ļ� & rename file
         * @return true��ʾ�ɹ�,false��ʾʧ�� & true for success,false for failed
         */
        bool rename(const std::string& old_name,const std::string& new_name);

        /**
         * �б�Ŀ¼���ļ���Ŀ¼�� & list files or dirs in path
         * @return true��ʾ�ɹ�,false��ʾʧ�� & true for success,false for failed
         */
        bool listdir(const std::string& path,std::vector<std::string>& dirs_name,std::vector<std::string>& files_name);

        /**
         * ��ȡ�ļ���Ŀ¼�ľ���·�� & get absolute path for file or dir
         * @return true��ʾ�ɹ�,false��ʾʧ�� & true for success,false for failed
         */
        bool absolute(const std::string& path,std::string& absolute_path);
    }

} // hzd

#endif //IO_UTILS_FILESYSTEM_H
