#pragma once 

#include <unordered_set>

#include "terminal_window.h"

namespace tpp {

	class Session;

	class Application {
	public:

		/** Returns the application singleton. 
		 */
		template<typename T = Application>
		static T * Instance() {
			static_assert(std::is_base_of<Application, T>::value, "Must be child of Application");
#ifdef NDEBUG
			return reinterpret_cast<T*>(instance_);
#else
			return dynamic_cast<T*>(instance_);
#endif
		}

		static void MainLoop() {
			instance_->mainLoop();
		}

		virtual ~Application() {

		}

		virtual TerminalWindow* createTerminalWindow(Session * session, TerminalWindow::Properties const& properties, std::string const & name) = 0;

		virtual std::pair<unsigned, unsigned> terminalCellDimensions(unsigned fontSize) = 0;

		TerminalWindow::Properties const& defaultTerminalWindowProperties() const {
			return defaultTerminalWindowProperties_;
		}

		/** Returns the selection color. 
		 */
		vterm::Color selectionBackgroundColor() {
			return vterm::Color(0xc0, 0xc0, 0xff);
		}


	protected:


		virtual void start();

		virtual void mainLoop() = 0;

		virtual void sendFPSTimerMessage() = 0;

	
		Application();

		TerminalWindow::Properties defaultTerminalWindowProperties_;

		static Application* instance_;
	}; // tpp::Application

	/** Stores and retrieves font objects so that they do not have to be created each time they are needed.

		Templated by the actual font handle, which is platform dependent.
	 */
	template<typename T>
	class FontSpec {
	public:

		/** Return a font for given terminal font description and zoom.
		 */
		static FontSpec* GetOrCreate(vterm::Font const& font, unsigned height) {
			vterm::Font f = StripEffects(font);
			unsigned id = (height << 8) + f.raw();
			auto i = Fonts_.find(id);
			if (i == Fonts_.end())
				i = Fonts_.insert(std::make_pair(id, Create(font, height))).first;
			return i->second;
		}

		vterm::Font font() const {
			return font_;
		}

		T const& handle() const {
			return handle_;
		}

		unsigned widthPx() const {
			return widthPx_;
		}

		unsigned heightPx() const {
			return heightPx_;
		}

		/** Does nothing, but explicitly mentioned so that it can be specialized when necessary.
		 */
		~FontSpec() {
		}

	private:

		/** Strips effects that does not alter the font selection on the given platform.

			By default strips only the blinking attribute, implementations can override this to strip other font effects as well.
		 */
		static vterm::Font StripEffects(vterm::Font const& font) {
			vterm::Font result(font);
			result.setBlink(false);
			return result;
		}

		FontSpec(vterm::Font font, unsigned width, unsigned height, T const& handle) :
			font_(font),
			widthPx_(width),
			heightPx_(height),
			handle_(handle) {
		}

		vterm::Font font_;
		unsigned widthPx_;
		unsigned heightPx_;
		T handle_;

		/** This must be implemented in platform specific code.
		 */
		static FontSpec* Create(vterm::Font font, unsigned baseHeight);

		static std::unordered_map<unsigned, FontSpec*> Fonts_;
	};


	template<typename T>
	std::unordered_map<unsigned, FontSpec<T>*> FontSpec<T>::Fonts_;


} // namespace tpp
