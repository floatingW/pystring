// Copyright Contributors to the Pystring project.
// SPDX-License-Identifier: BSD-3-Clause
// https://github.com/imageworks/pystring/blob/master/LICENSE


#include "pystring.h"

#include <algorithm>
#include <cctype>
#include <cstdint>
#include <cstring>
#include <sstream>
#include <string_view>

namespace pystring
{

#if defined(_WIN32) || defined(_WIN64) || defined(_WINDOWS) || defined(_MSC_VER)
#ifndef WINDOWS
#define WINDOWS
#endif
#endif

// This definition codes from configure.in in the python src.
// Strictly speaking this limits us to str sizes of 2**31.
// Should we wish to handle this limit, we could use an architecture
// specific #defines and read from ssize_t (unistd.h) if the header exists.
// But in the meantime, the use of int assures maximum arch compatibility.
// This must also equal the size used in the end = MAX_32BIT_INT default arg.

typedef int Py_ssize_t;
const std::string forward_slash = "/";
const std::string double_forward_slash = "//";
const std::string triple_forward_slash = "///";
const std::string double_back_slash = "\\";
const std::string empty_string = "";
const std::string dot = ".";
const std::string double_dot = "..";
const std::string colon = ":";


/* helper macro to fixup start/end slice values */
#define ADJUST_INDICES(start, end, len)         \
    if (end > len)                          \
        end = len;                          \
    else if (end < 0) {                     \
        end += len;                         \
        if (end < 0)                        \
        end = 0;                        \
    }                                       \
    if (start < 0) {                        \
        start += len;                       \
        if (start < 0)                      \
        start = 0;                      \
    }


	namespace {

		//////////////////////////////////////////////////////////////////////////////////////////////
		/// why doesn't the std::reverse work?
		///
		void reverse_strings( std::vector< std::string > & result)
		{
			for (std::vector< std::string >::size_type i = 0; i < result.size() / 2; i++ )
			{
				std::swap(result[i], result[result.size() - 1 - i]);
			}
		}

		//////////////////////////////////////////////////////////////////////////////////////////////
		///
		///
		void split_whitespace( std::string_view str, std::vector< std::string > & result, int maxsplit )
		{
			std::string::size_type i, j, len = str.size();
			for (i = j = 0; i < len; )
			{

				while ( i < len && ::isspace( str[i] ) ) i++;
				j = i;

				while ( i < len && ! ::isspace( str[i]) ) i++;



				if (j < i)
				{
					if ( maxsplit-- <= 0 ) break;

					result.push_back(std::string( str.substr( j, i - j )));

					while ( i < len && ::isspace( str[i])) i++;
					j = i;
				}
			}
			if (j < len)
			{
				result.push_back(std::string( str.substr( j, len - j )));
			}
		}


		//////////////////////////////////////////////////////////////////////////////////////////////
		///
		///
		void rsplit_whitespace( std::string_view str, std::vector< std::string > & result, int maxsplit )
		{
			std::string::size_type len = str.size();
			std::string::size_type i, j;
			for (i = j = len; i > 0; )
			{

				while ( i > 0 && ::isspace( str[i - 1] ) ) i--;
				j = i;

				while ( i > 0 && ! ::isspace( str[i - 1]) ) i--;



				if (j > i)
				{
					if ( maxsplit-- <= 0 ) break;

					result.push_back(std::string(str.substr( i, j - i )));

					while ( i > 0 && ::isspace( str[i - 1])) i--;
					j = i;
				}
			}
			if (j > 0)
			{
				result.push_back(std::string(str.substr( 0, j )));
			}
			//std::reverse( result, result.begin(), result.end() );
			reverse_strings( result );
		}

	} //anonymous namespace


    //////////////////////////////////////////////////////////////////////////////////////////////
    ///
    ///
    void split( std::string_view str, std::vector< std::string > & result, std::string_view sep, int maxsplit )
    {
        result.clear();

        if ( maxsplit < 0 ) maxsplit = MAX_32BIT_INT;//result.max_size();


        if ( sep.size() == 0 )
        {
            split_whitespace( str, result, maxsplit );
            return;
        }

        std::string::size_type i,j, len = str.size(), n = sep.size();

        i = j = 0;

        while ( i+n <= len )
        {
            if ( str[i] == sep[0] && str.substr( i, n ) == sep )
            {
                if ( maxsplit-- <= 0 ) break;

                result.push_back(std::string(str.substr( j, i - j )));
                i = j = i + n;
            }
            else
            {
                i++;
            }
        }

        result.push_back(std::string(str.substr( j, len-j )));
    }

    //////////////////////////////////////////////////////////////////////////////////////////////
    ///
    ///
    void rsplit( std::string_view str, std::vector< std::string > & result, std::string_view sep, int maxsplit )
    {
        if ( maxsplit < 0 )
        {
            split( str, result, sep, maxsplit );
            return;
        }

        result.clear();

        if ( sep.size() == 0 )
        {
            rsplit_whitespace( str, result, maxsplit );
            return;
        }

        Py_ssize_t i,j, len = (Py_ssize_t) str.size(), n = (Py_ssize_t) sep.size();

        i = j = len;

        while ( i >= n )
        {
            if ( str[i - 1] == sep[n - 1] && str.substr( i - n, n ) == sep )
            {
                if ( maxsplit-- <= 0 ) break;

                result.push_back(std::string(str.substr( i, j - i )));
                i = j = i - n;
            }
            else
            {
                i--;
            }
        }

        result.push_back(std::string(str.substr( 0, j )));
        reverse_strings( result );
    }

