/**
 *
 * Copyright (c) 2020-2021 IRbaby-IRext
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

#include <WiFiUdp.h>
#include <ESP8266WiFi.h>
#include "IRbabyUDP.h"
#include "IRbabySerial.h"

WiFiUDP UDP;
IPAddress remote_ip;

char incomingPacket[UDP_PACKET_SIZE]; // buffer for incoming packets

void udpInit()
{
    UDP.begin(UDP_PORT);
    DEBUGF("Now listening at IP %s, UDP port %d\n", WiFi.localIP().toString().c_str(), UDP_PORT);
}

char *udpRecive()
{
    int packetSize = UDP.parsePacket();
    if (packetSize)
    {
        /* receive incoming UDP packets */
        DEBUGF("Received %d bytes from %s, port %d\n", packetSize, UDP.remoteIP().toString().c_str(), UDP.remotePort());
        int len = UDP.read(incomingPacket, 255);
        if (len > 0)
        {
            incomingPacket[len] = 0;
        }
        return incomingPacket;
    }
    return nullptr;
}

uint32_t sendUDP(StaticJsonDocument<1024>* doc, IPAddress ip)
{
    DEBUGF("return message %s\n", ip.toString().c_str());
    UDP.beginPacket(ip, UDP_PORT);
    serializeJson(*doc, UDP);
    return UDP.endPacket();
}

uint32_t returnUDP(StaticJsonDocument<1024>* doc)
{
    DEBUGF("return message to %s\n", UDP.remoteIP().toString().c_str());
    UDP.beginPacket(UDP.remoteIP(), UDP_PORT);
    serializeJson(*doc, UDP);
    return UDP.endPacket();
}