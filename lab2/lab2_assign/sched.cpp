#include <iostream>
#include <fstream>
#include <vector>
#include <queue>
#include <cstdlib>
#include <cstring>
#include <getopt.h>
#include <sstream>
#include <string>
#include <list>
#include <map>
#include <iomanip>

using namespace std;

vector<int> randvals;
int ofs = 1;
int maxprio = 4;
int CURRENT_TIME = 0;
int TOTAL_IO;
bool CALL_SCHEDULER = false;
int block_num = 0;
int totalb = 0;
int block_time = 0;
bool verbose = false;
bool trace = false;
bool error = false;
bool process = false;

// Define process states using an enumeration
enum ProcessState
{
    CREATED,
    READY,
    RUNNING,
    BLOCKED,
    TERMINATED
};

map<ProcessState, string> stateToString = {
    {CREATED, "CREATED"},
    {READY, "READY"},
    {RUNNING, "RUNNING"},
    {BLOCKED, "BLOCKED"},
    {TERMINATED, "DONE"}};

enum Transition
{
    TRANS_TO_READY,
    TRANS_TO_PREEMPT,
    TRANS_TO_RUN,
    TRANS_TO_BLOCK,
    DONE
};

int myrandom(int burst)
{
    int result;
    if (ofs > randvals[0])
    {
        ofs = 1;
    }
    result = 1 + (randvals[ofs] % burst);
    ofs++;
    return result;
}

// Define a Process class to represent processes
class Process
{
public:
    int pid;           // Process ID
    int at;            // Arrival Time
    int tc;            // Total CPU Time
    int cb;            // CPU Burst
    int io;            // IO Burst
    int static_prio;   // Static Priority
    int dynamic_prio;  // Dynamic Priority
    int cpu_burst = 0; // cpu burst
    int io_burst = 0;  // io burst
    int state_ts = 0;
    int rem;
    int ft;
    int ready;
    int it = 0;
    int cw = 0;
    ProcessState state; // Current State

    // Constructor to initialize a process
    Process(int pid, int at, int tc, int cb, int io)
    {
        this->pid = pid;
        this->at = at;
        this->tc = tc;
        this->cb = cb;
        this->io = io;
        this->static_prio = myrandom(maxprio);
        this->dynamic_prio = this->static_prio - 1;
        this->rem = tc;
        this->state = CREATED;
    }

    // Implement methods to update the process state and calculate statistics
};

// Define an Event class to represent events
class Event
{
public:
    int timestamp;         // Event Timestamp
    Process *process;      // Associated Process
    Transition transition; // State Transition

    // Constructor to initialize an event
    Event(int timestamp, Process *process, Transition transition)
    {
        this->timestamp = timestamp;
        this->process = process;
        this->transition = transition;
    }
};

Process *CURRENT_RUNNING_PROCESS = nullptr;
list<Event *> eventQueue;

class Scheduler
{
public:
    virtual Process *getNextProcess() = 0;
    virtual void addrunQ(Process *process) = 0;
    virtual string toString() = 0;
    virtual int getQuantum() = 0;
    virtual bool test_preempt(Process *p) = 0;
};

class FCFS_Scheduler : public Scheduler
{
public:
    string toString() override
    {
        return "FCFS";
    }

    // Implement the interface functions
    void addrunQ(Process *process) override
    {
        runQueue.push_back(process);
    }

    Process *getNextProcess() override
    {
        if (!runQueue.empty())
        {
            Process *nextProcess = runQueue.front();
            runQueue.pop_front();
            return nextProcess;
        }
        return nullptr;
    }

    int getQuantum() override
    {
        return 10000;
    }

    bool test_preempt(Process *p) override
    {
        return false;
    }

private:
    list<Process *> runQueue;
};

class LCFS_Scheduler : public Scheduler
{
public:
    string toString()
    {
        return "LCFS";
    }

    // Implement the interface functions
    void addrunQ(Process *process) override
    {
        runQueue.push_front(process);
    }

    Process *getNextProcess() override
    {
        if (!runQueue.empty())
        {
            Process *nextProcess = runQueue.front();
            runQueue.pop_front();
            return nextProcess;
        }
        return nullptr;
    }

    int getQuantum() override
    {
        return 10000;
    }

    bool test_preempt(Process *p) override
    {
        return false;
    }

private:
    list<Process *> runQueue;
};

class SRTF_Scheduler : public Scheduler
{
public:
    string toString() override
    {
        return "SRTF";
    }

