#pragma once
#if (defined ARCH_WINDOWS)
#include <windows.h>
#include <fileapi.h>
#include <ShlObj_core.h>
#else
#include <limits.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <pwd.h>
#endif

#include <fstream>
#include <filesystem>
#include <algorithm>

#include "helpers.h"
#include "string.h"

/** Filesystem functions and helpful classes

    Adds extra filesystem routines such as creation of unique folders, temporary folder, etc. and wraps around the std::filesystem with UTF8 strings instead of the platform specific paths, which are used under the hood. 
 
    To include thsi header, the the `stdc++fs` library must be included, such as typing the following in CMakeLists:

    target_link_libraries(PROJECT stdc++fs)    

 */

HELPERS_NAMESPACE_BEGIN

    /** Reads the entire file. 
     
        Note that the file should be modestly sized as it is returned as a single string
     */
    inline std::string ReadEntireFile(std::string const & filename) {
        std::ifstream f(filename, std::ios::in | std::ios::binary | std::ios::ate);
        OSCHECK(f.good()) << "Unable to open file " << filename;
        size_t size = f.tellg();
        f.seekg(0, std::ios::beg);
        std::vector<char> result(size);
        f.read(&result[0], size);
        return std::string(&result[0], size);
    }

    /** Returns the hostname of the current computer. 
     */
    inline std::string GetHostname() {
#if (defined ARCH_WINDOWS)
        TCHAR buffer[MAX_COMPUTERNAME_LENGTH + 1];
        DWORD bufSize = MAX_COMPUTERNAME_LENGTH + 1;
        OSCHECK(GetComputerName(buffer, &bufSize));
        buffer[bufSize] = 0;
        return UTF16toUTF8(buffer);
#elif (defined ARCH_LINUX)
        char buffer[HOST_NAME_MAX];
        gethostname(buffer, HOST_NAME_MAX);
        return std::string(buffer);
#else // maxOS, BSD
        char buffer[_POSIX_HOST_NAME_MAX];
        gethostname(buffer, _POSIX_HOST_NAME_MAX);
        return std::string(buffer);
#endif
    }

    inline std::string GetFilename(std::string const & path) {
#if (defined ARCH_WINDOWS)
        std::filesystem::path p(UTF8toUTF16(path));
        return p.filename().u8string();
#else
        std::filesystem::path p(path);
        return p.filename();
#endif
    }

    inline std::string JoinPath(std::string const & first, std::string const & second) {
#if (defined ARCH_WINDOWS)
        std::filesystem::path p(UTF8toUTF16(first));
        p.append(UTF8toUTF16(second));
        return p.u8string();
#else
        std::filesystem::path p(first);
        p.append(second);
        return p.u8string();
#endif
    }

    inline std::string JoinPath(std::initializer_list<std::string> elements) {
        if (elements.size() == 0)
            return "";
        auto i = elements.begin(), e = elements.end();
#if (defined ARCH_WINDOWS)
        std::filesystem::path p{UTF8toUTF16(*i)};
        while (++i != e)
            p.append(UTF8toUTF16(*i));
#else
        std::filesystem::path p{*i};
        while (++i != e)
            p.append(*i);
#endif
        return p.u8string();
    }

    inline bool PathExists(std::string const path) {
#if (defined ARCH_WINDOWS)
        std::filesystem::path p{UTF8toUTF16(path)};
#else
        std::filesystem::path p{path};
#endif
        return std::filesystem::exists(p);
    }

    /** Creates the given path. 
     
        Returns true if the path had to be created, false if it already exists.
     */
    inline bool CreatePath(std::string const & path) {
        return std::filesystem::create_directories(path);
    }

    /** Copies given file or folder. 
     */
    inline void Copy(std::string const & from, std::string const & to) {
    #if (defined ARCH_WINDOWS)
        std::filesystem::path f{UTF8toUTF16(from)};
        std::filesystem::path t{UTF8toUTF16(to)};
    #else
        std::filesystem::path f{from};
        std::filesystem::path t{to};
    #endif
        std::filesystem::copy(from, to);
    }

    /** Renames given file or folder. 
     */
    inline void Rename(std::string const & from, std::string const & to) {
    #if (defined ARCH_WINDOWS)
        std::filesystem::path f{UTF8toUTF16(from)};
        std::filesystem::path t{UTF8toUTF16(to)};
    #else
        std::filesystem::path f{from};
        std::filesystem::path t{to};
    #endif
        std::filesystem::rename(from, to);
    }

    /** Returns the directory in which local application settings should be stored on given platform. 
     */
    inline std::string LocalSettingsFolder() {
#if (defined ARCH_WINDOWS)
		wchar_t * wpath;
		OSCHECK(SHGetKnownFolderPath(
			FOLDERID_RoamingAppData,
			0, 
			nullptr,
			& wpath
		) == S_OK) << "Unable to determine stetings folder location";
		std::string path(HELPERS_NAMESPACE_DECL::UTF16toUTF8(wpath));
		CoTaskMemFree(wpath);
        return path;
#elif (defined ARCH_MACOS)
        std::string path(getpwuid(getuid())->pw_dir);
        return path + "/Library/Application Support";
#else
        std::string path(getpwuid(getuid())->pw_dir);
        return path + "/.config";
#endif
    }

    /** Returns the home directory for the current user. 
     */
    inline std::string HomeDir() {
#if (defined ARCH_WINDOWS)
        return STR(getenv("HOMEDRIVE") << getenv("HOMEDIR"));
#else
        return getenv("HOME");
#endif
    }

    /** Returns the directory where temporary files should be stored. 
     
        This usually goes to `%APP_DATA%/local/Temp` on Windows and `/tmp` on Linux and friends. 
     */
    inline std::string TempDir() {
#if (defined ARCH_WINDOWS)
        return UTF16toUTF8(std::filesystem::temp_directory_path().c_str());
#else
        return std::filesystem::temp_directory_path().c_str();
#endif
    }

    /** Splits the given filename to file name and extension. 
     
        If the argument is a complete path, only the filename and the extension are returned. 
     */
    inline std::pair<std::string, std::string> SplitFilenameExt(std::filesystem::path const & path) {
        std::string ext = path.extension().string();
        std::string filename = path.filename().string();
        filename = filename.substr(0, filename.size() - ext.size());
        return std::make_pair(filename, ext);
    }

    /** Given an existing folder, creates a new path that is guaranteed not to exist in the folder. 
     
        The path is a string with given prefix followed by the specified number of alphanumeric characters. Once created, the path is checked for existence and if not found is returned.
     */
    inline std::string UniqueNameIn(std::filesystem::path const & path, std::string const & prefix, std::string suffix = "", size_t length = 16) {
        while (true) {
            std::string x{CreateRandomAlphanumericString(length)};
            std::string filename = prefix + CreateRandomAlphanumericString(length) + suffix;
            if (! std::filesystem::exists(path / filename))
                return filename;
        }
    }
    /*
    inline std::string UniqueNameIn(std::string const & folder, std::string const & prefix, std::string suffix = "", size_t length = 16) {
        while (true) {
            std::filesystem::path p{folder};
            std::string x{CreateRandomAlphanumericString(length)};
            p.append(prefix + x + suffix);
            if (! std::filesystem::exists(p))
#if (defined ARCH_WINDOWS)
                return UTF16toUTF8(p.c_str());
#else
                return p.c_str();
#endif
        }
    }
    */

    /** Makes the given filename unique by attaching random alphanumeric string of given size separated by the given separator. 
     */
    inline std::string MakeUnique(std::string const & path, std::string const & separator = ".", size_t length = 16) {
        while (true) {
            std::filesystem::path p{path + separator + CreateRandomAlphanumericString(length)};
            if (! std::filesystem::exists(p))
#if (defined ARCH_WINDOWS)
                return UTF16toUTF8(p.c_str());
#else
                return p.c_str();
#endif
        }
    }

    /** Makes sure that the given folder contains no more than maxFiles.
     
        Erases the oldest files if the number of files exceeds the given number.
     */ 
    inline void EraseOldestFiles(std::string const & folder, size_t maxFiles) {
        typedef std::pair<long long, std::filesystem::path> FileDesc;
        std::vector<FileDesc> files;
        for(auto& p: std::filesystem::directory_iterator(folder)) {
            if (p.is_regular_file())
                files.push_back(std::make_pair(p.last_write_time().time_since_epoch().count(), p.path()));
        }
        size_t numFiles = files.size();
        if (numFiles > maxFiles) {
            std::sort(files.begin(), files.end(), [](FileDesc const & a, FileDesc const & b) {
                return a.first < b.first;
            });
            for (FileDesc const & f : files) {
                try {
                    std::filesystem::remove(f.second);
                    --numFiles;
                } catch (...) {
                    // don't do anything, just don't delete the file
                }
                if (numFiles <= maxFiles)
                    break;
            }
        }
    }
