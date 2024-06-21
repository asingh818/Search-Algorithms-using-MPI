#pragma once
// The myTimer class is implemented by using the C-based timing functions and interface to underlying hardware platform
#include <time.h> // declares clock_t type, function clock(), and constant CLOCKS_PER_SEC
class myTimer
{
private:
	// starting and ending time measured in clock ticks
	clock_t startTime, endTime;
public:
	myTimer();
	// initialize the timer.
	// Postconditions: set the starting and ending event times to 0.
	// this creates an event whose time is 0.0
	void start();
	// start timing an event.
	// Postcondition:   record the current time as the starting time
	void stop();
	// stop timing an event.
	// Postconditon:    record the current time as the ending time
	double time() const;
	// return the time the event took in seconds by computing
	// the difference between the ending and starting times.
};
// ***********************************************************
//      timer class implementation
// ***********************************************************
// constructor. set starting and ending times to 0
myTimer::myTimer() :startTime(0), endTime(0)
{}
// determine clock ticks at start
void myTimer::start()
{
	startTime = clock();
}
// determine clock ticks at end
void myTimer::stop()
{
	endTime = clock();
}
// return the elapsed time in seconds
double myTimer::time() const
{
	return (endTime - startTime) / double(CLOCKS_PER_SEC);
}