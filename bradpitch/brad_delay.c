
#include "brad_delay.h"
#include "definebrad.h"
#include "brad_input.h"

void setupDelayParams(DelayParams *this, fixedp *buffer, Uint32 bufferSize) {
	this->wp = 0;
	this->rp = 0;
	this->inc = Q1;
	this->fb = float2q(0.8f);

	this->useExternalFeedback = 0;
	this->externalFbSample = 0;
	this->currentFbSample = 0;
	
	this->wet = Q0_5;
	this->dry = Q0_5;

	this->delayInSamples = 0;
	this->buffer = buffer;
	this->bufferSize = bufferSize;

	this->active = 0;

	memset(buffer, 0, sizeof(fixedp)*bufferSize);
}

// Utf�r en delay. Just nu fixed p� 8000 samples (1s, 8khz samplerate)
void process_delay(DelayParams *this, fixedp *process, Uint32 processSize) {
	int i;
	fixedp out, in;
	int rpi; // integer part
	fixedp frac; // fractional part
	fixedp next; // next value in case of wrapping
	fixedp result;
	
	fixedp feedbackSample;

	for(i = 0; i < processSize; i++) {
		in = process[i];
		
		rpi = (int)qipart(this->rp);
		frac = qfpart(this->rp);
		next = rpi + 1 != this->bufferSize ? this->buffer[rpi+1] : this->buffer[0];
		
		out = this->buffer[rpi];

		// out = out + frac * (next - out) (linear interp)
		result = qadd(out, qmul(frac, qsub(next, out))); 
		
		// in i buffern (in + ut*feedback)
		if(this->useExternalFeedback == 0) {
			this->buffer[this->wp] = qadd(in, qmul(result, this->fb));
		}
		else {
			this->buffer[this->wp] = qadd(in, qmul(this->externalFbSample, this->fb)); // + delayP.fb*result ;
		}
		

		// Summera och sl�ng ut! (ut * wet + in * dry)
		process[i] = qadd(qmul(result,this->wet), qmul(in,this->dry));
		
		// Increase read pointer and wrap around
		this->rp = qadd(this->rp, this->inc); 
		rpi = (int)qipart(this->rp);
		if(rpi >= this->bufferSize) this->rp = qsub(this->rp, this->bufferSize << FIXED_FRACBITS);

		// Increase write pointer and wrap around
		this->wp += 1;
		if(this->wp >= this->bufferSize) this->wp = 0;
	}
	return;
}

void delay_setParam(DelayParams *this, Uint32 param, int val) {
	switch(param) {
	case DELAY_ACTIVE:
		this->active = val ? 1 : 0;
		break;
	case DELAY_TIME:
		delay_setDelayTime(this, val);
		break;
	case DELAY_MIX:
		delay_setMix(this, val);
		break;
	case DELAY_FDB:
		delay_setFb(this, val);
		break;
	}
}

void delay_setDelayTime(DelayParams *this, int delayInSamples) {
	// subtract to make read index
	this->rp = int2q(this->wp - delayInSamples); // cast as int!

	// check and wrap BACKWARDS if the index is negative
	if( this->rp < 0 )
		this->rp += int2q(this->bufferSize);
}

void delay_setMix(DelayParams *this, int mix) {
	this->wet = mix;

	if(this->wet > AUDIOMAX) this->wet = AUDIOMAX;
	else if(this->wet < 0) this->wet = 0;

	this->dry = (Q1 - 1) - this->wet;
}

void delay_setFb(DelayParams *this, int fb) {

	this->fb = fb;
	if(this->fb > Q1) this->fb = Q1;
	else if(this->fb < -Q1) this->fb = -Q1;

}