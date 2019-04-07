#pragma once

#include <thread>
#include <atomic>
#include <mutex>
#include <string>
#include <list>

#include "helpers/object.h"
#include "helpers/shapes.h"

#include "color.h"
#include "font.h"
#include "char.h"
#include "key.h"



namespace vterm {

	/** Describes a single character cell of the terminal. 

	    Each cell may specify its own font and attributes, text and background color and a character to display. 
	 */
	class Cell {
	public:
		/** The font to be used to render the cell.
		 */
		Font font;

		/** Text color.
		 */
		Color fg;

		/** Background color.
		 */
		Color bg;

		/** Character to be displayed (utf8).
		 */
		Char::UTF8 c;

		/** Default constructor for a cell created white space on a black background.
		 */
		Cell() :
			fg(Color::White()),
			bg(Color::Black()),
			c(' ') {
		}
	}; // vterm::Cell

	/** Virtual Terminal Implementation. 

	    The terminal only implements the storage and programmatic manipulation of the terminal data and does not concern itself with either making the contents visible to the user (this is the job of terminal renderer), or by specifying ways on how & when the contents of the terminal should be updated, which is a job of classes that inherit from the virtual terminal (such as PTYTerminal). 

	    TODO for now the terminal class is rather small and its functionality limited, but in the future with support for layers, overlays, graphic objects, links, etc. its size would grow a lot. 
	 */
	class Terminal : public helpers::Object {
	public:
		class LayerImpl;

		/** Non-copyable pointer to terminal layer which also acts as a terminal data acccess guard.

			TODO add layer switching when multiple layers are supported in the terminal (and determine *how* they are supported.
		 */
		class Layer {
		public:

			/** The layer is above all a smart pointer to the layer implementation.
			 */
			LayerImpl * operator -> () {
				return layer_;
			}

			LayerImpl & operator * () {
				return *layer_;
			}

			LayerImpl const * operator -> () const {
				return layer_;
			}

			LayerImpl const & operator * () const {
				return * layer_;
			}

			/** Marks the given rectangle as changed, which will trigger the onPaint event on a rectangle encompassing when the layer is deleted. 
			 */
			void commitRect(helpers::Rect rect) {
				changed_ = helpers::Rect::Union(changed_, rect);
			}

			/** Marks the entire terminal area as changed, which will trigger repaint of everything when the layer is deleted. 
			 */
			void commitAll() {
				changed_ = helpers::Rect{ 0,0, terminal_->cols_, terminal_->rows_ };
			}

			/** To prevent data inconsistency, there could be only one instance of a layer pointer per terminal at any given time.
			 */
			Layer(Layer const &) = delete;

			/** Move constuctor creates a copy and then invalidates the original so that its deletion would not release the terminal lock.
			 */
			Layer(Layer && from) :
				terminal_(from.terminal_),
				layer_(from.layer_),
			    changed_(from.changed_) {
				from.terminal_ = nullptr;
			}

			/** When layer is deleted, it releases the lock on the terminal access and informs it to trigger the on change event. 
			 */
			~Layer() {
				if (terminal_ != nullptr) {
					terminal_->m_.unlock();
					terminal_->repaintRect(changed_);
				}
			}

		private:
			friend class Terminal;

			Layer(Terminal * terminal, LayerImpl * layer) :
				terminal_(terminal),
				layer_(layer),
				changed_{ 0,0,0,0 } {
				terminal_->m_.lock();
			}

			Terminal * terminal_;
			LayerImpl * layer_;
			helpers::Rect changed_;
		};

		// events ---------------------------------------------------------------------------------

		/** Size of the terminal (in cell columns and rows).
		 */
		struct Size {
			/** Number of columns. */
			unsigned cols;
			/** Number of rows. */
			unsigned rows;

			Size(unsigned cols, unsigned rows) :
				cols(cols),
				rows(rows) {
			}
		};

		typedef helpers::EventPayload<Size, helpers::Object> ResizeEvent;
		typedef helpers::EventPayload<helpers::Rect, helpers::Object> RepaintEvent;

		/** Triggered when the terminal is resized.
		 */
		helpers::Event<ResizeEvent> onResize;

		/** Triggered when the terminal contents change and should be repainted.
		 */
		helpers::Event<RepaintEvent> onRepaint;

		// properties -----------------------------------------------------------------------------

		/** Returns the size of the terminal.

			Separate access to rows and columns is not permitted because should the terminal size change in between the calls, inconsistent results would be returned.
		 */
		Size size() const {
			std::lock_guard<std::mutex> g(m_);
			return Size{ cols_, rows_ };
		}