    //////////////////////////////////////////////////////////////////////////////////////////////
    ///
    ///
    enum class string_strip_direction_ : std::uint8_t
    {
        leftstrip,
        rightstrip,
        bothstrip
    };

    //////////////////////////////////////////////////////////////////////////////////////////////
    ///
    ///
    std::string do_strip( std::string_view str, string_strip_direction_ striptype, std::string_view chars  )
    {
        Py_ssize_t len = (Py_ssize_t) str.size(), i, j, charslen = (Py_ssize_t) chars.size();

        if ( charslen == 0 )
        {
            i = 0;
            if ( striptype != string_strip_direction_::rightstrip )
            {
                while ( i < len && ::isspace( str[i] ) )
                {
                    i++;
                }
            }

            j = len;
            if ( striptype != string_strip_direction_::leftstrip )
            {
                do
                {
                    j--;
                }
                while (j >= i && ::isspace(str[j]));

                j++;
            }


        }
        else
        {
            const char * sep = chars.data();

            i = 0;
            if ( striptype != string_strip_direction_::rightstrip )
            {
                while ( i < len && memchr(sep, str[i], charslen) )
                {
                    i++;
                }
            }

            j = len;
            if (striptype != string_strip_direction_::leftstrip)
            {
                do
                {
                    j--;
                }
                while (j >= i &&  memchr(sep, str[j], charslen)  );
                j++;
            }


        }

        if ( i == 0 && j == len )
        {
            return std::string(str);
        }
        else
        {
            return std::string(str.substr( i, j - i ));
        }

    }

    //////////////////////////////////////////////////////////////////////////////////////////////
    ///
    ///
    void partition( std::string_view str, std::string_view sep, std::vector< std::string > & result )
    {
        result.resize(3);
        int index = find( str, sep );
        if ( index < 0 )
        {
            result[0] = str;
            result[1] = empty_string;
            result[2] = empty_string;
        }
        else
        {
            result[0] = str.substr( 0, index );
            result[1] = sep;
            result[2] = str.substr( index + sep.size(), str.size() );
        }
    }

    //////////////////////////////////////////////////////////////////////////////////////////////
    ///
    ///
    void rpartition( std::string_view str, std::string_view sep, std::vector< std::string > & result )
    {
        result.resize(3);
        int index = rfind( str, sep );
        if ( index < 0 )
        {
            result[0] = empty_string;
            result[1] = empty_string;
            result[2] = str;
        }
        else
        {
            result[0] = str.substr( 0, index );
            result[1] = sep;
            result[2] = str.substr( index + sep.size(), str.size() );
        }
    }

    //////////////////////////////////////////////////////////////////////////////////////////////
    ///
    ///
    std::string strip( std::string_view str, std::string_view chars )
    {
        return do_strip( str, string_strip_direction_::bothstrip, chars );
    }

    //////////////////////////////////////////////////////////////////////////////////////////////
    ///
    ///
    std::string lstrip( std::string_view str, std::string_view chars )
    {
        return do_strip( str, string_strip_direction_::leftstrip, chars );
    }

    //////////////////////////////////////////////////////////////////////////////////////////////
    ///
    ///
    std::string rstrip( std::string_view str, std::string_view chars )
    {
        return do_strip( str, string_strip_direction_::rightstrip, chars );
    }

    //////////////////////////////////////////////////////////////////////////////////////////////
    ///
    ///
    std::string join( std::string_view str, const std::vector< std::string > & seq )
    {
        std::vector< std::string >::size_type seqlen = seq.size(), i;

        if ( seqlen == 0 ) return empty_string;
        if ( seqlen == 1 ) return seq[0];

        std::string result( seq[0] );

        for ( i = 1; i < seqlen; ++i )
        {
            result += std::string(str) + seq[i];

        }


        return result;
    }


    //////////////////////////////////////////////////////////////////////////////////////////////
    ///
    ///
    
    namespace
    {
        enum class string_tailmatch_direction_ : std::uint8_t
        {
            startswith,
            endswith
        };

        /* Matches the end (direction >= 0) or start (direction < 0) of self
         * against substr, using the start and end arguments. Returns
         * -1 on error, 0 if not found and 1 if found.
         */
        bool _string_tailmatch(std::string_view self, std::string_view substr,
                              Py_ssize_t start, Py_ssize_t end,
                              string_tailmatch_direction_ direction)
        {
            Py_ssize_t len = (Py_ssize_t) self.size();
            Py_ssize_t slen = (Py_ssize_t) substr.size();
            
            const char* sub = substr.data();
            const char* str = self.data();
            
            ADJUST_INDICES(start, end, len);
            
            if (direction == string_tailmatch_direction_::startswith) {
                // startswith
                if (start+slen > len)
                    return false;
            } else {
                // endswith
                if (end-start < slen || start > len)
                    return false;
                if (end-slen > start)
                    start = end - slen;
            }
            if (end-start >= slen)
                return (!std::memcmp(str+start, sub, slen));
            
            return false;
        }
    }
    
