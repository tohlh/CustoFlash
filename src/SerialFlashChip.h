/* SerialFlash Library - for filesystem-like access to SPI Serial Flash memory
* https://github.com/PaulStoffregen/SerialFlash
* Copyright (C) 2015, Paul Stoffregen, paul@pjrc.com
*
* Development of this library was funded by PJRC.COM, LLC by sales of Teensy.
* Please support PJRC's efforts to develop open source software by purchasing
* Teensy or other genuine PJRC products.
*
* Permission is hereby granted, free of charge, to any person obtaining a copy
* of this software and associated documentation files (the "Software"), to deal
* in the Software without restriction, including without limitation the rights
* to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
* copies of the Software, and to permit persons to whom the Software is
* furnished to do so, subject to the following conditions:
*
* The above copyright notice, development funding notice, and this permission
* notice shall be included in all copies or substantial portions of the Software.
*
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
* IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
* FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
* AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
* LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
* OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
* THE SOFTWARE.
*/


#include <Arduino.h>
#include <SPI.h>

class SerialFlashChip
{
public:
	static bool begin(SPIClass& device, uint8_t pin = 6);
	static bool begin(uint8_t pin = 6);
	static uint32_t capacity(const uint8_t *id);
	static uint32_t blockSize();
	static void sleep();
	static void wakeup();
	static void readID(uint8_t *buf);
	static void readSerialNumber(uint8_t *buf);
	static void read(uint32_t addr, void *buf, uint32_t len);
	static bool ready();
	static void wait();
	static void write(uint32_t addr, const void *buf, uint32_t len);
	static void eraseAll();
	static void eraseBlock(uint32_t addr);
	static void eraseSector(uint32_t addr);
private:
	static uint8_t flags;	// chip features
	static uint8_t busy;
	// 0 = ready
	// 1 = suspendable program operation
	// 2 = suspendable erase operation
	// 3 = busy for realz!!
};