		// methods --------------------------------------------------------------------------------

		virtual void keyDown(Key k) = 0;
		virtual void keyUp(Key k) = 0;

		/** Resizes the terminal to given size.
		 */
		bool resize(unsigned cols, unsigned rows) {
			{
				std::lock_guard<std::mutex> g(m_);
				// don't do anything if the size is the same
				if (cols_ == cols && rows_ == rows)
					return false;
				// perform the actual resize operation
				doResize(cols, rows);
			}
			// after the lock has been released, emit the onResize event
			trigger(onResize, Size{ cols, rows });
			return true;
		}

		/** Returns the contents of the default layer.

			The returned layer smart pointer locks the whole terminal for reads or updates of the screen and its size so the layer should be kept around for as little time as needed (typically to update its contents, or render it).
		 */
		Layer getDefaultLayer() {
			return Layer(this, defaultLayer_);
		}

		// constructors & destructors -------------------------------------------------------------

		/** Deletes the virtual terminal.
		 */
		~Terminal() override;

	protected:

		/** Creates the virtual terminal of given size.
		 */
		Terminal(unsigned cols, unsigned rows);

		void repaintRect(helpers::Rect rect) {
			ASSERT(helpers::Rect::Intersection(rect, helpers::Rect{ 0,0,cols_,rows_ }) == rect);
			if (!rect.empty())
				trigger(onRepaint, rect);
		}

		void repaintAll() {
			trigger(onRepaint, helpers::Rect{ 0,0,cols_, rows_ });
		}

		/** Actually resizes the terminal.

		    The called code may assume that the terminal is locked. 
		 */
		virtual void doResize(unsigned cols, unsigned rows);

		/** Mutex for protecting read and write access to the terminal contents to make sure that all requests will return consistent data.
		 */
		mutable std::mutex m_;

		/** Number of columns the terminal stores in each layer. */
		unsigned cols_;

		/** Number of rows the terminal stores in each layer. */
		unsigned rows_;

		/** The default layer.
		 */
		LayerImpl * defaultLayer_;

	private:


	}; // vterm::VTerm

	/** Implementation of a layer for the virtual terminal. 
	 */
	class Terminal::LayerImpl {
	public:
		unsigned cols() const {
			return terminal_->cols_;
		}

		unsigned rows() const {
			return terminal_->rows_;
		}

		Cell & at(unsigned col, unsigned row) {
			ASSERT(col < cols() && row < rows());
			return cells_[row * cols() + col];
		}

		Cell const & at(unsigned col, unsigned row) const {
			ASSERT(col < cols() && row < rows());
			return cells_[row * cols() + col];
		}

	private:
		friend class Terminal;

		LayerImpl(Terminal * terminal):
			terminal_(terminal),
			cells_(new Cell[terminal_->cols_ * terminal_->rows_]) {
		}

		~LayerImpl() {
			delete[] cells_;
		}

		/** Resizes the layer. 
		 */
		void resize(unsigned cols, unsigned rows) {
			ASSERT(terminal_->cols_ == cols && terminal_->rows_ == rows);
			delete[] cells_;
			cells_ = new Cell[cols * rows];
		}

		/** Terminal the layer belongs to. 
		 */
		Terminal * const terminal_;

		/** The actual cells in the layer.
		 */
		Cell * cells_;
	};

	/** Terminal extension to allow communication over input output streams. 
	 */
	class IOTerminal : public Terminal {
	public:
		static constexpr size_t DEFAULT_BUFFER_SIZE = 1024;

	protected:
		IOTerminal(unsigned cols, unsigned rows, size_t bufferSize = DEFAULT_BUFFER_SIZE) :
			Terminal(cols, rows),
			inputBuffer_(nullptr),
		    inputBufferSize_ (bufferSize) {
		}

		~IOTerminal() override;

		virtual void doStart();

		/** Called when new data has been received on the input stream. 
		 */
		virtual void processInputStream(char * buffer, size_t & size) = 0;

		/** Called by the reader thread when new data from the input stream should be read. 
		 */
		virtual bool readInputStream(char * buffer, size_t & size) = 0;

		/** Writes the given buffer to the output, i.e. sends the data to the connected application. 
		 */
		virtual bool write(char const * buffer, size_t size) = 0;

	private:

		static void InputStreamReader(IOTerminal * terminal);

		std::thread inputReader_;

		char * inputBuffer_;

		size_t inputBufferSize_;

	};

} // namespace vterm
