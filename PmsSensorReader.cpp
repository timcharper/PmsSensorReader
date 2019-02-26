/*
  Reader class for PMS3003 sensor.
  
  Copyright (c) 2019 Tim Harper

  https://github.com/timcharper

  Permission is hereby granted, free of charge, to any person obtaining a copy
  of this software and associated documentation files (the "Software"), to deal
  in the Software without restriction, including without limitation the rights
  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
  copies of the Software, and to permit persons to whom the Software is
  furnished to do so, subject to the following conditions:

  The above copyright notice and this permission notice shall be included in all
  copies or substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
  SOFTWARE.
*/

#include "PmsSensorReader.h"
#include <Arduino.h>

// PMS-5003: http://www.aqmd.gov/docs/default-source/aq-spec/resources-page/plantower-pms5003-manual_v2-3.pdf
// PMS-3003: https://github.com/avaldebe/AQmon/blob/master/Documents/PMS3003_LOGOELE.pdf

unsigned short readShort(unsigned char bytes[]) {
  unsigned short result = *bytes;
  result = result << 8;
  result += *(bytes + 1);
  return result;
}


PmsSensorReader::PmsSensorReader() {
  this->packetLength = 0;
  this->packetIndex = 0;
  this->pm1 = 0;
  this->pm2_5 = 0;
  this->pm10 = 0;
  this->qty_pm0_3_dl = 0;
  this->qty_pm0_5_dl = 0;
  this->qty_pm1_dl = 0;
  this->qty_pm2_5_dl = 0;
  this->qty_pm5_dl = 0;
  this->qty_pm10_dl = 0;
}

PmsSensorReaderResult PmsSensorReader::offer(unsigned char value) {
  this->buffer[this->packetIndex] = value;
  if (this->packetIndex == 0) {
    if(value == 0x42) {
      // reset
      this->packetLength = 0;
      this->packetIndex = 1;
    }
    return resultPending;
  } else if (this->packetIndex == 1) {
    if (value == 0x4d) {
      this->packetIndex = 2;
      return resultPending;
    } else {
      this->packetIndex = 0;
      return resultPending;
    }
  } else if (this->packetIndex == 2) {
    this->packetIndex = 3;
    return resultPending;
  } else if (this->packetIndex == 3) {
    this->packetLength = readShort(this->buffer + 2) + 4; // Plus the 4 we've already read
    if ((this->packetLength) > (sizeof(this->buffer))) {
      this->packetIndex = 0;
      return frameTooLarge;
    } else {
      this->packetIndex = 4;
      return resultPending;
    }
  } else if (this->packetIndex > (this->packetLength - 1)) {
    this->packetIndex = 0;
    return invalidFrameLength;
  } else if (this->packetIndex == (this->packetLength - 1)) {
    this->packetIndex = 0;
    return this->_measure();
  } else {
    this->packetIndex ++;
    return resultPending;
  }
}


PmsSensorReaderResult PmsSensorReader::_measure() {
  unsigned short checksum = readShort(this->buffer + (this->packetLength - 2));
  unsigned short sum = 0;
  for (int i = 0; i < this->packetLength - 2; i++) {
    sum += this->buffer[i];
  }
  if (sum != checksum) {
    return checksumFailed;
  }
  if (this->packetLength == 32) {
    this->sensorType = Pms5003;
  } else {
    this->sensorType = Pms3003;
  }
  this->pm1 = readShort(this->buffer + 4);
  this->pm2_5 = readShort(this->buffer + 6);
  this->pm10 = readShort(this->buffer + 8);
  /* +10, +12, +14 are the previous measurements "under atmospheric environment". IE the same. */

  if (this->sensorType == Pms5003) {
    this->qty_pm0_3_dl = readShort(this->buffer+16);
    this->qty_pm0_5_dl = readShort(this->buffer+18);
    this->qty_pm1_dl = readShort(this->buffer+20);
    this->qty_pm2_5_dl = readShort(this->buffer+22);
    this->qty_pm5_dl = readShort(this->buffer+24);
    this->qty_pm10_dl = readShort(this->buffer+26);
  }

  return success;
}
