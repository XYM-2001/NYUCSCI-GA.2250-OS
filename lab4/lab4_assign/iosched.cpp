#include <iostream>
#include <fstream>
#include <sstream>
#include <queue>
#include <getopt.h>
#include <list>
#include <iomanip>
#include <cstdlib>
#include <limits.h>

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
    // Constructor for initializing an IORequest
    IORequest(int iop_val, int arr_time_val, int track_number_val)
        : iop(iop_val),
          arr_time(arr_time_val),
          track_number(track_number_val),
          start_time(-1),
          end_time(-1)
    {
        // You can add additional initialization logic if needed
    }
};

list<IORequest *> requests;
list<IORequest *> finished_requests;
int curr_time = 1;
int curr_track = 0;
int time_io_was_busy = 0;
int prev_io_change = 0;
int dir = 1;
int total_movement = 0;
bool v_out = false;
bool q_out = false;
IORequest *active_request = nullptr;

// Comparator function to sort based on iop
bool compareIop(const IORequest *a, const IORequest *b)
{
    return a->iop < b->iop;
}

// Define classes for different scheduling algorithms

class Scheduler
{
public:
    virtual void addQ(IORequest *) = 0;
    virtual IORequest *get_next_request() = 0;
    virtual bool io_empty() = 0;
};

// FIFO Scheduler
class FIFO_Scheduler : public Scheduler
{
    // Implement FIFO scheduling logic
public:
    void addQ(IORequest *request) override
    {
        IO_Queue.push_back(request);
    }

    IORequest *get_next_request() override
    {
        IORequest *res = IO_Queue.front();
        IO_Queue.erase(IO_Queue.begin());
        (res->track_number > curr_track) ? dir = 1 : dir = -1;
        return res;
    }

    bool io_empty() override
    {
        if (IO_Queue.empty())
        {
            return true;
        }
        return false;
    }

private:
    vector<IORequest *> IO_Queue;
};

// SSTF Scheduler
class SSTF_Scheduler : public Scheduler
{
    // Implement SSTF scheduling logic
public:
    void addQ(IORequest *request) override
    {
        IO_Queue.push_back(request);
    }

    IORequest *get_next_request() override
    {
        auto closestIterator = IO_Queue.begin(); // Iterator to the closest element
        int minDistance = abs(curr_track - (*closestIterator)->track_number);

        for (auto it = IO_Queue.begin() + 1; it != IO_Queue.end(); ++it)
        {
            int distance = abs(curr_track - (*it)->track_number);
            if (distance < minDistance)
            {
                minDistance = distance;
                closestIterator = it;
            }
        }

        IORequest *res = *closestIterator;
        if (q_out)
        {
            cout << "\tGet: "
                 << "(";
            for (auto request : IO_Queue)
            {
                cout << request->iop << ":" << abs(request->track_number - curr_track) << " ";
            }
            cout << ") --> " << res->iop << endl;
        }
        IO_Queue.erase(closestIterator);
        (res->track_number > curr_track) ? dir = 1 : dir = -1;
        return res;
    }

    bool io_empty() override
    {
        if (IO_Queue.empty())
        {
            return true;
        }
        return false;
    }

private:
    vector<IORequest *> IO_Queue;
};

// LOOK Scheduler
class LOOK_Scheduler : public Scheduler
{
    // Implement LOOK scheduling logic
    void addQ(IORequest *request) override
    {
        IO_Queue.push_back(request);
    }

    IORequest *get_next_request() override
    {
        bool found = false;
        auto closestIterator = IO_Queue.begin();
        int minDistance = INT_MAX;
        for (auto it = IO_Queue.begin(); it != IO_Queue.end(); ++it)
        {
            int distance = abs(curr_track - (*it)->track_number);
            if ((((*it)->track_number - curr_track) * dir >= 0) && distance < minDistance)
            {
                found = true;
                minDistance = distance;
                closestIterator = it;
            }
        }

        if (!found) // if all requests are in the other direction
        {
            dir = -dir;
            for (auto it = IO_Queue.begin(); it != IO_Queue.end(); ++it)
            {
                int distance = abs(curr_track - (*it)->track_number);
                if (distance < minDistance)
                {
                    minDistance = distance;
                    closestIterator = it;
                }
            }
        }
        IORequest *res = *closestIterator;
        if (q_out)
        {
            // cout << curr_track << endl;
            cout << "\tGet: "
                 << "(";
            for (auto request : IO_Queue)
            {
                cout << request->iop << ":" << dir * (request->track_number - curr_track) << " ";
            }
            cout << ") --> " << res->iop << " dir=" << dir << endl;
        }
        IO_Queue.erase(closestIterator);
        return res;
    }

