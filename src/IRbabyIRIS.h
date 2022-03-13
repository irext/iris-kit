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

#ifndef IRBABY_IRIS_H
#define IRBABY_IRIS_H

#define CREDENTIAL_MAX               (40)

// web http call URL list
#define FETCH_CREDENTIAL_SUFFIX      "/irext-collect/credentials/fetch_credential"
#define LOAD_ALIOT_ACCOUNT_SUFFIX    "/irext-collect/aliot/load_account"
#define DOWNLOAD_PREFIX              "http://irext-debug.oss-cn-hangzhou.aliyuncs.com/irda_"
#define DOWNLOAD_SUFFIX              ".bin"


int getIRISKitVersion(char *buffer, int buffer_size);

int getDeviceID(char* buffer, int buffer_size);

int fetchIrisCredential(String credential_token,
                        String& product_key,
                        String& device_name,
                        String& device_secret);

void sendIrisKitConnect();

void sendIrisKitHeartBeat();

#endif // IRBABY_IRIS_H