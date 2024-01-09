/**
 *
 * Copyright (c) 2020-2022 IRbaby-IRext
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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <MD5Builder.h>

#include "IRISKitUtils.h"

// public variable definitions
MD5Builder md5_builder;

int split_string(const String source, char* parts[], const char* delimiter) {
    char* pch = NULL;
    char* copy = NULL;
    char* tmp = NULL;
    int i = 0;

    copy = strdup(source.c_str());
    if (NULL == copy) {
        goto exit;
    }

    pch = strtok(copy, delimiter);

    tmp = strdup(pch);
    if (NULL == tmp) {
        goto exit;
    }

    parts[i++] = tmp;

    while (pch) {
        pch = strtok(NULL, delimiter);
        if (NULL == pch) break;

        tmp = strdup(pch);
        if (NULL == tmp) {
            goto exit;
        }

        parts[i++] = tmp;
    }

    free(copy);
    return i;

exit:
    free(copy);
    for (int j = 0; j < i; j++) {
        free(parts[j]);
    }
    return -1;
}

String md5(String str) {
    md5_builder.begin();
    md5_builder.add(String(str));
    md5_builder.calculate();
    return md5_builder.toString();
}