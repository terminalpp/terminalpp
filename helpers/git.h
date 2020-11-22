#pragma once

#include "helpers.h"
#include "process.h"
#include "string.h"

HELPERS_NAMESPACE_BEGIN

	/** A simple abstraction over a local git repository. 
	
	    The repository is identified by its path and is basically a wrapper over system calls to the `git` command line to retrieve various information. 
	 */
	class GitRepo {
	public:

		GitRepo(std::string const& path) :
			path_(path) {
		}

		std::string const& path() const {
			return path_;
		}

		/** Returns the SHA-1 hash of the current commit, i.e. head.  
		 */
		std::string currentCommit() const {
			try {
				std::string result = Trim(Exec(Command("git", { "rev-parse", "HEAD" }, path_)));
				ASSERT(result.size() == 40) << "Invalid SHA1 hash size";
				return result;
			} catch (Exception &) {
				return "Not a git repo";
			}
		}

		/** Checks if the repo has any uncommited changes or untracked files
		 */
		bool hasPendingChanges() const {
			try {
			    return ! Exec(Command("git", { "status", "--short" }, path_)).empty();
			} catch (Exception &) {
				// if it's not a git repo, then there is no way to determine if we have pending changes
				return false;
			}
		}

		/** Returns list of tags pointing to the current commit (head). 
		 */
		std::vector<std::string_view> currentTags() const {
			try {
				return Split(Trim(Exec(Command("git", { "tag", "--points-at", "HEAD" }, path_))), "\n");
			} catch (Exception &) {
				return std::vector<std::string_view>{};
			}
		}

	private:
		std::string path_;

	};

HELPERS_NAMESPACE_END