    // Implement the interface functions
    void addrunQ(Process *process) override
    {
        if (runQueue.empty())
        {
            runQueue.push_back(process);
        }
        else
        {
            // Find the correct position to insert the event.
            auto it = runQueue.begin();
            while (it != runQueue.end() && ((*it)->rem < process->rem))
            {
                ++it;
            }

            // If there are events with the same timestamp, insert the new event after them.
            while (it != runQueue.end() && (*it)->rem == process->rem)
            {
                ++it;
            }

            runQueue.insert(it, process);
        }
    }

    Process *getNextProcess() override
    {
        if (!runQueue.empty())
        {
            Process *nextProcess = runQueue.front();
            runQueue.pop_front();
            return nextProcess;
        }
        return nullptr;
    }

    int getQuantum() override
    {
        return 10000;
    }

    bool test_preempt(Process *p) override
    {
        return false;
    }

private:
    list<Process *> runQueue;
};

class RR_Scheduler : public Scheduler
{
public:
    RR_Scheduler(int quantum)
    {
        this->quantum = quantum;
    }
    string toString() override
    {
        return ("RR" + to_string(this->quantum));
    }

    // Implement the interface functions
    void addrunQ(Process *process) override
    {
        runQueue.push_back(process);
    }

    Process *getNextProcess() override
    {
        if (!runQueue.empty())
        {
            Process *nextProcess = runQueue.front();
            runQueue.pop_front();
            return nextProcess;
        }
        return nullptr;
    }

    int getQuantum() override
    {
        return quantum;
    }

    bool test_preempt(Process *p) override
    {
        return false;
    }

private:
    list<Process *> runQueue;
    int quantum;
};

class PRIO_Scheduler : public Scheduler
{
public:
    PRIO_Scheduler(int quantum)
    {
        activeQ.resize(maxprio);
        expiredQ.resize(maxprio);
        this->quantum = quantum;
    }
    string toString() override
    {
        return ("PRIO " + to_string(this->quantum));
    }

    // Implement the addrunQ function
    void addrunQ(Process *process) override
    {
        // Add the process to the activeQ based on its dynamic priority
        process->dynamic_prio--;
        if (process->dynamic_prio < 0)
        {
            process->dynamic_prio = process->static_prio - 1;
            expiredQ[process->dynamic_prio].push_back(process);
        }
        else
        {
            activeQ[process->dynamic_prio].push_back(process);
        }
    }

    // Implement the getNextProcess function
    Process *getNextProcess() override
    {
        // Iterate through the activeQ and find the first non-empty list with the highest priority
        for (int priority = maxprio - 1; priority >= 0; priority--)
        {
            if (!activeQ[priority].empty())
            {
                Process *nextProcess = activeQ[priority].front();
                activeQ[priority].pop_front();
                return nextProcess;
            }
        }

        // If activeQ is empty, swap activeQ and expiredQ and return the first non-empty list from activeQ
        activeQ.swap(expiredQ);
        for (int priority = maxprio - 1; priority >= 0; priority--)
        {
            if (!activeQ[priority].empty())
            {
                Process *nextProcess = activeQ[priority].front();
                activeQ[priority].pop_front();
                return nextProcess;
            }
        }
        return nullptr; // If both queues are empty, return nullptr
    }

    int getQuantum() override
    {
        return quantum;
    }

    bool test_preempt(Process *p) override
    {
        return false;
    }

private:
    vector<list<Process *>> activeQ, expiredQ;
    int quantum;
};

class PREPRIO_Scheduler : public Scheduler
{
public:
    PREPRIO_Scheduler(int quantum)
    {
        activeQ.resize(maxprio);
        expiredQ.resize(maxprio);
        this->quantum = quantum;
    }
    string toString() override
    {
        return ("PREPRIO " + to_string(this->quantum));
    }

    // Implement the addrunQ function
    void addrunQ(Process *process) override
    {
        // Add the process to the activeQ based on its dynamic priority
        process->dynamic_prio--;
        if (process->dynamic_prio < 0)
        {
            process->dynamic_prio = process->static_prio - 1;
            expiredQ[process->dynamic_prio].push_back(process);
        }
        else
        {
            activeQ[process->dynamic_prio].push_back(process);
        }
    }

