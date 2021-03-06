/*
 * SGTL5000 Recorder for Teensy 
 * WMXZ 2019
 */
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
// "mRecordQueue <53> queue1;"
// data are returned as void *, so "readBuffer" needs cast to proper type as defined in AudioStream
 
#ifndef M_QUEUE_H
#define M_QUEUE_H

#include "mAudioStream.h"

//#define MQ 53
template <int MQ>
class mRecordQueue : public mAudioStream
{
public:
	mRecordQueue(void) : mAudioStream(1, inputQueueArray),
		userblock(NULL), head(0), tail(0), enabled(0) { }
   
	void begin(void) {  clear();	 enabled = 1;	}
    void end(void) { enabled = 0; }
	int available(void);
	void clear(void);
	void * readBuffer(void);
	void freeBuffer(void);
	virtual void update(void);

	uint32_t dropCount=0;

private:
	maudio_block_t *inputQueueArray[1];
	maudio_block_t * volatile queue[MQ];
	maudio_block_t *userblock;
	volatile uint16_t head, tail, enabled;

  //temp variables
  uint16_t h, t;

};

template <int MQ>
int mRecordQueue<MQ>::available(void)
{
	h = head;
	t = tail;
	if (h >= t) return h - t;
	return MQ + h - t;
}

template <int MQ>
void mRecordQueue<MQ>::clear(void)
{
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

template <int MQ>
void * mRecordQueue<MQ>::readBuffer(void)
{
	if (userblock) return NULL;
	t = tail;
	if (t == head) return NULL;
	if (++t >= MQ) t = 0;
	userblock = queue[t];
	tail = t;
	return (void *) userblock->data;
}

template <int MQ>
void mRecordQueue<MQ>::freeBuffer(void)
{
	if (userblock == NULL) return;
	release(userblock);
	userblock = NULL;
}

template <int MQ>
void mRecordQueue<MQ>::update(void)
{
	maudio_block_t *block;

	block = receiveReadOnly();
	if (!block) return;
	if (!enabled) {
		release(block);
		return;
	}
	h = head + 1;
	if (h >= MQ) h = 0;
	if (h == tail) {
		dropCount++;
		release(block);
	} else {
		queue[h] = block;
		head = h;
	}
}

#endif