    bool endswith( std::string_view str, std::string_view suffix, int start, int end )
    {
        auto result = _string_tailmatch(str, suffix,
                                       (Py_ssize_t) start, (Py_ssize_t) end,
                                       string_tailmatch_direction_::endswith);
        //if (result == -1) // TODO: Error condition
        
        return result;
    }
    
    
    bool startswith( std::string_view str, std::string_view prefix, int start, int end )
    {
        auto result = _string_tailmatch(str, prefix,
                                       (Py_ssize_t) start, (Py_ssize_t) end,
                                       string_tailmatch_direction_::startswith);
        //if (result == -1) // TODO: Error condition
        
        return result;
    }

    //////////////////////////////////////////////////////////////////////////////////////////////
    ///
    ///

    bool isalnum( std::string_view str )
    {
        std::string::size_type len = str.size(), i;
        if ( len == 0 ) return false;


        if( len == 1 )
        {
            return ::isalnum( str[0] );
        }

        for ( i = 0; i < len; ++i )
        {
            if ( !::isalnum( str[i] ) ) return false;
        }
        return true;
    }

    //////////////////////////////////////////////////////////////////////////////////////////////
    ///
    ///
    bool isalpha( std::string_view str )
    {
        std::string::size_type len = str.size(), i;
        if ( len == 0 ) return false;
        if( len == 1 ) return ::isalpha( (int) str[0] );

        for ( i = 0; i < len; ++i )
        {
           if ( !::isalpha( (int) str[i] ) ) return false;
        }
        return true;
    }

    //////////////////////////////////////////////////////////////////////////////////////////////
    ///
    ///
    bool isdigit( std::string_view str )
    {
        std::string::size_type len = str.size(), i;
        if ( len == 0 ) return false;
        if( len == 1 ) return ::isdigit( str[0] );

        for ( i = 0; i < len; ++i )
        {
           if ( ! ::isdigit( str[i] ) ) return false;
        }
        return true;
    }

    //////////////////////////////////////////////////////////////////////////////////////////////
    ///
    ///
    bool islower( std::string_view str )
    {
        std::string::size_type len = str.size(), i;
        if ( len == 0 ) return false;
        if( len == 1 ) return ::islower( str[0] );

        for ( i = 0; i < len; ++i )
        {
           if ( !::islower( str[i] ) ) return false;
        }
        return true;
    }

    //////////////////////////////////////////////////////////////////////////////////////////////
    ///
    ///
    bool isspace( std::string_view str )
    {
        std::string::size_type len = str.size(), i;
        if ( len == 0 ) return false;
        if( len == 1 ) return ::isspace( str[0] );

        for ( i = 0; i < len; ++i )
        {
           if ( !::isspace( str[i] ) ) return false;
        }
        return true;
    }

    //////////////////////////////////////////////////////////////////////////////////////////////
    ///
    ///
    bool istitle( std::string_view str )
    {
        std::string::size_type len = str.size(), i;

        if ( len == 0 ) return false;
        if ( len == 1 ) return ::isupper( str[0] );

        bool cased = false, previous_is_cased = false;

        for ( i = 0; i < len; ++i )
        {
            if ( ::isupper( str[i] ) )
            {
                if ( previous_is_cased )
                {
                    return false;
                }

                previous_is_cased = true;
                cased = true;
            }
            else if ( ::islower( str[i] ) )
            {
                if (!previous_is_cased)
                {
                    return false;
                }

                previous_is_cased = true;
                cased = true;

            }
            else
            {
                previous_is_cased = false;
            }
        }

        return cased;
    }

    //////////////////////////////////////////////////////////////////////////////////////////////
    ///
    ///
    bool isupper( std::string_view str )
    {
        std::string::size_type len = str.size(), i;
        if ( len == 0 ) return false;
        if( len == 1 ) return ::isupper( str[0] );

        for ( i = 0; i < len; ++i )
        {
           if ( !::isupper( str[i] ) ) return false;
        }
        return true;
    }

    //////////////////////////////////////////////////////////////////////////////////////////////
    ///
    ///
    std::string capitalize( std::string_view str )
    {
        std::string s( str );
        std::string::size_type len = s.size(), i;

        if ( len > 0)
        {
            if (::islower(s[0])) s[0] = (char) ::toupper( s[0] );
        }

        for ( i = 1; i < len; ++i )
        {
            if (::isupper(s[i])) s[i] = (char) ::tolower( s[i] );
        }

        return s;
    }

    //////////////////////////////////////////////////////////////////////////////////////////////
    ///
    ///
    std::string lower( std::string_view str )
    {
        std::string s( str );
        std::string::size_type len = s.size(), i;

        for ( i = 0; i < len; ++i )
        {
            if ( ::isupper( s[i] ) ) s[i] = (char) ::tolower( s[i] );
        }

        return s;
    }

