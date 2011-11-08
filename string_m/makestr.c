/*
 *
 * Copyright (c) 2005 Carnegie Mellon University.
 * All rights reserved.

 * Permission to use this software and its documentation for any purpose is hereby granted, 
 * provided that the above copyright notice appear and that both that copyright notice and 
 * this permission notice appear in supporting documentation, and that the name of CMU not 
 * be used in advertising or publicity pertaining to distribution of the software without 
 * specific, written prior permission.
 * 
 * CMU DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES 
 * OF MERCHANTABILITY AND FITNESS, IN NO EVENT SHALL CMU BE LIABLE FOR ANY SPECIAL, INDIRECT OR 
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, 
 * WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, RISING OUT OF OR IN 
 * CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE. 
 */
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <wchar.h>

#include "string_m.h"
#include "string_m_internals.h"

//  internal routine used to allocate memory for string
errno_t makestr(string_m s, const char *cstr){
    size_t len;

	if (!s){
		ErrorHandler("makestr: 1st Argument NULL ptr", s, EINVAL);	
		return EINVAL;
	}

    s->strtype = STRTYPE_WSTR;
    if (!cstr) {
        s->size = 0;
        s->str.cstr = NULL;
        return 0;
    }

    len = strlen(cstr) + 1;

    // allocate lengths in multiples of 16
    if (len % 16 > 0) {
        len = ((len>>4) + 1)<<4;
    }

    s->size = len * sizeof(char);
    s->strtype = STRTYPE_NTBS;
    s->str.cstr = (char *)calloc(1, s->size);
    if (!s->str.cstr) {
        s->size = 0;
        return 0;
    }

    memcpy(s->str.cstr, cstr, len * sizeof(char)); 
    s->str.cstr[len-1] = '\0';

    s->charset.cstr = NULL;
    s->maxsize = 0;

    return 0;
}


//  internal routine used to allocate memory for string
errno_t wmakestr(string_m s, const wchar_t *wstr){
    size_t len;

	if (!s){
		ErrorHandler("wmakestr: 1st Argument NULL pointer", s, EINVAL);
		return EINVAL;
	}

    s->strtype = STRTYPE_WSTR;
    if (!wstr) {
        s->size = 0;
        s->str.wstr = NULL;
        return 0;
    }

    len = wcslen(wstr) + 1;

    // allocate lengths in multiples of 16
    if (len % 16 > 0) {
        len = ((len>>4) + 1)<<4;
    }

    s->size = len * sizeof(wchar_t);
    s->strtype = STRTYPE_WSTR;
    s->str.wstr = (wchar_t *)calloc(1, s->size);
    if (!s->str.wstr) {
        s->size = 0;
        return ENOMEM;
    }

    memcpy(s->str.wstr, wstr, len * sizeof(wchar_t)); 
    s->str.wstr[len-1] = 0;

    s->charset.cstr = NULL;
    s->maxsize = 0;

    return 0;
}

// routine used to convert a managed string's internal storage to wide string
errno_t
str2wstr_m(string_m s) {
	wchar_t *nstr;
	wchar_t *ncharset;
	size_t rv;


	if (!s) return EINVAL;
	if (s->strtype == STRTYPE_WSTR) return 0;
	if (s->strtype != STRTYPE_NTBS){
		ErrorHandler("str2wstr_m: 1st Argument Invalid String Type", s, EINVAL); 
		return EINVAL;
	}

	if (s->str.cstr == NULL) {
		nstr = NULL;
	} else {
		nstr = (wchar_t*)calloc(s->size, sizeof(wchar_t));
		if (!nstr) {
			ErrorHandler("str2wstr_m: Calloc Error", nstr, ENOMEM);
			return ENOMEM;
		}
	
		rv = mbstowcs(nstr,s->str.cstr, s->size);
		if (rv == -1) {
			free(nstr);
			return errno;
		}
	}

	if (s->charset.cstr == NULL) {
		ncharset = NULL;
	} else {
		size_t len = strlen(s->charset.cstr);
		ncharset = (wchar_t*)calloc(len, sizeof(wchar_t));
		if (!ncharset) {
			free(nstr);
			ErrorHandler("str2wstr_m: Calloc Error", ncharset, ENOMEM);
			return ENOMEM;
		}
	
		rv = mbstowcs( ncharset, s->charset.cstr, len);
		if (rv == -1) {
			free(ncharset);
			free(nstr);
			return errno;
		}
	}

	if (s->str.cstr) free(s->str.cstr);
	s->str.wstr = nstr;
	if (s->charset.cstr) free(s->charset.cstr);
	s->charset.wstr = ncharset;

	s->strtype = STRTYPE_WSTR;

	return 0;
}