    // Implement the getNextProcess function
    Process *getNextProcess() override
    {
        // Iterate through the activeQ and find the first non-empty list with the highest priority
        for (int priority = maxprio - 1; priority >= 0; priority--)
        {
            if (!activeQ[priority].empty())
            {
                Process *nextProcess = activeQ[priority].front();
                activeQ[priority].pop_front();
                return nextProcess;
            }
        }

        // If activeQ is empty, swap activeQ and expiredQ and return the first non-empty list from activeQ
        activeQ.swap(expiredQ);
        for (int priority = maxprio - 1; priority >= 0; priority--)
        {
            if (!activeQ[priority].empty())
            {
                Process *nextProcess = activeQ[priority].front();
                activeQ[priority].pop_front();
                return nextProcess;
            }
        }
        return nullptr; // If both queues are empty, return nullptr
    }

    int getQuantum() override
    {
        return quantum;
    }

    bool test_preempt(Process *p) override
    {
        if (CURRENT_RUNNING_PROCESS == nullptr)
        {
            return false;
        }
        bool cond1 = p->dynamic_prio > CURRENT_RUNNING_PROCESS->dynamic_prio;
        auto it = eventQueue.begin();
        while (it != eventQueue.end() && ((*it)->process->pid != CURRENT_RUNNING_PROCESS->pid))
        {
            ++it;
        }
        bool cond2 = (*it)->timestamp > CURRENT_TIME;
        if (verbose)
        {
            cout << endl;
            cout << " --> PrioPreempt Cond1=";
            cout << cond1 << " Cond2=";
            cout << cond2 << " (" << (*it)->timestamp - CURRENT_TIME << ") --> ";
        }
        if (cond1 && cond2)
        {
            if (verbose)
            {
                cout << "YES";
            }

            CURRENT_RUNNING_PROCESS->state_ts -= ((*it)->timestamp - CURRENT_TIME);
            // activeQ[p->dynamic_prio].pop_back();
            delete *it;
            eventQueue.erase(it);
        }
        else
        {
            if (verbose)
            {
                cout << "NO";
            }
        }
        return (cond1 && cond2);
    }

private:
    vector<list<Process *>> activeQ, expiredQ;
    int quantum;
};

Scheduler *scheduler;
queue<Process *> res;

void add_event(Event *evt)
{
    if (eventQueue.empty())
    {
        eventQueue.push_back(evt);
    }
    else
    {
        // Find the correct position to insert the event.
        auto it = eventQueue.begin();
        while (it != eventQueue.end() && ((*it)->timestamp < evt->timestamp))
        {
            ++it;
        }

        // If there are events with the same timestamp, insert the new event after them.
        while (it != eventQueue.end() && (*it)->timestamp == evt->timestamp)
        {
            ++it;
        }

        eventQueue.insert(it, evt);
    }
}

Event *get_event()
{
    if (!eventQueue.empty())
    {
        Event *event = eventQueue.front();
        eventQueue.pop_front();
        return event;
    }
    return nullptr;
}

int get_next_event_time()
{
    if (!eventQueue.empty())
    {
        // The eventQueue is not empty, so you can access the timestamp of the first event.
        return eventQueue.front()->timestamp;
    }
    else
    {
        // Handle the case when the eventQueue is empty, e.g., return a special value.
        return -1; // You can choose an appropriate value or error code.
    }
}