    //////////////////////////////////////////////////////////////////////////////////////////////
    ///
    ///
    std::string upper( std::string_view str )
    {
        std::string s( str ) ;
        std::string::size_type len = s.size(), i;

        for ( i = 0; i < len; ++i )
        {
            if ( ::islower( s[i] ) ) s[i] = (char) ::toupper( s[i] );
        }

        return s;
    }

    //////////////////////////////////////////////////////////////////////////////////////////////
    ///
    ///
    std::string swapcase( std::string_view str )
    {
        std::string s( str );
        std::string::size_type len = s.size(), i;

        for ( i = 0; i < len; ++i )
        {
            if ( ::islower( s[i] ) ) s[i] = (char) ::toupper( s[i] );
            else if (::isupper( s[i] ) ) s[i] = (char) ::tolower( s[i] );
        }

        return s;
    }

    //////////////////////////////////////////////////////////////////////////////////////////////
    ///
    ///
    std::string title( std::string_view str )
    {
        std::string s( str );
        std::string::size_type len = s.size(), i;
        bool previous_is_cased = false;

        for ( i = 0; i < len; ++i )
        {
            int c = s[i];
            if ( ::islower(c) )
            {
                if ( !previous_is_cased )
                {
                    s[i] = (char) ::toupper(c);
                }
                previous_is_cased = true;
            }
            else if ( ::isupper(c) )
            {
                if ( previous_is_cased )
                {
                    s[i] = (char) ::tolower(c);
                }
                previous_is_cased = true;
            }
            else
            {
                previous_is_cased = false;
            }
        }

        return s;
    }

    //////////////////////////////////////////////////////////////////////////////////////////////
    ///
    ///
    std::string translate( std::string_view str, std::string_view table, std::string_view deletechars )
    {
        std::string s;
        std::string::size_type len = str.size(), dellen = deletechars.size();

        if ( table.size() != 256 )
        {
            // TODO : raise exception instead
            return std::string(str);
        }

        //if nothing is deleted, use faster code
        if ( dellen == 0 )
        {
            s = str;
            for ( std::string::size_type i = 0; i < len; ++i )
            {
                s[i] = table[ s[i] ];
            }
            return s;
        }


        int trans_table[256];
        for ( int i = 0; i < 256; i++)
        {
            trans_table[i] = table[i];
        }

        for ( std::string::size_type i = 0; i < dellen; i++)
        {
            trans_table[(int) deletechars[i] ] = -1;
        }

        for ( std::string::size_type i = 0; i < len; ++i )
        {
            if ( trans_table[ (int) str[i] ] != -1 )
            {
                s += table[ str[i] ];
            }
        }

        return s;

    }


    //////////////////////////////////////////////////////////////////////////////////////////////
    ///
    ///
    std::string zfill( std::string_view str, int width )
    {
        int len = (int)str.size();

        if ( len >= width )
        {
            return std::string(str);
        }

        std::string s( str );

        int fill = width - len;

        s = std::string( fill, '0' ) + s;


        if ( s[fill] == '+' || s[fill] == '-' )
        {
            s[0] = s[fill];
            s[fill] = '0';
        }

        return s;

    }

    //////////////////////////////////////////////////////////////////////////////////////////////
    ///
    ///
    std::string ljust( std::string_view str, int width )
    {
        std::string::size_type len = str.size();
        if ( (( int ) len ) >= width ) return std::string(str);
        return std::string(str) + std::string( width - len, ' ' );
    }

    //////////////////////////////////////////////////////////////////////////////////////////////
    ///
    ///
    std::string rjust( std::string_view str, int width )
    {
        std::string::size_type len = str.size();
        if ( (( int ) len ) >= width ) return std::string(str);
        return std::string( width - len, ' ' ) + std::string(str);
    }

    //////////////////////////////////////////////////////////////////////////////////////////////
    ///
    ///
    std::string center( std::string_view str, int width )
    {
        int len = (int) str.size();
        int marg, left;

        if ( len >= width ) return std::string(str);

        marg = width - len;
        left = marg / 2 + (marg & width & 1);

        return std::string( left, ' ' ) + std::string(str) + std::string( marg - left, ' ' );

    }

    //////////////////////////////////////////////////////////////////////////////////////////////
    ///
    ///
    std::string slice( std::string_view str, int start, int end )
    {
        ADJUST_INDICES(start, end, (int) str.size());
        if ( start >= end ) return empty_string;
        return std::string(str.substr( start, end - start ));
    }
    
    
    //////////////////////////////////////////////////////////////////////////////////////////////
    ///
    ///
    int find( std::string_view str, std::string_view sub, int start, int end  )
    {
        ADJUST_INDICES(start, end, (int) str.size());
        
        std::string::size_type result = str.find( sub, start );
        
        // If we cannot find the string, or if the end-point of our found substring is past
        // the allowed end limit, return that it can't be found.
        if( result == std::string::npos || 
           (result + sub.size() > (std::string::size_type)end) )
        {
            return -1;
        }
        
        return (int) result;
    }

