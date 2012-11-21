#include <iostream>
#include <assert.h>
#include "pulseStateMachine.h"
#include "string.h"

using std::cout;
using std::endl;

void runPulseChannelTests() {
    // should be off by default
    {
        PulseChannel p;
        assert(p.on() == false);
        assert(p.onTime() == 0);
        //assert(p.offTime() == 0); // no off pulse to finish
        p.advanceTime(1);
        assert(p.on() == false);
        p.advanceTime(1);
        assert(p.on() == false);
    }

    // should be able to set on and off times, resulting in appropriate on and
    // off intervals.
    {
        PulseChannel p;
        p.setOnOffTime(20, 50);
        assert(p.on() == true);
        p.advanceTime(19);
        assert(p.on() == true);
        p.advanceTime(1);
        assert(p.on() == false);
        p.advanceTime(49);
        assert(p.on() == false);
        p.advanceTime(1);
        assert(p.on() == true);
        p.advanceTime(19);
        assert(p.on() == true);
        p.advanceTime(1);
        assert(p.on() == false);
        p.advanceTime(49);
        assert(p.on() == false);
        p.advanceTime(1);
        assert(p.on() == true);
    }

    // setting 0 for on time should turn off and leave off
    {
        PulseChannel p;
        p.setOnOffTime(0, 10);
        assert(p.on() == false);
        p.setOnOffTime(20, 50);
        assert(p.on() == true);
        p.setOnOffTime(0, 10);
        assert(p.on() == false);
    }

    // setting 0 for on time should turn on and leave on
    {
        PulseChannel p;
        p.setOnOffTime(10, 0);
        assert(p.on() == true);
        p.setOnOffTime(20, 50);
        assert(p.on() == true);
        p.setOnOffTime(10, 0);
        assert(p.on() == true);
    }

    // should correctly compute time until next state change
    {
        PulseChannel p;
        p.setOnOffTime(20, 50);
        assert(p.timeUntilNextStateChange() == 20);
        p.advanceTime(11);
        assert(p.timeUntilNextStateChange() == 9);
        p.advanceTime(9);
        assert(p.timeUntilNextStateChange() == 50);
        p.advanceTime(23);
        assert(p.timeUntilNextStateChange() == 27);
        p.advanceTime(27);
        assert(p.timeUntilNextStateChange() == 20);
    }
}


void runPulseStateCommandParsingTests() {
    // Basic parsing tests
    {
        PulseStateCommand c;
        const char* error;
        c.parseFromString("end", &error);
        assert(error == NULL);
        assert(c.type == PulseStateCommand::end);
    }
    {
        PulseStateCommand c;
        const char* error;
        // us and Latin-1 micro
        c.parseFromString("change channel 3 to repeat 213 us on 182 \xB5s off", &error);
        assert(error == NULL);
        assert(c.type == PulseStateCommand::setChannel);
        assert(c.channel == 3);
        assert(c.onTime == 213);
        assert(c.offTime == 182);
    }
    {
        PulseStateCommand c;
        const char* error;
        c.parseFromString("change channel 1 to repeat 12 ms on 18 s off", &error);
        assert(error == NULL);
        assert(c.type == PulseStateCommand::setChannel);
        assert(c.channel == 1);
        assert(c.onTime == 12000);
        assert(c.offTime == 18000000UL);
    }
    {
        PulseStateCommand c;
        const char* error;
        // utf-8 micro and utf-8 greek lowercase mu
        c.parseFromString("change  channel  2  to  repeat  13  \u00B5s  on  23  \u03BCs  off", &error);
        assert(error == NULL);
        assert(c.type == PulseStateCommand::setChannel);
        assert(c.channel == 2);
        assert(c.onTime == 13);
        assert(c.offTime == 23);
    }
    {
        PulseStateCommand c;
        const char* error;
        c.parseFromString("turn off channel 4", &error);
        assert(error == NULL);
        assert(c.type == PulseStateCommand::setChannel);
        assert(c.channel == 4);
        assert(c.onTime == 0);
        assert(c.offTime == forever);
    }
    {
        PulseStateCommand c;
        const char* error;
        c.parseFromString("turn on channel 2", &error);
        assert(error == NULL);
        assert(c.type == PulseStateCommand::setChannel);
        assert(c.channel == 2);
        assert(c.onTime == forever);
        assert(c.offTime == 0);
    }
    {
        PulseStateCommand c;
        const char* error;
        c.parseFromString("wait 273 ms", &error);
        assert(error == NULL);
        assert(c.type == PulseStateCommand::wait);
        assert(c.waitTime == 273000);
    }

    // Error tests
    {
        PulseStateCommand c;
        const char* error;
        assert(numChannels == 4);
        c.parseFromString("turn off channel 5", &error);
        assert(error && strcmp(error, "Channel number must be between 1 and 4") == 0);
    }
    {
        PulseStateCommand c;
        const char* error;
        c.parseFromString("change channel 0 to repeat 12 ms on 18 s off", &error);
        assert(error && strcmp(error, "Channel number must be between 1 and 4") == 0);
    }
    // TODO: tests for other error messages.

    // cout << "Error: " << (error ? error : "none") << endl;
}


void runPulseStateCommandExecuteTests() {
    bool completed;

    // should be able to run commands
    {
        PulseStateCommand c;
        Microseconds remainingTime = 100;

        c.type = PulseStateCommand::end;

        completed = c.execute(NULL, 0, &remainingTime);
        assert(remainingTime == 0);
        assert(!completed);

        remainingTime = 100;
        completed = c.execute(NULL, 100, &remainingTime);
        assert(remainingTime == 0);
        assert(!completed);
    }
    {
        PulseStateCommand c;
        PulseChannel p[numChannels];

        c.type = PulseStateCommand::setChannel;
        c.channel = 1;
        c.onTime = 12;
        c.offTime = 10;

        completed = c.execute(p, 0, NULL);
        assert(p[0].onTime() == 12);
        assert(p[0].offTime() == 10);
        assert(completed);
    }
    {
        PulseStateCommand c;
        Microseconds remainingTime = 100;

        c.type = PulseStateCommand::wait;
        c.waitTime = 120;

        completed = c.execute(NULL, 0, &remainingTime);
        assert(remainingTime == 0);
        assert(!completed);

        remainingTime = 70;
        completed = c.execute(NULL, 100, &remainingTime);
        assert(remainingTime == 50);
        assert(completed);
    }
}


int main() {
    cout << "running PulseChannel tests\n";
    runPulseChannelTests();
    cout << "running PulseStateCommand parsing tests\n";
    runPulseStateCommandParsingTests();
    cout << "running PulseStateCommand execute tests\n";
    runPulseStateCommandExecuteTests();
    cout << "**************** Tests passed! ****************\n";
    return 0;
}