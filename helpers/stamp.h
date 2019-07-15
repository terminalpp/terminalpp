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

		std::string const& compiler() const {
			return compiler_;
		}

		std::string const& compilerVersion() const {
			return compilerVersion_;
		}

		std::string const& time() const {
			return time_;
		}

		/** Outputs the stamp as a C++ macro. 
		 */
		void outputCppMacro(std::ostream& s) const {
			s << "#define STAMP helpers::Stamp("
				<< '"' << version_ << "\", "
				<< '"' << commit_ << "\", "
				<< (clean_ ? "true," : "false,")
				<< '"' << compiler_ << "\", "
				<< '"' << compilerVersion_ << "\", "
				<< '"' << time_ << "\")";
		}

		Stamp(std::string const& version, std::string const& commit, bool clean, std::string const compiler, std::string const & compilerVersion, std::string const & time) :
			version_(version),
			commit_(commit),
			clean_(clean),
			compiler_(compiler),
			compilerVersion_(compilerVersion),
		    time_(time) {
		}

		static Stamp Stored() {
#ifdef STAMP
			return STAMP;
#else
			return Stamp("", "?", false, "?", "?", "?");
#endif
		}

		static Stamp FromGit(std::string const& path, std::string const & compiler, std::string const & compilerVersion) {
			GitRepo repo(path);
			std::string commit = repo.currentCommit();
			bool changed = repo.hasPendingChanges();
			std::string version;
			for (std::string const& tag : repo.currentTags()) {
				if (tag[0] == 'v' && tag.size() > 1 && IsDecimalDigit(tag[1])) {
					version = tag.substr(1);
					break;
				}
			}
			return Stamp(version, commit, !changed, compiler, compilerVersion, TimeInISO8601());
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
			s << " [" << stamp.compiler_ << " " << stamp.compilerVersion_ << "]";
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