    //////////////////////////////////////////////////////////////////////////////////////////////
    ///
    ///
    int index( std::string_view str, std::string_view sub, int start, int end  )
    {
        return find( str, sub, start, end );
    }

    //////////////////////////////////////////////////////////////////////////////////////////////
    ///
    ///
    int rfind( std::string_view str, std::string_view sub, int start, int end )
    {
        ADJUST_INDICES(start, end, (int) str.size());
        
        std::string::size_type result = str.rfind( sub, end );
        
        if( result == std::string::npos || 
            result < (std::string::size_type)start  || 
           (result + sub.size() > (std::string::size_type)end))
            return -1;
        
        return (int)result;
    }

    //////////////////////////////////////////////////////////////////////////////////////////////
    ///
    ///
    int rindex( std::string_view str, std::string_view sub, int start, int end )
    {
        return rfind( str, sub, start, end );
    }

    //////////////////////////////////////////////////////////////////////////////////////////////
    ///
    ///
    std::string expandtabs( std::string_view str, int tabsize )
    {
        std::string s( str );

        std::string::size_type len = str.size(), i = 0;
        int offset = 0;

        int j = 0;

        for ( i = 0; i < len; ++i )
        {
            if ( str[i] == '\t' )
            {

                if ( tabsize > 0 )
                {
                    int fillsize = tabsize - (j % tabsize);
                    j += fillsize;
                    s.replace( i + offset, 1, std::string( fillsize, ' ' ));
                    offset += fillsize - 1;
                }
                else
                {
                    s.replace( i + offset, 1, empty_string );
                    offset -= 1;
                }

            }
            else
            {
                j++;

                if (str[i] == '\n' || str[i] == '\r')
                {
                    j = 0;
                }
            }
        }

        return s;
    }

    //////////////////////////////////////////////////////////////////////////////////////////////
    ///
    ///
    int count( std::string_view str, std::string_view substr, int start, int end )
    {
        int nummatches = 0;
        int cursor = start;

        while ( 1 )
        {
            cursor = find( str, substr, cursor, end );

            if ( cursor < 0 ) break;

            cursor += (int) substr.size();
            nummatches += 1;
        }

        return nummatches;


    }

    //////////////////////////////////////////////////////////////////////////////////////////////
    ///
    ///
    
    std::string replace( std::string_view str, std::string_view oldstr, std::string_view newstr, int count )
    {
        int sofar = 0;
        int cursor = 0;
        std::string s( str );

        std::string::size_type oldlen = oldstr.size(), newlen = newstr.size();
        
        cursor = find( s, oldstr, cursor );

        while ( cursor != -1 && cursor <= (int)s.size() )
        {
            if ( count > -1 && sofar >= count )
            {
                break;
            }

            s.replace( cursor, oldlen, newstr );
            cursor += (int) newlen;

            if ( oldlen != 0)
            {
                cursor = find( s, oldstr, cursor );
            }
            else
            {
                ++cursor;
            }

            ++sofar;
        }

        return s;

    }
    
    
    //////////////////////////////////////////////////////////////////////////////////////////////
    ///
    ///
    void splitlines(  std::string_view str, std::vector< std::string > & result, bool keepends )
    {
        result.clear();
        std::string::size_type len = str.size(), i, j, eol;

         for (i = j = 0; i < len; )
         {
            while (i < len && str[i] != '\n' && str[i] != '\r') i++;

            eol = i;
            if (i < len)
            {
                if (str[i] == '\r' && i + 1 < len && str[i+1] == '\n')
                {
                    i += 2;
                }
                else
                {
                    i++;
                }
                if (keepends)
                eol = i;

            }

            result.push_back(std::string( str.substr( j, eol - j ) ));
            j = i;

        }

        if (j < len)
        {
            result.push_back(std::string( str.substr( j, len - j ) ));
        }

    }

    //////////////////////////////////////////////////////////////////////////////////////////////
    ///
    ///
    std::string mul( std::string_view str, int n )
    {
        // Early exits
        if (n <= 0) return empty_string;
        if (n == 1) return std::string(str);
        
        std::ostringstream os;
        for(int i=0; i<n; ++i)
        {
            os << str;
        }
        return os.str();
    }

    //////////////////////////////////////////////////////////////////////////////////////////////
    ///
    ///
    std::string removeprefix( std::string_view str, std::string_view prefix )
    {
        if (pystring::startswith(str, prefix))
        {
            return std::string(str.substr(prefix.length()));
        }

        return std::string(str);
    }

    //////////////////////////////////////////////////////////////////////////////////////////////
    ///
    ///
    std::string removesuffix( std::string_view str, std::string_view suffix )
    {
        if (pystring::endswith(str, suffix))
        {
            return std::string(str.substr(0, str.length() - suffix.length()));
        }

        return std::string(str);
    }


namespace os
{
namespace path
{
    
    //////////////////////////////////////////////////////////////////////////////////////////////
    ///
    ///
    /// These functions are C++ ports of the python2.6 versions of os.path,
    /// and come from genericpath.py, ntpath.py, posixpath.py

