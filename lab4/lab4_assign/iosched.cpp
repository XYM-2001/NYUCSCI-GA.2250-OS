#include <iostream>
#include <fstream>
#include <sstream>
#include <queue>
#include <getopt.h>
#include <list>
#include <iomanip>

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
queue<IORequest *> finished_requests;
int curr_time = 1;
int curr_track = 0;
int time_io_was_busy = 0;
int prev_io_change = 0;
int dir = 1;
int total_movement = 0;
bool v_out = false;
bool q_out = false;
IORequest *active_request = nullptr;

// Define classes for different scheduling algorithms

class Scheduler
{
public:
    virtual void addQ(IORequest *) = 0;
    virtual IORequest *get_next_request() = 0;
    virtual bool io_empty() = 0;
    virtual void print_io_queue() = 0;
};

// FIFO Scheduler
class FIFO_Scheduler : public Scheduler
{
    // Implement FIFO scheduling logic
public:
    void print_io_queue() override
    {
        for (auto request : IO_Queue)
        {
            cout << request->iop << ":" << request->track_number << " ";
        }
    }

    void addQ(IORequest *request) override
    {
        IO_Queue.push_back(request);
        if (q_out)
        {
            cout << "Q=" << (active_request != nullptr ? 1 : 0) << " ( ";
            print_io_queue();
            cout << ")" << endl;
        }
    }

    IORequest *get_next_request() override
    {
        IORequest *res = IO_Queue.front();
        IO_Queue.pop_front();
        return res;
    }

    bool io_empty()
    {
        if (IO_Queue.empty())
        {
            return true;
        }
        return false;
    }

private:
    list<IORequest *> IO_Queue;
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
            cout << curr_time << ":\t" << active_request->iop << " finish " << active_request->end_time - active_request->arr_time << endl;
            finished_requests.push(active_request);
            active_request = nullptr;
            time_io_was_busy += curr_time - prev_io_change;
        }
        // If no IO is active
        // Check for pending requests and start a new IO if available
        // Otherwise, exit simulation if all requests are processed
        if (active_request == nullptr)
        {
            if (sched->io_empty() && requests.empty())
            {
                break;
            }
            else if (!sched->io_empty())
            {
                active_request = sched->get_next_request();
                active_request->start_time = curr_time;
                prev_io_change = curr_time;
                if (active_request->track_number <= curr_track)
                {
                    dir = -dir;
                }
                if (q_out)
                {
                    cout << "AQ=" << (active_request != nullptr ? 1 : 0) << " dir=" << dir << " curtrack=" << curr_track << "\tQ[0]=( ";
                    if (active_request == nullptr)
                    {
                        cout << ")";
                    }
                    else
                    {
                        cout << active_request->iop << ":" << active_request->track_number << ":" << active_request->track_number - curr_track;
                    }
                }
                if (v_out)
                {
                    cout << curr_time << ":\t" << active_request->iop << " issue " << active_request->track_number << " " << curr_track << endl;
                }
            }
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
    cout << "request is empty: " << requests.empty() << endl;
    cout << "ioq is empty: " << sched->io_empty() << endl;
    int total_turnaround = 0;
    int total_wait_time = 0;
    int max_wait_time = 0;
    while (!finished_requests.empty())
    {
        IORequest *temp = finished_requests.front();
        total_turnaround += temp->end_time - temp->start_time;
        int curr_wait_time = temp->start_time - temp->arr_time;
        total_wait_time += curr_wait_time;
        if (max_wait_time < curr_wait_time)
        {
            max_wait_time = curr_wait_time;
        }
        cout << setw(5) << temp->iop << temp->arr_time << temp->start_time << temp->end_time;
        finished_requests.pop();
        delete temp;
    }
    requests.clear();
    delete sched;
    // Print SUM line with computed statistics
    cout << "SUM: " << curr_time << " " << total_movement << time_io_was_busy / curr_time << " " << total_turnaround / iop << total_wait_time / iop << max_wait_time;
    cout << "Simulation finished" << endl;
    return 0;
}