void Simulation()
{
    Event *evt;
    int quantum = scheduler->getQuantum();
    while ((evt = get_event()))
    {
        Process *proc = evt->process;
        CURRENT_TIME = evt->timestamp;
        Transition transition = evt->transition;
        int timeInPrevState = CURRENT_TIME - proc->state_ts;
        delete evt;
        evt = nullptr;
        if (verbose)
        {
            cout << CURRENT_TIME << " " << proc->pid << " " << proc->state_ts << ": ";
        }

        switch (transition)
        {
        case TRANS_TO_READY:
            // must come from BLOCKED or CREATED
            // add to run queue, no event created
            if (verbose)
            {
                cout << stateToString[proc->state] << " -> READY";
            }
            if (proc->state == BLOCKED)
            {
                proc->io_burst = 0;
                block_num -= 1;
                proc->dynamic_prio = proc->static_prio;
                if (block_num == 0)
                {
                    totalb += CURRENT_TIME - block_time;
                }
            }
            else
            {
                proc->dynamic_prio += 1;
            }
            scheduler->addrunQ(proc);
            if (proc->state == BLOCKED || proc->state == CREATED)
            {
                bool preempt = scheduler->test_preempt(proc);
                if (preempt)
                {
                    evt = new Event(CURRENT_TIME, CURRENT_RUNNING_PROCESS, TRANS_TO_PREEMPT);
                    eventQueue.push_front(evt);
                }
            }
            proc->state = READY;
            proc->ready = CURRENT_TIME;
            proc->state_ts = 0;
            CALL_SCHEDULER = true;
            break;
        case TRANS_TO_PREEMPT:
            // must come from RUNNING (preemption)
            // add to runqueue (no event is generated)
            proc->cpu_burst -= proc->state_ts;
            proc->rem -= proc->state_ts;
            if (verbose)
            {
                cout << stateToString[proc->state] << " -> READY";
                cout << " cb=" << proc->cpu_burst << " rem=" << proc->rem << " prio=" << proc->dynamic_prio;
            }
            scheduler->addrunQ(proc);
            CURRENT_RUNNING_PROCESS = nullptr;
            proc->state = READY;
            proc->ready = CURRENT_TIME;
            proc->state_ts = 0;
            CALL_SCHEDULER = true;
            break;
        case TRANS_TO_RUN:
            // create event for either preemption or blocking
            if (proc->cpu_burst == 0)
            {
                proc->cpu_burst = myrandom(proc->cb);
            }
            if (proc->cpu_burst <= quantum)
            {
                proc->state_ts = proc->cpu_burst;
            }
            else
            {
                proc->state_ts = quantum;
            }
            if ((proc->rem - proc->state_ts) <= 0)
            {
                proc->cpu_burst = proc->rem;
                proc->state_ts = proc->rem;
                if (verbose)
                {
                    cout << stateToString[proc->state] << " -> RUNNG cb=" << proc->cpu_burst << " rem=" << proc->rem << " prio=" << proc->dynamic_prio;
                }

                evt = new Event(CURRENT_TIME + proc->state_ts, proc, DONE);
            }
            else if (proc->state_ts < proc->cpu_burst)
            {
                if (verbose)
                {
                    cout << stateToString[proc->state] << " -> RUNNG cb=" << proc->cpu_burst << " rem=" << proc->rem << " prio=" << proc->dynamic_prio;
                }

                evt = new Event(CURRENT_TIME + proc->state_ts, proc, TRANS_TO_PREEMPT);
            }
            else
            {
                if (verbose)
                {
                    cout << stateToString[proc->state] << " -> RUNNG cb=" << proc->cpu_burst << " rem=" << proc->rem << " prio=" << proc->dynamic_prio;
                }

                evt = new Event(CURRENT_TIME + proc->state_ts, proc, TRANS_TO_BLOCK);
            }
            if (proc->state == READY)
            {
                proc->cw += CURRENT_TIME - proc->ready;
            }
            proc->state = RUNNING;
            add_event(evt);
            break;
        case TRANS_TO_BLOCK:
            // create an event for when process becomes READY again
            proc->rem -= proc->state_ts;
            proc->cpu_burst -= proc->state_ts;
            proc->io_burst = myrandom(proc->io);
            if (verbose)
            {
                cout << stateToString[proc->state] << " -> BLOCK ib=" << proc->io_burst << " rem=" << proc->rem;
            }
            proc->it += proc->io_burst;
            if (proc->state == READY)
            {
                proc->cw += proc->ready - CURRENT_TIME;
            }
            if (block_num == 0)
            {
                block_time = CURRENT_TIME;
            }
            block_num += 1;
            proc->state_ts = proc->io_burst;
            proc->state = BLOCKED;
            CURRENT_RUNNING_PROCESS = nullptr;
            evt = new Event(CURRENT_TIME + proc->state_ts, proc, TRANS_TO_READY);
            add_event(evt);
            CALL_SCHEDULER = true;
            break;
        case DONE:
            proc->rem -= proc->state_ts;
            proc->ft = CURRENT_TIME;
            CURRENT_RUNNING_PROCESS = nullptr;
            if (verbose)
            {
                cout << "Done";
            }

            CALL_SCHEDULER = true;
            break;
        }
        if (CALL_SCHEDULER)
        {
            if (get_next_event_time() == CURRENT_TIME)
            {
                if (verbose)
                {
                    cout << endl;
                }
                continue; // process next event from Event queue
            }
            CALL_SCHEDULER = false;
            if (CURRENT_RUNNING_PROCESS == nullptr)
            {
                CURRENT_RUNNING_PROCESS = scheduler->getNextProcess();
                if (CURRENT_RUNNING_PROCESS == nullptr)
                {
                    if (verbose)
                    {
                        cout << endl;
                    }

                    continue;
                }
                // create event to make this process runnable for same time.
                evt = new Event(CURRENT_TIME, CURRENT_RUNNING_PROCESS, TRANS_TO_RUN);
                add_event(evt);
            }
        }
        if (verbose)
        {
            cout << endl;
        }
    }
}

