
#pragma once

class threaded
{
private:
	friend void * thread_proc(void *);

    pthread_t m_thread;
    sem_t m_run_flag;
protected:
	virtual void run() = 0;

    std::string m_log_cat;

public:
	threaded();
	virtual ~threaded();

	virtual int start();
	virtual int stop();
    int still_running();
};
