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

#include "helpers.h"
#include "string.h"

namespace helpers {

    inline std::string RemoveLastPathElement(std::string const & path) {
        size_t pos = path.find_last_of("\\/");
        if (pos == std::string::npos)
            return "";
        return path.substr(0, pos);
    }

    inline void EnsurePath(std::string const & path) {
        std::vector<std::string> stack;
        stack.push_back(path);
        while (!stack.empty()) {
#if (defined ARCH_WINDOWS)
            utf16_string p = UTF8toUTF16(stack.back());
            // if the directory was successfully created, or if it exists, remove it from the stack
            if (CreateDirectory(p.c_str(), nullptr) != 0 || GetLastError() == ERROR_ALREADY_EXISTS) {
#else
            if (mkdir(stack.back().c_str(), S_IRUSR | S_IWUSR | S_IXUSR) == 0 || errno == EEXIST) {
#endif
                stack.pop_back();
                continue;
            } else {
                std::string subPath(RemoveLastPathElement(stack.back()));
                if (subPath.empty())
                    OSCHECK(false) << "Unable to create directory " << path;
                stack.push_back(subPath);
            }
        }
    }

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

} // namespace helpers