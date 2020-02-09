#pragma once

#include "helpers.h"
#include "git.h"
#include "time.h"

namespace helpers {

	class Stamp {
	public:

		std::string const & version() const {
			return version_;
		}

		std::string const& commit() const {
			return commit_;
		}

		bool clean() const {
			return clean_;
		}

		std::string const& time() const {
			return time_;
		}

		std::string buildType() const {
#if (defined NDEBUG)
			return "release";
#else
			return "debug";
#endif
		}

		/** Outputs the stamp as a C++ macro. 
		 */
		void outputCppMacro(std::ostream& s) const {
			s << "#define STAMP helpers::Stamp("
				<< '"' << version_ << "\", "
				<< '"' << commit_ << "\", "
				<< (clean_ ? "true," : "false,")
				<< '"' << time_ << "\")";
		}

		Stamp(std::string const& version, std::string const& commit, bool clean, std::string const & time) :
			version_(version),
			commit_(commit),
			clean_(clean),
		    time_(time) {
		}

		static Stamp Stored() {
#ifdef STAMP
			return STAMP;
#else
			return Stamp("", "?", false, "?");
#endif
		}

		static Stamp FromGit(std::string const& path) {
			GitRepo repo(path);
			std::string commit;
			bool changed;
			std::string version;
			commit = repo.currentCommit();
			changed = repo.hasPendingChanges();
			// if project version is available as macro, use that, otherwise check if the commit is tagged in git 
#if (defined PROJECT_VERSION)
			version = PROJECT_VERSION;
#else
			for (std::string const& tag : repo.currentTags()) {
				if (tag[0] == 'v' && tag.size() > 1 && IsDecimalDigit(tag[1])) {
					version = tag.substr(1);
					break;
				}
			}
#endif
			return Stamp(version, commit, !changed, TimeInISO8601());
		}

	private:

		friend std::ostream& operator << (std::ostream& s, Stamp const& stamp) {
			s << "v";
			if (!stamp.version_.empty())
				s << stamp.version_;
			else
				s << "?";
			if (! stamp.commit_.empty() && (stamp.version_.empty() || !stamp.clean_))
				s << "-" << stamp.commit_;
			if (!stamp.clean_)
				s << "*";
#ifdef NDEBUG
			s << " release";
#else
			s << " debug";
#endif
			s << " [" << ARCH << " " << ARCH_SIZE << "bit, " << ARCH_COMPILER << " " << ARCH_COMPILER_VERSION << "]";
			s << " " << stamp.time_;
			return s;
		}


		std::string version_;
		std::string commit_;
		bool clean_;
		std::string compiler_;
		std::string compilerVersion_;
		std::string time_;


	};





} // namespace helpers