    /// Split a pathname into drive and path specifiers.
    /// Returns drivespec, pathspec. Either part may be empty.
    void splitdrive_nt(std::string & drivespec, std::string & pathspec,
                       std::string_view p)
    {
        if (p.size() >= 2 && p[1] == ':')
        {
            std::string path = std::string(p); // In case drivespec == p
            drivespec = pystring::slice(path, 0, 2);
            pathspec = pystring::slice(path, 2);
        }
        else
        {
            drivespec = empty_string;
            pathspec = p;
        }
    }

    // On Posix, drive is always empty
    void splitdrive_posix(std::string & drivespec, std::string & pathspec,
                          std::string_view path)
    {
        drivespec = empty_string;
        pathspec = path;
    }

    void splitdrive(std::string & drivespec, std::string & pathspec,
                    std::string_view path)
    {
#ifdef WINDOWS
        return splitdrive_nt(drivespec, pathspec, path);
#else
        return splitdrive_posix(drivespec, pathspec, path);
#endif
    }

    //////////////////////////////////////////////////////////////////////////////////////////////
    ///
    ///

    // Test whether a path is absolute
    // In windows, if the character to the right of the colon
    // is a forward or backslash it's absolute.
    bool isabs_nt(std::string_view path)
    {
        std::string drivespec, pathspec;
        splitdrive_nt(drivespec, pathspec, path);
        if(pathspec.empty()) return false;
        return ((pathspec[0] == '/') || (pathspec[0] == '\\'));
    }

    bool isabs_posix(std::string_view s)
    {
        return pystring::startswith(s, forward_slash);
    }

    bool isabs(std::string_view path)
    {
#ifdef WINDOWS
        return isabs_nt(path);
#else
        return isabs_posix(path);
#endif
    }


    //////////////////////////////////////////////////////////////////////////////////////////////
    ///
    ///
    
    std::string abspath_nt(std::string_view path, std::string_view cwd)
    {
        std::string p = std::string(path);
        if(!isabs_nt(p)) p = join_nt(cwd, p);
        return normpath_nt(p);
    }
    
    std::string abspath_posix(std::string_view path, std::string_view cwd)
    {
        std::string p = std::string(path);
        if(!isabs_posix(p)) p = join_posix(cwd, p);
        return normpath_posix(p);
    }
    
    std::string abspath(std::string_view path, std::string_view cwd)
    {
#ifdef WINDOWS
        return abspath_nt(path, cwd);
#else
        return abspath_posix(path, cwd);
#endif
    }
    

    //////////////////////////////////////////////////////////////////////////////////////////////
    ///
    ///

    std::string join_nt(const std::vector< std::string > & paths)
    {
        if(paths.empty()) return empty_string;
        if(paths.size() == 1) return paths[0];
        
        std::string path = paths[0];
        
        for(unsigned int i=1; i<paths.size(); ++i)
        {
            std::string b = paths[i];
            
            bool b_nts = false;
            if(path.empty())
            {
                b_nts = true;
            }
            else if(isabs_nt(b))
            {
                // This probably wipes out path so far.  However, it's more
                // complicated if path begins with a drive letter:
                //     1. join('c:', '/a') == 'c:/a'
                //     2. join('c:/', '/a') == 'c:/a'
                // But
                //     3. join('c:/a', '/b') == '/b'
                //     4. join('c:', 'd:/') = 'd:/'
                //     5. join('c:/', 'd:/') = 'd:/'
                

                if ((path.size() >= 2 && path[1] != ':') || (b.size() >= 2 && b[1] == ':'))
                {
                    // Path doesnt start with a drive letter
                    b_nts = true;
                }
                // Else path has a drive letter, and b doesn't but is absolute.
                else if((path.size()>3) || 
                        ((path.size()==3) && !pystring::endswith(path, forward_slash) && !pystring::endswith(path, double_back_slash)))
                {
                    b_nts = true;
                }
            }
            
            if(b_nts)
            {
                path = b;
            }
            else
            {
                // Join, and ensure there's a separator.
                // assert len(path) > 0
                if( pystring::endswith(path, forward_slash) || pystring::endswith(path, double_back_slash))
                {
                    if(pystring::startswith(b,forward_slash) || pystring::startswith(b,double_back_slash))
                    {
                        path += pystring::slice(b, 1);
                    }
                    else
                    {
                        path += b;
                    }
                }
                else if(pystring::endswith(path, colon))
                {
                    path += b;
                }
                else if(!b.empty())
                {
                    if(pystring::startswith(b, forward_slash) || pystring::startswith(b,double_back_slash))
                    {
                        path += b;
                    }
                    else
                    {
                        path += double_back_slash + b;
                    }
                }
                else
                {
                    // path is not empty and does not end with a backslash,
                    // but b is empty; since, e.g., split('a/') produces
                    // ('a', ''), it's best if join() adds a backslash in
                    // this case.
                    path += double_back_slash;
                }
            }
        }
        
        return path;
    }
    
    // Join two or more pathname components, inserting double_back_slash as needed.
    std::string join_nt(std::string_view a, std::string_view b)
    {
        std::vector< std::string > paths(2);
        paths[0] = a;
        paths[1] = b;
        return join_nt(paths);
    }

