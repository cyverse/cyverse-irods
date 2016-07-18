#include "irods_stacktrace.hpp"
#include "rodsErrorTable.h"
#include "irods_log.hpp"

#include <iostream>

#include <execinfo.h>
#include <stdlib.h>
#include <cxxabi.h>
namespace {
    class stacktrace_resolution_exception : public std::runtime_error
    {
        public :
            stacktrace_resolution_exception(const std::string& message) :
                std::runtime_error(message) {}
    };
}
namespace irods {

    stacktrace::stacktrace( void ) {
        size_ = backtrace( backtrace_, max_stack_size );
    }

    stacktrace::~stacktrace( void ) {
        // TODO - stub
    }

    //this is very, very, very slow. do not call it more
    //than once per stacktrace object
    stacktrace::stacklist stacktrace::resolve_stack() const {
        if ( size_ == 0 ) {
            throw stacktrace_resolution_exception("Stack trace is empty");
        }

        char** symbols = backtrace_symbols( backtrace_, size_ );
        if ( !symbols ) {
            throw stacktrace_resolution_exception("Cannot generate stack symbols");
        }

        stacklist stack;
        for ( int i = 1; i < size_; ++i ) {
            char* symbol = symbols[i];
            if ( !symbol ) {
                free( symbols );
                throw stacktrace_resolution_exception("Corrupt stack trace. Symbol is NULL.");
            }
            std::string demangled;
            std::string offset;
            demangle_symbol( symbol, demangled, offset );
            stack_entry_t entry;
            entry.function = demangled;
            entry.offset = offset;
            entry.address = backtrace_[i];
            stack.push_back( entry );
        }
        free( symbols );
        return stack;
    }

    const std::string& stacktrace::dump() const {
        if ( !dump_ ) {
            try {
                stacklist stack = resolve_stack();

                unsigned int max_function_length = 0;
                for ( stacklist::const_iterator it = stack.begin(); it != stack.end(); ++it ) {
                    stack_entry_t entry = *it;
                    if ( entry.function.length() > max_function_length ) {
                        max_function_length = entry.function.length();
                    }
                }
                std::stringstream strm;
                strm << std::endl << "Dumping stack trace" << std::endl;
                int frame = 0;
                for ( stacklist::const_iterator it = stack.begin(); it != stack.end(); ++it ) {
                    stack_entry_t entry = *it;
                    strm << "<" << frame << ">";
                    strm << "\t" << entry.function;
                    int pad_amount = max_function_length - entry.function.length();
                    for ( int i = 0; i < pad_amount; ++i ) {
                        strm << " ";
                    }
                    strm << "\t" << "Offset: " << entry.offset;
                    strm << "\t" << "Address: " << entry.address;
                    strm << std::endl;
                    ++frame;
                }
                strm << std::endl;
                dump_.reset(strm.str());
            } catch ( const stacktrace_resolution_exception& e ) {
                dump_.reset( e.what() );
            }
        }
        return *dump_;
    }

    void stacktrace::demangle_symbol (
        const std::string& _symbol,
        std::string& _rtn_name,
        std::string& _rtn_offset ) const {
        error result = SUCCESS();
        _rtn_name = _symbol; // if we cannot demangle the symbol return the original.
        _rtn_offset.clear();

        // find the open paren
        size_t startpos = _symbol.find( "(" );
        size_t offsetpos = _symbol.find( "+", startpos );
        size_t nameendpos = _symbol.find( ")", startpos );

        if ( startpos != std::string::npos && nameendpos != std::string::npos ) {
            ++startpos;
            std::string name_symbol;
            std::string offset_string;
            if ( offsetpos != std::string::npos ) { // handle the case where there is no offset
                name_symbol = _symbol.substr( startpos, offsetpos - startpos );
                ++offsetpos;
                offset_string = _symbol.substr( offsetpos, nameendpos - offsetpos );
            }
            else {
                name_symbol = _symbol.substr( startpos, nameendpos - startpos );
            }
            char* name_buffer;
            int status;
            name_buffer = abi::__cxa_demangle( name_symbol.c_str(), NULL, NULL, &status );
            if ( status == 0 ) {
                _rtn_name = name_buffer;
                if ( !offset_string.empty() ) {
                    _rtn_offset = offset_string;
                }
                free( name_buffer );
            }

        }
    }

}; // namespace irods
