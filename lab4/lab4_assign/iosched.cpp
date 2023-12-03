#include <iostream>
#include <fstream>
#include <queue>
#include <vector>
#include <getopt.h>

using namespace std;

// Define structs or classes for IO requests and other necessary components

struct IORequest
{
    int iop;
    int arr_time;
    int track_number;
    int start_time;
    int end_time;
    // Add other necessary fields
};

// Define comparison functions for sorting in certain schedulers if needed

// Comparator for sorting based on track number
struct CompareTrack
{
    bool operator()(const IORequest &a, const IORequest &b)
    {
        return a.track_number > b.track_number;
    }
};

// Define classes for different scheduling algorithms

class Scheduler
{
};

// FIFO Scheduler
class FIFO_Scheduler : public Scheduler
{
    // Implement FIFO scheduling logic
};

// SSTF Scheduler
class SSTF_Scheduler : public Scheduler
{
    // Implement SSTF scheduling logic
};

// LOOK Scheduler
class LOOK_Scheduler : public Scheduler
{
    // Implement LOOK scheduling logic
};

// CLOOK Scheduler
class CLOOK_Scheduler : public Scheduler
{
    // Implement CLOOK scheduling logic
};

// FLOOK Scheduler
class FLOOK_Scheduler : public Scheduler
{
    // Implement FLOOK scheduling logic
};

int main(int argc, char *argv[])
{
    // Parse command-line arguments and input file
    // Initialize necessary variables, data structures, and flags

    int opt;
    const char *optstring = "svqf";
    string algo;
    while ((opt = getopt(argc, argv, optstring)) != -1)
    {
        switch (opt)
        {
        case 's':
            algo = optarg;
        default:
            cerr << "Error: Unknown option or invalid usage." << endl;
            return 1; // Return an error code for unknown options
        }
    }
    // Read input file and create IO requests
    // Store them in a suitable data structure

    // Implement simulation logic
    while (true)
    {
        // Check for new arrivals and add requests to IO-queue

        // Check if an IO operation is completed

        // If no IO is active
        // Check for pending requests and start a new IO if available
        // Otherwise, exit simulation if all requests are processed

        // If an IO is active, move the head by one unit

        // Increment time by 1
    }

    // Compute statistics and print information for each IO request

    // Print SUM line with computed statistics

    return 0;
}
