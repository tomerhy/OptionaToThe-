
#pragma once

class dreamer : public informer
{
public:
	dreamer();
	virtual ~dreamer();

	virtual int init(const informer_conf * conf);
	virtual int start();
	virtual int stop();
};