#ifdef HAHA
    /** Temporary folder with optional cleanup.

        Creates a temporary folder in the appropriate location for given platform. The folder may start with a selected prefix, which is not necessary but may help a human orient in the temp directories. The temporary folder and its contents are deleted when the object is destroyed, unless the `deleteWhenDestroyed` argument is set to false, in which case the folder and its files are never deleted by the object and their deletion is either up to the user, or more likely to the operating system. 
     */
    class TemporaryFolder {
    public:

        /** Creates the temporary folder of given prefix and persistence. 
         */
        TemporaryFolder(std::string prefix = "", bool deleteWhenDestroyed = true):
            path_(UniqueNameIn(TempDir(), prefix)),
            deleteWhenDestroyed_(deleteWhenDestroyed) {
            std::filesystem::create_directory(path_);
        }

        TemporaryFolder(TemporaryFolder && from):
            path_(from.path_),
            deleteWhenDestroyed_(from.deleteWhenDestroyed_) {
            from.deleteWhenDestroyed_ = false;
        }

        TemporaryFolder(TemporaryFolder const &) = delete;

        /** If not persistent, deletes the temporary folder and its contents. 
         */
        ~TemporaryFolder() {
            if (deleteWhenDestroyed_)
                std::filesystem::remove_all(path_);
        }

        TemporaryFolder & operator = (TemporaryFolder && from) {
            if (& from != this) {
                if (deleteWhenDestroyed_)
                    std::filesystem::remove_all(path_);
                path_ = from.path_;
                deleteWhenDestroyed_ = from.deleteWhenDestroyed_;
                from.deleteWhenDestroyed_ = false;
            }
            return *this;
        }

        TemporaryFolder & operator = (TemporaryFolder const &) = delete;

        /** Returns the path of the temporary object. 
         */
        std::string const & path() const {
            return path_;
        }

    private:
        std::string path_;
        bool deleteWhenDestroyed_;

    };
#endif

HELPERS_NAMESPACE_END