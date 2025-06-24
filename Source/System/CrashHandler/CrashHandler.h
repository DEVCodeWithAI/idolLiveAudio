#pragma once

#include <JuceHeader.h>

/**
    A utility class to install a global crash handler for the application.
    This helps in catching unhandled exceptions and providing useful debug
    information to the user.
*/
class CrashHandler
{
public:
    /** Installs the crash handler. 
        This should be called once, at the very beginning of the application's startup.
    */
    static void install();

private:
    // This is a static-only class, no instantiation allowed.
    CrashHandler() = delete;
    ~CrashHandler() = delete;
    CrashHandler(const CrashHandler&) = delete;
    CrashHandler& operator=(const CrashHandler&) = delete;
};