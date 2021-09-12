/*@
Copyright (c) 2013-2021, Su Zhenyu steven.known@gmail.com
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

    * Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright
      notice, this list of conditions and the following disclaimer in the
      documentation and/or other materials provided with the distribution.
    * Neither the name of the Su Zhenyu nor the names of its contributors
      may be used to endorse or promote products derived from this software
      without specific prior written permission.

THIS SOFTWARE IS PROVIDED "AS IS" AND ANY
EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE
USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
@*/
#ifndef _STRBUF_H_
#define _STRBUF_H_

namespace xcom {

class StrBuf {
    COPY_CONSTRUCTOR(StrBuf);
public:
    CHAR * buf; //the buffer that holds the string.
    UINT buflen; //the byte length of the buffer. Note the buffer may longer
                 //than the string needed.
public:
    //initsize: the byte size that string buffer expected to be.
    //Note the buffer may be longer than a string needed.
    StrBuf(UINT initsize)
    {
        ASSERT0(initsize > 0);
        buflen = initsize;
        buf = (CHAR*)::malloc(initsize);
        buf[0] = 0;
    }
    ~StrBuf()
    {
        if (buf != nullptr) {
            ::free(buf);
        }
    }

    void clean()
    {
        ASSERT0(buf);
        buf[0] = 0;
    }
    void copy(StrBuf const& src)
    {
        if (buflen < src.buflen) {
            buflen = src.buflen;            
            ASSERT0(buf);
            ::free(buf);
            buf = (CHAR*)::malloc(buflen);            
        }
        buf[0] = 0;
        ASSERT0(src.buf);
        ::memcpy(buf, src.buf, buflen);
    }

    //String comparation.
    //Return true if s equal to current string.
    bool is_equal(CHAR const* s) const { return ::strcmp(buf, s) == 0; }

    //Return true if string is empty.
    bool is_empty() const { return buf == nullptr || buf[0] == 0; }

    //Composes a string that formed by 'format'.
    void sprint(CHAR const* format, ...);

    //This function print string according to 'format'.
    //args: a list of argument store in stack.
    void vsprint(CHAR const* format, va_list args);

    //Concatenate original string and new strings.
    //Appends a copy of the source string to the current string buffer,
    //the new string is consist of original string and the string formed
    //by 'format'.
    void strcat(CHAR const* format, ...);

    //Concatenate original string and new strings.
    //Appends a copy of the source string to the current string buffer,
    //the new string is consist of original string and the string formed
    //by 'format'.
    //size: the maximum possible byte size of string.
    void strcat(UINT l, CHAR const* format, va_list args);

    //Return byte size of current string.
    //Note the size does NOT include the end-character '\0'.
    size_t strlen() const { return ::strlen(buf); }

    //The functions snprintf() and vsnprintf() do not write more than size
    //bytes (including the terminating null byte ('\0')).
    //size: the maximum possible byte size of string.
    void nstrcat(UINT size, CHAR const* format, ...);

    //Concatenate original string and new strings.
    //Appends a copy of the source string to the current string buffer,
    //the new string is consist of original string and the string formed
    //by 'args'.
    void vstrcat(CHAR const* format, va_list args);
};

} //namespace xcom
#endif