errno_t
wsetcharset_m(string_m s, const wchar_t *charset){
	size_t size;
	int rv;

	if(s->strtype == STRTYPE_NTBS){
		rv = str2wstr_m(s);
		if(rv != 0) ERROR(rv);
	}

	if(s->strtype != STRTYPE_WSTR){
		ErrorHandler("setcharset_m: 1st Argument Invalid String Type", s, EINVAL);
		ERROR(EINVAL);
	}

	if(!s->size){
	  s->charset.wstr = wcsdup(charset);
		return 0;
	}

	rv = strlen_m(s, &size);
	if(rv != 0) ERROR(EINVAL);

	s->charset.wstr = wcsdup(charset);

	if( (charset) && (wcsspn(s->str.wstr, charset) < size)){
		ErrorHandler("setcharset_m: Charset Invalidates String", wcsdup(charset), EINVAL);
		ERROR(EINVAL);
	}

	return 0;
}

errno_t
csetcharset_m(string_m s, const char *charset){
	size_t size;
	int rv;

	if(s->strtype == STRTYPE_WSTR){
		wchar_t *t = (wchar_t *)calloc(strlen(charset),sizeof(wchar_t));
		if(!t) {
			ErrorHandler("csetcharset_m: Calloc Error", t, ENOMEM);
			ERROR(ENOMEM);
		}
		rv = wsetcharset_m(s, t);
		free(t);
		if(rv != 0) ERROR(rv);
		return rv;
	}

	if(s->strtype != STRTYPE_NTBS){
		ErrorHandler("csetcharset_m: 1st Argument Invalid String Type", s, EINVAL);
		ERROR(EINVAL);
	}

	if(!s->size){
		s->charset.cstr = strdup(charset);
		return 0;
	}

	rv = strlen_m(s, &size);
	if(rv != 0) ERROR(EINVAL);

	s->charset.cstr = strdup(charset);

	if( (charset) && (strspn(s->str.cstr, charset) != size)){
		ErrorHandler("setcharset_m: Charset Invalidates String", strdup(charset), EINVAL);
		ERROR(EINVAL);
	}

	return 0;
}

errno_t
setcharset_m(string_m s, const string_m charset){
	if(!s){
		ErrorHandler("setcharset_m: 1st Argument NULL Pointer", s, EINVAL);
		ERROR(EINVAL);
	}

	if(!charset || !(charset->size)){
		if(s->strtype == STRTYPE_NTBS)
			s->charset.cstr = NULL;
		else if(s->strtype == STRTYPE_WSTR)
			s->charset.wstr = NULL;
		else{
			ErrorHandler("setcharset_m: 1st Argument Invalid String Type", s, EINVAL);
			ERROR(EINVAL);
		}
		return 0;
	}

	if(charset->strtype == STRTYPE_NTBS)
		return csetcharset_m(s, charset->str.cstr);
	else if(charset->strtype == STRTYPE_WSTR)
		return wsetcharset_m(s, charset->str.wstr);
	else{
		ErrorHandler("setcharset_m: 2nd Argument Invalid String Type", charset, EINVAL);
		ERROR(EINVAL);
	}
}

errno_t
setmaxlen_m(string_m s, rsize_t maxlen){
	if(!s){
		ErrorHandler("setmaxlen_m: 1st Argument NULL Pointer", s, EINVAL);
		ERROR(EINVAL);
	}

	if(s->size > maxlen){
		ErrorHandler("setmaxlen_m: Size Greater Than Maxsize", s, EINVAL);
		ERROR(EINVAL);
	}

	if(maxlen == 0){
		s->maxsize = BUFSIZ-1;
	} else {
		s->maxsize = maxlen;
	}



	return 0;
}
