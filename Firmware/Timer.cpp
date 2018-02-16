#include "Timer.h"
#include "Console.h"

using namespace Systems;

Timer Systems::timer;

#define TIMER2_PRESCALER (8)
#define TIMER2_RESOLUTION ((1<<TIMER2_PRESCALER)/16) // TimerTick = 16M/2^8 = 16 us
// Note: at 16us per tick, the longest delay that can be requested is roughly 1s

/// <summary>
/// Method used by clients to request timer callbacks at specific intervals
/// </summary>
/// <param name="resolutionInMicroSeconds">The callback period</param>
/// <param name="client">The callback</param>
void Timer::hook(int resolutionInMicroSeconds, Timer::ClientMethod client, void* param)
{
	if (count < 4)
	{
		auto& clientInfo = clients[count];
		clientInfo.callback = client;
		clientInfo.param = param;
		clientInfo.ticks = resolutionInMicroSeconds / TIMER2_RESOLUTION;
		NRF_TIMER2->CC[count] = clientInfo.ticks;
		NRF_TIMER2->INTENSET = TIMER_INTENSET_COMPARE0_Enabled << (TIMER_INTENSET_COMPARE0_Pos + count);
		//NRF_TIMER2->SHORTS |= (TIMER_SHORTS_COMPARE0_STOP_Enabled << (TIMER_SHORTS_COMPARE0_STOP_Pos + count));
		count++;
	}
	else
	{
		console.println("Too many timer hooks registered.");
	}
}

/// <summary>
/// Method used by clients to stop getting timer callbacks
/// </summary>
/// <param name="client">the method to unregister</param>
void Timer::unHook(Timer::ClientMethod client)
{
	int clientIndex = 0;
	for (; clientIndex < 4; ++clientIndex)
	{
		if (clients[clientIndex].callback == client)
		{
			break;
		}
	}

	if (clientIndex != 4)
	{
		// Clear the entry and timer entry
		NRF_TIMER2->CC[clientIndex] = 0;
		auto& clientInfo = clients[clientIndex];
		clientInfo.callback = nullptr;
		clientInfo.param = nullptr;
		clientInfo.ticks = 0;

		// Shift entries down
		for (; clientIndex < count-1; ++clientIndex)
		{
			clients[clientIndex] = clients[clientIndex + 1];
			NRF_TIMER2->CC[clientIndex] = NRF_TIMER2->CC[clientIndex + 1];
		}

		// Decrement total count
		count--;

		// Disable compare event for the last client
		NRF_TIMER2->INTENCLR = TIMER_INTENSET_COMPARE0_Enabled << (TIMER_INTENSET_COMPARE0_Pos + count);  // taken from Nordic dev zone
	}
	else
	{
		console.println("Timer hook was not found in the list of registered hooks.");
	}
}

/// <summary>
/// The static interrupt method called by the hardware
/// </summary>
void Timer::timer2Interrupt()
{
	timer.update();
}

/// <summary>
/// Constructor
/// </summary>
Timer::Timer()
	: count(0)
{
	// Get the timer ready to tick, but without kicking it off
	NRF_TIMER2->TASKS_STOP = 1;	// Stop timer
	NRF_TIMER2->MODE = TIMER_MODE_MODE_Timer;  // taken from Nordic dev zone
	NRF_TIMER2->BITMODE = TIMER_BITMODE_BITMODE_16Bit;
	NRF_TIMER2->PRESCALER = TIMER2_PRESCALER;	// 16MHz / (2^8) = 16 us resolution
	NRF_TIMER2->TASKS_CLEAR = 1; // Clear timer
	NVIC_SetPriority(TIMER2_IRQn, 3);
	dynamic_attachInterrupt(TIMER2_IRQn, Timer::timer2Interrupt);
}

/// <summary>
/// Kick off the timer
/// </summary>
void Timer::begin()
{
	NRF_TIMER2->TASKS_START = 1;	// Start TIMER
}

/// <summary>
/// Stop the timer
/// </summary>
void Timer::stop()
{
	NRF_TIMER2->TASKS_STOP = 1;	// Stop timer
}

/// <summary>
/// Called when a timer interrupt occurs
/// </summary>
void Timer::update()
{
	for (int i = 0; i < count; ++i)
	{
		if (NRF_TIMER2->EVENTS_COMPARE[i] != 0)
		{
			if (i == 0)
				digitalWrite(0, HIGH);

			// Clear interrupt
			NRF_TIMER2->EVENTS_COMPARE[i] = 0;

			auto& clientInfo = clients[i];

			// Update the compare value for next multiple
			NRF_TIMER2->CC[i] += clientInfo.ticks;

			if (clientInfo.callback != nullptr)
			{
				clientInfo.callback(clientInfo.param);
			}
			else
			{
				console.print("Timer event ");
				console.print(i);
				console.print(" does not have a registered hook!");
			}

			if (i == 0)
				digitalWrite(0, LOW);
		}
	}
}