int main(int argc, char *argv[])
{
    // Parse command line arguments and open input and rand files
    int opt;
    string schedspec;
    string inputfile;
    string randfile;

    const char *optstring = "vtes:p";

    // Use getopt to parse command-line arguments
    while ((opt = getopt(argc, argv, optstring)) != -1)
    {
        switch (opt)
        {
        case 'v':
            verbose = true;
            break;
        case 't':
            trace = true;
            break;
        case 'e':
            error = true;
            break;
        case 'p':
            process = true;
            break;
        case 's':
            schedspec = optarg; // optarg contains the value following -s
            break;
        default:
            cerr << "Error: Unknown option or invalid usage." << endl;
            return 1; // Return an error code for unknown options
        }
    }

    // Parse non-option arguments
    if (optind + 2 == argc)
    {
        inputfile = argv[optind];
        randfile = argv[optind + 1];
    }
    else
    {
        cerr << "Error: Missing input file and/or rand file." << endl;
        return 1; // Return an error code for missing input files
    }

    ifstream randFile(randfile);
    int num;
    if (!(randFile >> num))
    {
        cerr << "cannot read size from randfile" << endl;
        exit(1);
    }
    randvals.push_back(num);
    for (int i = 0; i < randvals[0]; i++)
    {
        randFile >> num;
        randvals.push_back(num);
    }
    randFile.close();
    // Initialize the event queue and other data structures
    if (schedspec[0] == 'F')
    {
        scheduler = new FCFS_Scheduler();
    }
    else if (schedspec[0] == 'L')
    {
        scheduler = new LCFS_Scheduler();
    }
    else if (schedspec[0] == 'S')
    {
        scheduler = new SRTF_Scheduler();
    }
    else if (schedspec[0] == 'R')
    {
        scheduler = new RR_Scheduler(stoi(schedspec.substr(1)));
    }
    else if (schedspec[0] == 'P')
    {
        size_t colonPos = schedspec.find(':');
        if (colonPos != string::npos)
        {
            maxprio = stoi(schedspec.substr(colonPos + 1));
        }
        scheduler = new PRIO_Scheduler(stoi(schedspec.substr(1, colonPos - 1)));
    }
    else if (schedspec[0] == 'E')
    {
        size_t colonPos = schedspec.find(':');
        if (colonPos != string::npos)
        {
            maxprio = stoi(schedspec.substr(colonPos + 1));
        }
        scheduler = new PREPRIO_Scheduler(stoi(schedspec.substr(1, colonPos - 1)));
    }
    else
    {
        cerr << "error in scheduler speficication" << endl;
        exit(1);
    }
    // Read input processes and create initial events
    ifstream inputFile(inputfile);
    int pid = 0;
    string line;
    while (getline(inputFile, line))
    {
        istringstream iss(line);
        string token;
        iss >> token;
        int at = stoi(token); // Arrival Time
        iss >> token;
        int tc = stoi(token); // Total CPU Time
        iss >> token;
        int cb = stoi(token); // CPU Burst
        iss >> token;
        int io = stoi(token); // IO Burst
        Process *process = new Process(pid, at, tc, cb, io);
        pid++;
        res.push(process);
        Event *evt = new Event(at, process, Transition(TRANS_TO_READY));
        add_event(evt);
    }
    inputFile.close();
    // Run the simulation
    Simulation();
    // Calculate and print simulation results
    cout << scheduler->toString() << endl;
    int total_cpu = 0;
    int total_turnaround = 0;
    int proclen = res.size();
    int total_cw = 0;
    while (!res.empty())
    {
        Process *proc = res.front();
        total_cpu += proc->tc;
        total_turnaround += proc->ft - proc->at;
        total_cw += proc->cw;
        cout << setw(4) << setfill('0') << proc->pid << ": ";
        cout << setw(5) << right << setfill(' ') << proc->at << " " << proc->tc << " " << proc->cb << " " << proc->io << " " << proc->static_prio << " | " << proc->ft
             << " " << proc->ft - proc->at << " " << proc->it << " " << proc->cw << endl;
        delete proc;
        res.pop();
    }
    cout << "SUM: " << CURRENT_TIME << " " << fixed << setprecision(2)
         << 100 * (static_cast<double>(total_cpu) / CURRENT_TIME) << " "
         << 100 * (static_cast<double>(totalb) / CURRENT_TIME) << " "
         << static_cast<double>(total_turnaround) / proclen << " "
         << static_cast<double>(total_cw) / proclen << " " << setprecision(3)
         << 100 * static_cast<double>(proclen) / CURRENT_TIME << endl;
    return 0;
}