    // Join pathnames.
    // If any component is an absolute path, all previous path components
    // will be discarded.
    // Ignore the previous parts if a part is absolute.
    // Insert a '/' unless the first part is empty or already ends in '/'.

    std::string join_posix(const std::vector< std::string > & paths)
    {
        if(paths.empty()) return empty_string;
        if(paths.size() == 1) return paths[0];
        
        std::string path = paths[0];
        
        for(unsigned int i=1; i<paths.size(); ++i)
        {
            std::string b = paths[i];
            if(pystring::startswith(b, forward_slash))
            {
                path = b;
            }
            else if(path.empty() || pystring::endswith(path, forward_slash))
            {
                path += b;
            }
            else
            {
                path += forward_slash + b;
            }
        }
        
        return path;
    }

    std::string join_posix(std::string_view a, std::string_view b)
    {
        std::vector< std::string > paths(2);
        paths[0] = a;
        paths[1] = b;
        return join_posix(paths);
    }
    
    std::string join(std::string_view path1, std::string_view path2)
    {
#ifdef WINDOWS
        return join_nt(path1, path2);
#else
        return join_posix(path1, path2);
#endif
    }


    std::string join(const std::vector< std::string > & paths)
    {
#ifdef WINDOWS
        return join_nt(paths);
#else
        return join_posix(paths);
#endif
    }
    
    //////////////////////////////////////////////////////////////////////////////////////////////
    ///
    ///

        
    // Split a pathname.
    // Return (head, tail) where tail is everything after the final slash.
    // Either part may be empty

    void split_nt(std::string & head, std::string & tail, std::string_view path)
    {
        std::string d, p;
        splitdrive_nt(d, p, path);
        
        // set i to index beyond p's last slash
        int i = (int)p.size();

        // walk back to find the index of the first slash from the end       
        while(i>0 && (p[i-1] != '\\') && (p[i-1] != '/'))
        {
            i = i - 1;
        }

        head = pystring::slice(p,0,i);
        tail = pystring::slice(p,i); // now tail has no slashes
        
        // remove trailing slashes from head, unless it's all slashes
        std::string head2 = head;
        while(!head2.empty() && ((pystring::slice(head2,-1) == forward_slash) ||
                                 (pystring::slice(head2,-1) == double_back_slash)))
        {
            head2 = pystring::slice(head2,0,-1);
        }
        
        if(!head2.empty()) head = head2;
        head = d + head;
    }


    // Split a path in head (everything up to the last '/') and tail (the
    // rest).  If the path ends in '/', tail will be empty.  If there is no
    // '/' in the path, head  will be empty.
    // Trailing '/'es are stripped from head unless it is the root.

    void split_posix(std::string & head, std::string & tail, std::string_view p)
    {
        int i = pystring::rfind(p, forward_slash) + 1;
        
        head = pystring::slice(p,0,i);
        tail = pystring::slice(p,i);
        
        if(!head.empty() && (head != pystring::mul(forward_slash, (int) head.size())))
        {
            head = pystring::rstrip(head, forward_slash);
        }
    }

    void split(std::string & head, std::string & tail, std::string_view path)
    {
#ifdef WINDOWS
        return split_nt(head, tail, path);
#else
        return split_posix(head, tail, path);
#endif
    }


    //////////////////////////////////////////////////////////////////////////////////////////////
    ///
    ///

    std::string basename_nt(std::string_view path)
    {
        std::string head, tail;
        split_nt(head, tail, path);
        return tail;
    }

    std::string basename_posix(std::string_view path)
    {
        std::string head, tail;
        split_posix(head, tail, path);
        return tail;
    }

    std::string basename(std::string_view path)
    {
#ifdef WINDOWS
        return basename_nt(path);
#else
        return basename_posix(path);
#endif
    }

    std::string dirname_nt(std::string_view path)
    {
        std::string head, tail;
        split_nt(head, tail, path);
        return head;
    }
    
    std::string dirname_posix(std::string_view path)
    {
        std::string head, tail;
        split_posix(head, tail, path);
        return head;
    }
    
    std::string dirname(std::string_view path)
    {
#ifdef WINDOWS
        return dirname_nt(path);
#else
        return dirname_posix(path);
#endif
    }


    //////////////////////////////////////////////////////////////////////////////////////////////
    ///
    ///