    bool io_empty() override
    {
        if (IO_Queue.empty())
        {
            return true;
        }
        return false;
    }

private:
    vector<IORequest *> IO_Queue;
};

// CLOOK Scheduler
class CLOOK_Scheduler : public Scheduler
{
    // Implement CLOOK scheduling logic
    void addQ(IORequest *request) override
    {
        IO_Queue.push_back(request);
    }

    IORequest *get_next_request() override
    {
        if (reverse)
        {
            reverse = false;
            dir = -dir;
        }
        bool found = false;
        auto closestIterator = IO_Queue.begin();
        int minDistance = INT_MAX;
        for (auto it = IO_Queue.begin(); it != IO_Queue.end(); ++it)
        {
            int distance = abs(curr_track - (*it)->track_number);
            if ((((*it)->track_number - curr_track) * dir >= 0) && distance < minDistance)
            {
                found = true;
                minDistance = distance;
                closestIterator = it;
            }
        }

        if (!found) // if all requests are in the other direction
        {
            reverse = true;
            dir = -dir;
            closestIterator = IO_Queue.begin();
            for (auto it = IO_Queue.begin(); it != IO_Queue.end(); ++it)
            {
                if ((*it)->track_number < (*closestIterator)->track_number)
                {
                    closestIterator = it;
                }
            }
        }
        IORequest *res = *closestIterator;
        if (q_out)
        {
            // cout << curr_track << endl;
            cout << "\tGet: "
                 << "(";
            for (auto request : IO_Queue)
            {
                cout << request->iop << ":" << request->track_number - curr_track << " ";
            }
            cout << ") --> " << res->iop << " dir=" << dir << endl;
        }
        IO_Queue.erase(closestIterator);
        return res;
    }

    bool io_empty() override
    {
        if (IO_Queue.empty())
        {
            return true;
        }
        return false;
    }

private:
    vector<IORequest *> IO_Queue;
    bool reverse = false;
};

// FLOOK Scheduler
class FLOOK_Scheduler : public Scheduler
{
    // Implement FLOOK scheduling logic
    void addQ(IORequest *request) override
    {
        addQueue.push_back(request);
    }

    IORequest *get_next_request() override
    {
        if (activeQueue.empty())
        {
            activeQueue.swap(addQueue);
        }
        bool found = false;
        auto closestIterator = activeQueue.begin();
        int minDistance = INT_MAX;
        for (auto it = activeQueue.begin(); it != activeQueue.end(); ++it)
        {
            int distance = abs(curr_track - (*it)->track_number);
            if ((((*it)->track_number - curr_track) * dir >= 0) && distance < minDistance)
            {
                found = true;
                minDistance = distance;
                closestIterator = it;
            }
        }

        if (!found) // if all requests are in the other direction
        {
            dir = -dir;
            for (auto it = activeQueue.begin(); it != activeQueue.end(); ++it)
            {
                int distance = abs(curr_track - (*it)->track_number);
                if (distance < minDistance)
                {
                    minDistance = distance;
                    closestIterator = it;
                }
            }
        }
        IORequest *res = *closestIterator;
        if (q_out)
        {
            // cout << curr_track << endl;
            cout << "\tGet: Q[0] (";
            for (auto request : activeQueue)
            {
                cout << request->iop << ":" << dir * (request->track_number - curr_track) << " ";
            }
            cout << ") Q[1] (";
            for (auto request : addQueue)
            {
                cout << request->iop << ":" << dir * (request->track_number - curr_track) << " ";
            }
            cout << "--> " << res->iop << " dir=" << dir << endl;
        }
        activeQueue.erase(closestIterator);
        return res;
    }

    bool io_empty() override
    {
        if (activeQueue.empty() && addQueue.empty())
        {
            return true;
        }
        return false;
    }

private:
    vector<IORequest *> activeQueue;
    vector<IORequest *> addQueue;
};

Scheduler *sched;

