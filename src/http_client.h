/**
 *
 * Copyright (c) 2020-2026 IRext Opensource Organization
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#ifndef IRIS_KIT_HTTP_H
#define IRIS_KIT_HTTP_H

#include <WString.h>

#define URL_SHORT_MAX   (128)

typedef enum {
    HTTP_ERROR_SUCCESS = 0,
    HTTP_ERROR_RESPONSE = 1,
    HTTP_ERROR_PAYLOAD = 2,
    HTTP_ERROR_BUSINESS = 3,
    HTTP_ERROR_LOCAL_SPACE = 4,
    HTTP_ERROR_GENERIC = 7,
    HTTP_ERROR_MAX = 15,
} http_error_t;


// public function export
http_error_t httpPost(String url, String request_data, String& result);

http_error_t downLoadFile(String url, String file, String path);


#endif // IRIS_KIT_HTTP_H