    // Normalize a path, e.g. A//B, A/./B and A/foo/../B all become A\B.
    std::string normpath_nt(std::string_view p)
    {
        std::string path = std::string(p);
        path = pystring::replace(path, forward_slash,double_back_slash);
        
        std::string prefix;
        splitdrive_nt(prefix, path, path);
        
        // We need to be careful here. If the prefix is empty, and the path starts
        // with a backslash, it could either be an absolute path on the current
        // drive (\dir1\dir2\file) or a UNC filename (\\server\mount\dir1\file). It
        // is therefore imperative NOT to collapse multiple backslashes blindly in
        // that case.
        // The code below preserves multiple backslashes when there is no drive
        // letter. This means that the invalid filename \\\a\b is preserved
        // unchanged, where a\\\b is normalised to a\b. It's not clear that there
        // is any better behaviour for such edge cases.
        
        if(prefix.empty())
        {
            // No drive letter - preserve initial backslashes
            while(pystring::slice(path,0,1) == double_back_slash)
            {
                prefix = prefix + double_back_slash;
                path = pystring::slice(path,1);
            }
        }
        else
        {
            // We have a drive letter - collapse initial backslashes
            if(pystring::startswith(path, double_back_slash))
            {
                prefix = prefix + double_back_slash;
                path = pystring::lstrip(path, double_back_slash);
            }
        }
        
        std::vector<std::string> comps;
        pystring::split(path, comps, double_back_slash);
        
        int i = 0;
        
        while(i<(int)comps.size())
        {
            if(comps[i].empty() || comps[i] == dot)
            {
                comps.erase(comps.begin()+i);
            }
            else if(comps[i] == double_dot)
            {
                if(i>0 && comps[i-1] != double_dot)
                {
                    comps.erase(comps.begin()+i-1, comps.begin()+i+1);
                    i -= 1;
                }
                else if(i == 0 && pystring::endswith(prefix, double_back_slash))
                {
                    comps.erase(comps.begin()+i);
                }
                else
                {
                    i += 1;
                }
            }
            else
            {
                i += 1;
            }
        }
        
        // If the path is now empty, substitute '.'
        if(prefix.empty() && comps.empty())
        {
            comps.push_back(dot);
        }
        
        return prefix + pystring::join(double_back_slash, comps);
    }

    // Normalize a path, e.g. A//B, A/./B and A/foo/../B all become A/B.
    // It should be understood that this may change the meaning of the path
    // if it contains symbolic links!
    // Normalize path, eliminating double slashes, etc.

    std::string normpath_posix(std::string_view p)
    {
        if(p.empty()) return dot;
        
        std::string path = std::string(p);
        
        int initial_slashes = pystring::startswith(path, forward_slash) ? 1 : 0;
        
        // POSIX allows one or two initial slashes, but treats three or more
        // as single slash.
        
        if (initial_slashes && pystring::startswith(path, double_forward_slash)
            && !pystring::startswith(path, triple_forward_slash))
            initial_slashes = 2;
        
        std::vector<std::string> comps, new_comps;
        pystring::split(path, comps, forward_slash);
        
        for(unsigned int i=0; i<comps.size(); ++i)
        {
            std::string comp = comps[i];
            if(comp.empty() || comp == dot)
                continue;
            
            if( (comp != double_dot) || ((initial_slashes == 0) && new_comps.empty()) ||
                (!new_comps.empty() && new_comps[new_comps.size()-1] == double_dot))
            {
                new_comps.push_back(comp);
            }
            else if (!new_comps.empty())
            {
                new_comps.pop_back();
            }
        }
        
        path = pystring::join(forward_slash, new_comps);
        
        if (initial_slashes > 0)
            path = pystring::mul(forward_slash, initial_slashes) + path;
        
        if(path.empty()) return dot;
        return path;
    }
    
    std::string normpath(std::string_view path)
    {
#ifdef WINDOWS
        return normpath_nt(path);
#else
        return normpath_posix(path);
#endif
    }

    //////////////////////////////////////////////////////////////////////////////////////////////
    ///
    ///

    // Split the extension from a pathname.
    // Extension is everything from the last dot to the end, ignoring
    // leading dots.  Returns "(root, ext)"; ext may be empty.
    // It is always true that root + ext == p

    void splitext_generic(std::string & root, std::string & ext,
                          std::string_view p,
                          std::string_view sep,
                          std::string_view altsep,
                          std::string_view extsep)
    {
        int sepIndex = pystring::rfind(p, sep);
        if(!altsep.empty())
        {
            int altsepIndex = pystring::rfind(p, altsep);
            sepIndex = std::max(sepIndex, altsepIndex);
        }

        int dotIndex = pystring::rfind(p, extsep);
        if(dotIndex > sepIndex)
        {
            // Skip all leading dots
            int filenameIndex = sepIndex + 1;

            while(filenameIndex < dotIndex)
            {
                if(pystring::slice(p,filenameIndex) != extsep)
                {
                    root = pystring::slice(p, 0, dotIndex);
                    ext = pystring::slice(p, dotIndex);
                    return;
                }

                filenameIndex += 1;
            }
        }

        root = p;
        ext = empty_string;
    }

    void splitext_nt(std::string & root, std::string & ext, std::string_view path)
    {
        return splitext_generic(root, ext, path,
                                double_back_slash, forward_slash, dot);
    }

    void splitext_posix(std::string & root, std::string & ext, std::string_view path)
    {
        return splitext_generic(root, ext, path,
                                forward_slash, empty_string, dot);
    }

    void splitext(std::string & root, std::string & ext, std::string_view path)
    {
#ifdef WINDOWS
        return splitext_nt(root, ext, path);
#else
        return splitext_posix(root, ext, path);
#endif
    }

} // namespace path
} // namespace os


}//namespace pystring


