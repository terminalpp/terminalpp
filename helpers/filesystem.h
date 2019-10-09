#pragma once
#if (defined ARCH_WINDOWS)
#include <fileapi.h>
#include <ShlObj_core.h>
#else
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <pwd.h>
#endif

#include <filesystem>

#include "helpers.h"
#include "string.h"

/** Filesystem functions and helpful classes

    Adds extra filesystem routines such as creation of unique folders, temporary folder, etc. and wraps around the std::filesystem with UTF8 strings instead of the platform specific paths, which are used under the hood. 
 
 */

namespace helpers {

    /** Returns the directory in which local application settings should be stored on given platform. 
     */
    inline std::string LocalSettingsDir() {
#if (defined ARCH_WINDOWS)
		wchar_t * wpath;
		OSCHECK(SHGetKnownFolderPath(
			FOLDERID_LocalAppData,
			0, 
			nullptr,
			& wpath
		) == S_OK) << "Unable to determine stetings folder location";
		std::string path(helpers::UTF16toUTF8(wpath));
		CoTaskMemFree(wpath);
        return path;
#else
        std::string path(getpwuid(getuid())->pw_dir);
        return path + "/.config";
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

    /** Given an existing folder, creates a new path that is guaranteed not to exist in the folder. 
     
        The path is a string with given prefix followed by the specified number of alphanumeric characters. Once created, the path is checked for existence and if not found is returned.
     */
    inline std::string UniqueNameIn(std::string const & folder, std::string const & prefix, size_t length = 16) {
        while (true) {
            std::filesystem::path p{folder};
            std::string x{CreateRandomAlphanumericString(length)};
            p.append(prefix + x);
            if (! std::filesystem::exists(p))
#if (defined ARCH_WINDOWS)
                return UTF16toUTF8(p.c_str());
#else
                return p.c_str();
#endif
        }
    }

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

} // namespace helpers