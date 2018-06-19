/* Audio Library for Teensy 3.X
 * Copyright (c) 2014, Paul Stoffregen, paul@pjrc.com
 *
 * Development of this audio library was funded by PJRC.COM, LLC by sales of
 * Teensy and Audio Adaptor boards.  Please support PJRC's efforts to develop
 * open source software by purchasing Teensy or other PJRC products.
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

// WMXZ 01-02-2018 modified to template for variable buffersize
// this routine is equivalent with stock record_queue if initiated as 
// "mRecordQueue <int16_t, 53> queue1;"
// Type T sgould be int16_t for standard AudioStream
// if using own AudioStream version i.e. modified audio_block_t definition than call with proper type

 
#ifndef M_QUEUE_H
#define M_QUEUE_H

#include "AudioStream.h"

//#define MQ 53
template <typename T, int MQ>
class mRecordQueue : public AudioStream
{
public:
	mRecordQueue(void) : AudioStream(1, inputQueueArray),
		userblock(NULL), head(0), tail(0), enabled(0) { }
   
	void begin(void) { clear();	enabled = 1;}
  void end(void) { enabled = 0; }
	int available(void);
	void clear(void);
	T * readBuffer(void);
	void freeBuffer(void);
	virtual void update(void);
private:
	audio_block_t *inputQueueArray[1];
	audio_block_t * volatile queue[MQ];
	audio_block_t *userblock;
	volatile uint16_t head, tail, enabled;
};

template <typename T, int MQ>
int mRecordQueue<T, MQ>::available(void)
{
	uint32_t h, t;

	h = head;
	t = tail;
	if (h >= t) return h - t;
	return MQ + h - t;
}

template <typename T, int MQ>
void mRecordQueue<T, MQ>::clear(void)
{
	uint32_t t;

	if (userblock) {
		release(userblock);
		userblock = NULL;
	}
	t = tail;
	while (t != head) {
		if (++t >= MQ) t = 0;
		release(queue[t]);
	}
	tail = t;
}

template <typename T, int MQ>
T * mRecordQueue<T, MQ>::readBuffer(void)
{
	uint32_t t;

	if (userblock) return NULL;
	t = tail;
	if (t == head) return NULL;
	if (++t >= MQ) t = 0;
	userblock = queue[t];
	tail = t;
	return userblock->data;
}

template <typename T, int MQ>
void mRecordQueue<T, MQ>::freeBuffer(void)
{
	if (userblock == NULL) return;
	release(userblock);
	userblock = NULL;
}

uint32_t outCount=0;

template <typename T, int MQ>
void mRecordQueue<T, MQ>::update(void)
{
	audio_block_t *block;
	uint32_t h;
	outCount++;

	block = receiveReadOnly();
	if (!block) return;
	if (!enabled) {
		release(block);
		return;
	}
	h = head + 1;
	if (h >= MQ) h = 0;
	if (h == tail) {
		release(block);
	} else {
		queue[h] = block;
		head = h;
	}
}

#endif