// $Id$

#ifndef __SCHEDULER_HH__
#define __SCHEDULER_HH__

#include "EmuTime.hh"
#include "HotKey.hh"
#include <SDL/SDL.h>
#include <set>
#include <string>

/**
 * Every class that wants to get scheduled at some point must inherit from
 * this class
 */
class Schedulable
{
	public:
		/**
		 * When the previously registered syncPoint is reached, this
		 * method gets called. The parameter "userData" is the same 
		 * as passed to setSyncPoint(). 
		 */
		virtual void executeUntilEmuTime(const EmuTime &time, int userData) = 0;
		
		/**
		 * This method is only used to print meaningfull debug messages
		 */
		virtual const std::string &getName();
		
	protected:
		static const std::string defaultName;
};

class Scheduler : public EventListener, public HotKeyListener, public Schedulable
{
	class SynchronizationPoint
	{
		public:
			SynchronizationPoint (const EmuTime &time, Schedulable *dev, int usrdat) : 
				timeStamp(time), device(dev) , userData(usrdat) {}
			const EmuTime &getTime() const { return timeStamp; }
			Schedulable* getDevice() const { return device; }
			int getUserData() const { return userData; }
			bool operator< (const SynchronizationPoint &n) const 
				{ return getTime() > n.getTime(); } // smaller time is higher priority
		private: 
			EmuTime timeStamp;
			Schedulable* device;
			int userData;
	};

	public:
		/**
		 * Destructor
		 */
		virtual ~Scheduler();

		/**
		 * This is a singleton class,
		 *  usage: Scheduler::instance()->method();
		 */
		static Scheduler *instance();

		/**
		 * Register a syncPoint. When the emulation reaches "timestamp",
		 * the executeUntilEmuTime() method of "device" gets called.
		 * SyncPoints are ordered: smaller EmuTime -> scheduled
		 * earlier.
		 * A device may register several syncPoints.
		 * Optionally a "userData" parameter can be passed, this
		 * parameter is not used by the Scheduler but it is passed to
		 * the executeUntilEmuTime() method of "device". This is useful
		 * if you want to distinguish between several syncPoint types.
		 * If you do not supply "userData" it is assumed to be zero.
		 */
		void setSyncPoint(const EmuTime &timestamp, Schedulable* device, int userData = 0);

		/**
		 * Removes a syncPoint of a given device that matches the given
		 * userData. Returns true if a match is found and removed.
		 * If there is more than one match only one will be removed,
		 * there is no guarantee that the earliest syncPoint is
		 * removed.
		 * Removing a syncPoint is a relatively expensive operation,
		 * if possible don't remove the syncPoint but ignore it in
		 * your executeUntilEmuTime() method.
		 */
		bool Scheduler::removeSyncPoints(Schedulable* device, int userdata = 0);
		
		/**
		 * This starts the schedule loop, should only be used by main
		 * the program.
		 */
		void scheduleEmulation();

		/**
		 * This stops the schedule loop, should only be used by the 
		 * quit program routine.
		 */
		void stopScheduling();

		/**
		 * This pauses the emulation.
		 */
		void pause();

		/**
		 * This unpauses the emulation
		 */
		void unpause();
		
		
		// Schedulable
		void executeUntilEmuTime(const EmuTime &time, int userData);
		// EventListener
		void signalEvent(SDL_Event &event);
		// HotKeyListener
		void signalHotKey(SDLKey key);
		
	private:
		Scheduler();
		const SynchronizationPoint &getFirstSP();
		void removeFirstSP();
		
		static Scheduler *oneInstance;
		// vector used as heap, not a priority queue because this
		// doesn't allow removal of non-top element
		std::vector<SynchronizationPoint> syncPoints;

		bool paused;
		bool noSound;
		bool runningScheduler;
		SDL_mutex *pauseMutex;
};

#endif