void Simulation()
{
    // Implement simulation logic
    while (true)
    {
        // Check for new arrivals and add requests to IO-queue
        if (!requests.empty() && requests.front()->arr_time == curr_time)
        {
            if (v_out)
            {
                cout << curr_time << ":\t" << requests.front()->iop << " add " << requests.front()->track_number << endl;
            }
            sched->addQ(requests.front());
            requests.pop_front();
        }

        // Check if an IO operation is completed
        if (active_request != nullptr && active_request->track_number == curr_track)
        {
            active_request->end_time = curr_time;
            if (v_out)
            {
                cout << curr_time << ":\t" << active_request->iop << " finish " << active_request->end_time - active_request->arr_time << endl;
            }
            finished_requests.push_back(active_request);
            active_request = nullptr;
            time_io_was_busy += curr_time - prev_io_change;
        }
        // If no IO is active
        // Check for pending requests and start a new IO if available
        // Otherwise, exit simulation if all requests are processed
        while (active_request == nullptr && !sched->io_empty())
        {
            active_request = sched->get_next_request();
            active_request->start_time = curr_time;
            prev_io_change = curr_time;
            // if (q_out)
            // {
            //     cout << "AQ=" << (active_request != nullptr ? 1 : 0) << " dir=" << dir << " curtrack=" << curr_track << "\tQ[0]=( ";
            //     if (active_request == nullptr)
            //     {
            //         cout << ")";
            //     }
            //     else
            //     {
            //         cout << active_request->iop << ":" << active_request->track_number << ":" << active_request->track_number - curr_track;
            //     }
            // }
            if (v_out)
            {
                cout << curr_time << ":\t" << active_request->iop << " issue " << active_request->track_number << " " << curr_track << endl;
            }
            if (active_request->track_number == curr_track)
            {
                active_request->end_time = curr_time;
                if (v_out)
                {
                    cout << curr_time << ":\t" << active_request->iop << " finish " << active_request->end_time - active_request->arr_time << endl;
                }
                finished_requests.push_back(active_request);
                active_request = nullptr;
            }
        }

        if (requests.empty() && sched->io_empty() && !active_request)
        {
            break;
        }

        // If an IO is active, move the head by one unit
        if (active_request)
        {
            curr_track += dir;
            total_movement++;
        }
        // Increment time by 1
        curr_time++;
    }
    return;
}

int main(int argc, char *argv[])
{
    // Parse command-line arguments and input file
    // Initialize necessary variables, data structures, and flags
    int opt;
    char algo;
    string inputfile;
    while ((opt = getopt(argc, argv, "s:vqf")) != -1)
    {
        switch (opt)
        {
        case 's':
            algo = optarg[0];
            break;
        case 'v':
            v_out = true;
            cout << "TRACE" << endl;
            break;
        case 'q':
            q_out = true;
            break;
        default:
            std::cerr << "Usage: " << argv[0] << " [-s <schedalgo>] <inputfile>" << std::endl;
            return 1;
        }
    }
    if (algo == 'N')
    {
        sched = new FIFO_Scheduler();
    }
    else if (algo == 'S')
    {
        sched = new SSTF_Scheduler();
    }
    else if (algo == 'L')
    {
        sched = new LOOK_Scheduler();
    }
    else if (algo == 'C')
    {
        sched = new CLOOK_Scheduler();
    }
    else if (algo == 'F')
    {
        sched = new FLOOK_Scheduler();
    }
    // Read input file and create IO requests
    // Store them in a suitable data structure

    if (optind + 1 == argc)
    {
        inputfile = argv[optind];
    }
    else
    {
        cerr << "Error: Missing input file and/or rand file." << endl;
        return 1; // Return an error code for missing input files
    }

    ifstream inputFile(inputfile);
    string line;
    int iop = 0;
    while (getline(inputFile, line))
    {
        if (line[0] != '#')
        {
            istringstream iss(line);
            int arr_time, track_num;
            iss >> arr_time >> track_num;
            IORequest *request = new IORequest(iop, arr_time, track_num);
            requests.push_back(request);
            iop++;
        }
    }
    inputFile.close();
    Simulation();
    // Compute statistics and print information for each IO request
    int total_turnaround = 0;
    int total_wait_time = 0;
    int max_wait_time = 0;
    finished_requests.sort(compareIop);
    while (!finished_requests.empty())
    {
        IORequest *temp = finished_requests.front();
        total_turnaround += temp->end_time - temp->arr_time;
        int curr_wait_time = temp->start_time - temp->arr_time;
        total_wait_time += curr_wait_time;
        if (max_wait_time < curr_wait_time)
        {
            max_wait_time = curr_wait_time;
        }
        cout << setw(5) << temp->iop << ": "
             << setw(5) << temp->arr_time << " "
             << setw(5) << temp->start_time << " "
             << setw(5) << temp->end_time << endl;
        finished_requests.pop_front();
        delete temp;
    }
    // Print SUM line with computed statistics
    cout << "SUM: " << curr_time << " " << total_movement << " "
         << fixed << setprecision(4) << static_cast<double>(time_io_was_busy) / curr_time << " "
         << setprecision(2) << static_cast<double>(total_turnaround) / iop << " "
         << setprecision(2) << static_cast<double>(total_wait_time) / iop << " " << max_wait_time << endl;
    // cout << "request is empty: " << requests.empty() << endl;
    // cout << "ioq is empty: " << sched->io_empty() << endl;
    // cout << "Simulation finished" << endl;
    delete sched;
    requests.clear();
    return 0;
}
