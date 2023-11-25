#include <iostream>
#include <vector>
#include <fstream>
#include <sstream>
#include <getopt.h>
#include <deque>
#include <math.h>
#include <climits>

using namespace std;

// Define constants
const int MAX_FRAMES = 128;
const int MAX_VPAGES = 64;
const int TAU = 49;
vector<int> randvals;
int num_frames;
int inst_count = 0;
int ctx_swiches = 0;
int process_exits = 0;
int pg_out = 0;
int frame_out = 0;
int ofs = 1;

int myrandom()
{
    int result;
    if (ofs > randvals[0])
    {
        ofs = 1;
    }
    result = randvals[ofs] % num_frames;
    ofs++;
    return result;
}

// Define a structure for a page table entry (PTE)
typedef struct
{
    // Use bitfields to represent the PTE
    unsigned int present : 1;       // 1 bit for PRESENT/VALID
    unsigned int referenced : 1;    // 1 bit for REFERENCED
    unsigned int modified : 1;      // 1 bit for MODIFIED
    unsigned int write_protect : 1; // 1 bit for WRITE_PROTECT
    unsigned int paged_out : 1;     // 1 bit for PAGEDOUT
    unsigned int frame_number : 7;  // 7 bits for the number of the physical frame (assuming 128 frames)

    // Use the remaining bits (20 bits) for custom usage
    unsigned int valid : 1;
    unsigned int file_mapped : 1;
    unsigned int searched : 1;
    unsigned int custom_bits : 17;
} pte_t;

// Constructor for pte_t
pte_t new_PTE()
{
    pte_t newPTE;
    newPTE.present = 0;
    newPTE.referenced = 0;
    newPTE.modified = 0;
    newPTE.write_protect = 0;
    newPTE.paged_out = 0;
    newPTE.frame_number = 0;

    newPTE.valid = 0;
    newPTE.searched = 0;
    newPTE.file_mapped = 0;
    newPTE.custom_bits = 0;
    return newPTE;
}

// Define a structure for a frame in the frame table
typedef struct
{
    int process_id;
    int virtual_page;
    int frame_index;
    int is_victim;
    int mapped;
    unsigned int age : 32;
    // Add more fields as needed
} frame_t;

typedef struct
{
    int starting_virtual_page;
    int ending_virtual_page;
    int write_protected;
    int filemapped;
} vma;

class Process
{
public:
    int pid;
    vector<vma *> vmas;
    pte_t page_table[MAX_VPAGES];
    int unmaps;
    int maps;
    int ins;
    int outs;
    int fins;
    int fouts;
    int zeros;
    int segv;
    int segprot;
    Process()
    {
        unmaps = 0;
        maps = 0;
        ins = 0;
        outs = 0;
        fins = 0;
        fouts = 0;
        zeros = 0;
        segv = 0;
        segprot = 0;
    }
};

deque<int> free_frames;
frame_t frame_table[MAX_FRAMES];
vector<Process *> processes;
Process *current_process = nullptr;

// Define a class for the pager (base class for replacement algorithms)
class Pager
{
public:
    virtual frame_t *select_victim_frame() = 0;
    virtual void reset_age(frame_t *frame)
    {
    }
};

class FIFO_Pager : public Pager
{
public:
    frame_t *select_victim_frame() override
    {
        if (hand >= num_frames)
        {
            hand = 0;
        }
        frame_t *res = &frame_table[hand];
        res->is_victim = 1;
        hand++;
        return res;
    }

private:
    int hand = 0;
};

class Random_Pager : public Pager
{
public:
    frame_t *select_victim_frame() override
    {
        int index = myrandom();
        frame_t *res = &frame_table[index];
        res->is_victim = 1;
        return res;
    }
};

class Clock_Pager : public Pager
{
public:
    frame_t *select_victim_frame() override
    {
        frame_t *res;
        while (1)
        {
            res = &frame_table[hand];
            if (processes[res->process_id]->page_table[res->virtual_page].referenced)
            {
                processes[res->process_id]->page_table[res->virtual_page].referenced = 0;
                hand++;
                if (hand >= num_frames)
                {
                    hand = 0;
                }
            }
            else
            {
                hand++;
                if (hand >= num_frames)
                {
                    hand = 0;
                }
                break;
            }
        }
        res->is_victim = 1;
        return res;
    }

private:
    int hand = 0;
};

class ESC_Pager : public Pager
{
public:
    frame_t *select_victim_frame() override
    {
        int inst_since_last = inst_count + 1 - last_inst;
        bool reset_rbit;
        bool reset_only = false;
        // int pointer = hand;
        int class0 = -1;
        int class1 = -1;
        int class2 = -1;
        int class3 = -1;
        frame_t *res;
        if (inst_since_last > TAU)
        {
            reset_rbit = true;
            last_inst = inst_count + 1;
        }
        else
        {
            reset_rbit = false;
        }

        for (int i = 0; i < num_frames; i++)
        {
            if (reset_only)
            {
                processes[frame_table[hand].process_id]->page_table[frame_table[hand].virtual_page].referenced = 0;
            }
            else
            {

                if (!processes[frame_table[hand].process_id]->page_table[frame_table[hand].virtual_page].referenced)
                {
                    if (!processes[frame_table[hand].process_id]->page_table[frame_table[hand].virtual_page].modified)
                    {
                        if (class0 == -1)
                        {
                            class0 = hand;
                        }
                        if (reset_rbit)
                        {
                            reset_only = true;
                        }
                        else
                        {
                            break;
                        }
                    }
                    else
                    {
                        if (class1 == -1)
                        {
                            class1 = hand;
                        }
                    }
                }
                else
                {
                    if (!processes[frame_table[hand].process_id]->page_table[frame_table[hand].virtual_page].modified)
                    {
                        if (class2 == -1)
                        {
                            class2 = hand;
                        }
                    }
                    else
                    {
                        if (class3 == -1)
                        {
                            class3 = hand;
                        }
                    }
                }
                if (reset_rbit)
                {
                    processes[frame_table[hand].process_id]->page_table[frame_table[hand].virtual_page].referenced = 0;
                }
            }
            hand++;
            if (hand >= num_frames)
            {
                hand = 0;
            }
        }

        if (class0 != -1)
        {
            hand = class0;
        }
        else if (class1 != -1)
        {
            hand = class1;
        }
        else if (class2 != -1)
        {
            hand = class2;
        }
        else
        {
            hand = class3;
        }
        res = &frame_table[hand];
        hand++;
        if (hand >= num_frames)
        {
            hand = 0;
        }
        res->is_victim = 1;
        return res;
    }

private:
    int last_inst;
    int hand = 0;
};

class Aging_Pager : public Pager
{
public:
    frame_t *select_victim_frame() override
    {
        int ptr = hand;
        frame_t *res;
        unsigned int min_age = UINT_MAX;

        // Iterate through all frames to find the frame with the minimum age
        for (int i = 0; i < num_frames; i++)
        {
            res = &frame_table[ptr];
            res->age >>= 1;
            res->age += processes[res->process_id]->page_table[res->virtual_page].referenced * 0x80000000;
            if (res->age < min_age)
            {
                min_age = res->age;
                hand = ptr;
            }
            processes[res->process_id]->page_table[res->virtual_page].referenced = 0;
            ptr++;
            if (ptr >= num_frames)
            {
                ptr = 0;
            }
        }
        res = &frame_table[hand];
        res->is_victim = 1;
        hand++;
        if (hand >= num_frames)
        {
            hand = 0;
        }
        return res;
    }

    void reset_age(frame_t *frame) override
    {
        frame->age = 0;
    }

private:
    int hand = 0;
};

class WorkingSet_Pager : public Pager
{
public:
    frame_t *select_victim_frame() override
    {
        int min_last_inst = INT_MAX;
        bool scan = true;
        int ptr = hand;
        frame_t *res;
        for (int i = 0; i < num_frames; i++)
        {
            res = &frame_table[ptr];
            if (scan)
            {
                if ((inst_count + 1 - res->age) > TAU && processes[res->process_id]->page_table[res->virtual_page].referenced == 0)
                {
                    // stop the scan when algo found the first frame where the referenced is not set and the instruction since last use pass 49.
                    scan = false;
                }
                else if (res->age < min_last_inst && processes[res->process_id]->page_table[res->virtual_page].referenced == 0)
                {
                    hand = ptr;
                    min_last_inst = res->age;
                }
            }
            processes[res->process_id]->page_table[res->virtual_page].referenced = 0;
            ptr++;
            if (ptr >= num_frames)
            {
                ptr = 0;
            }
        }
        res = &frame_table[hand];
        res->is_victim = 1;
        hand++;
        if (hand >= num_frames)
        {
            hand = 0;
        }
        return res;
    }

    void reset_age(frame_t *frame) override
    {
        frame->age = inst_count + 1;
    }

private:
    int hand = 0;
};

Pager *THE_PAGER;

frame_t *allocate_frame_from_free_list()
{
    if (free_frames.empty())
    {
        return nullptr;
    }
    int res = free_frames.front();
    free_frames.pop_front();
    return &frame_table[res];
}

frame_t *get_frame()
{
    frame_t *frame = allocate_frame_from_free_list();
    if (frame == nullptr)
        frame = THE_PAGER->select_victim_frame();
    return frame;
}

string pgfault_handler(pte_t *pte, int vpage)
{
    // first determine vpage can be accessed
    if (!pte->searched)
    {
        for (auto vma : current_process->vmas)
        {
            if (vpage >= vma->starting_virtual_page && vpage <= vma->ending_virtual_page)
            {
                pte->valid = 1;
                pte->file_mapped = vma->filemapped;
                pte->write_protect = vma->write_protected;
                break;
            }
        }
        pte->searched = 1;
    }
    if (pte->valid)
    {
        pte->present = 1;
        return string();
    }
    current_process->segv++;
    return "SEGV";
}

void print_pgtable(pte_t page_table[], int pid)
{
    cout << "PT[" << pid << "]: ";
    for (int i = 0; i < MAX_VPAGES; i++)
    {
        if (page_table[i].present)
        {
            cout << i << ":";
            if (page_table[i].referenced)
            {
                cout << "R";
            }
            else
            {
                cout << "-";
            }

            if (page_table[i].modified)
            {
                cout << "M";
            }
            else
            {
                cout << "-";
            }

            if (page_table[i].paged_out)
            {
                cout << "S";
            }
            else
            {
                cout << "-";
            }
        }
        else
        {
            if (page_table[i].paged_out)
            {
                cout << "#";
            }
            else
            {
                cout << "*";
            }
        }
        cout << " ";
    }
    cout << endl;
    return;
}

void print_ftable()
{
    cout << "FT: ";
    for (int i = 0; i < num_frames; i++)
    {
        if (frame_table[i].mapped)
        {
            cout << frame_table[i].process_id << ":" << frame_table[i].virtual_page << " ";
        }
        else
        {
            cout << "* ";
        }
    }
    cout << endl;
    return;
}

void simulation(ifstream &inputFile)
{
    string line;
    char operation;
    int vpage;
    while (getline(inputFile, line))
    {
        if (line[0] == '#')
        {
            continue;
        }

        istringstream iss(line);
        iss >> operation >> vpage;
        cout << inst_count << ": ==> " << operation << " " << vpage << endl;
        // handle special case of “c” and “e” instruction
        // now the real instructions for read and write
        if (operation == 'c')
        {
            ctx_swiches++;
            current_process = processes[vpage];
        }
        else if (operation == 'e')
        {
            cout << "EXIT current process " << vpage << endl;
            process_exits++;
            for (int i = 0; i < MAX_VPAGES; i++)
            {
                pte_t *pte = &current_process->page_table[i];
                pte->paged_out = 0;
                if (pte->present)
                {
                    cout << " UNMAP " << vpage << ":" << i << endl;
                    current_process->unmaps++;
                    pte->present = 0;
                    if (pte->modified && pte->file_mapped)
                    {
                        cout << " FOUT" << endl;
                        current_process->fouts++;
                    }
                    frame_table[pte->frame_number].mapped = 0;
                    free_frames.push_back(pte->frame_number);
                }
            }
        }
        else
        {
            pte_t *pte = &current_process->page_table[vpage];
            if (!pte->present)
            {
                string segerror = pgfault_handler(pte, vpage);
                if (!segerror.empty())
                {
                    cout << segerror << endl;
                    inst_count++;
                    continue;
                }
                // this in reality generates the page fault exception and now you execute
                // verify this is actually a valid page in a vma if not raise error and next inst
                frame_t *newframe = get_frame();
                //-> figure out if/what to do with old frame if it was mapped
                // see general outline in MM-slides under Lab3 header and writeup below
                // see whether and how to bring in the content of the access page.
                if (newframe->is_victim)
                {
                    cout << " UNMAP " << newframe->process_id << ":" << newframe->virtual_page << endl;
                    processes[newframe->process_id]->page_table[newframe->virtual_page].present = 0;
                    processes[newframe->process_id]->unmaps++;
                    newframe->is_victim = 0;
                    if (processes[newframe->process_id]->page_table[newframe->virtual_page].modified)
                    {
                        if (processes[newframe->process_id]->page_table[newframe->virtual_page].file_mapped)
                        {
                            cout << " FOUT" << endl;
                            processes[newframe->process_id]->fouts++;
                        }
                        else
                        {
                            cout << " OUT" << endl;
                            processes[newframe->process_id]->page_table[newframe->virtual_page].modified = 0;
                            processes[newframe->process_id]->page_table[newframe->virtual_page].paged_out = 1;
                            processes[newframe->process_id]->outs++;
                        }
                    }
                    if (!processes[newframe->process_id]->page_table[newframe->virtual_page].paged_out)
                    {
                        processes[newframe->process_id]->page_table[newframe->virtual_page].modified = 0;
                    }
                    processes[newframe->process_id]->page_table[newframe->virtual_page].frame_number = 0;
                    newframe->virtual_page = 0;
                    newframe->mapped = 0;
                }
                if (pte->file_mapped)
                {
                    cout << " FIN" << endl;
                    current_process->fins++;
                }
                else if (pte->paged_out)
                {
                    cout << " IN" << endl;
                    pte->present = 1;
                    current_process->ins++;
                }
                else
                {
                    cout << " ZERO" << endl;
                    current_process->zeros++;
                }
                cout << " MAP " << newframe->frame_index << endl;
                THE_PAGER->reset_age(newframe);
                newframe->virtual_page = vpage;
                newframe->mapped = 1;
                newframe->process_id = current_process->pid;
                pte->frame_number = newframe->frame_index;
                current_process->maps++;
            }
            pte->referenced = 1;
            if (operation == 'w')
            {
                if (pte->write_protect)
                {
                    cout << " SEGPROT" << endl;
                    current_process->segprot++;
                }
                else
                {
                    pte->modified = 1;
                }
            }
            if (pg_out)
            {
                print_pgtable(current_process->page_table, current_process->pid);
            }
            if (frame_out)
            {
                print_ftable();
            }
        }
        inst_count++;
    }
    return;
}

int main(int argc, char *argv[])
{
    // Parse command-line arguments and initialize VMM
    // Create an instance of VMM and run the simulation
    int opt;
    const char *optstring = "a:o:f:";
    string algo, options, inputfile, randfile;
    while ((opt = getopt(argc, argv, optstring)) != -1)
    {
        switch (opt)
        {
        case 'o':
            options = optarg;
            break;
        case 'f':
            num_frames = stoi(optarg);
            break;
        case 'a':
            algo = optarg;
            break;
        default:
            cerr << "Error: Unknown option or invalid usage." << endl;
            return 1; // Return an error code for unknown options
        }
    }

    if (options.find('x') != string::npos)
    {
        // turn on the page table output
        pg_out = 1;
    }

    if (options.find('f') != string::npos)
    {
        // turn on the frame table output
        frame_out = 1;
    }

    // initializing Pager algorithms
    if (algo == "f")
    {
        THE_PAGER = new FIFO_Pager();
    }
    else if (algo == "r")
    {
        THE_PAGER = new Random_Pager();
    }
    else if (algo == "c")
    {
        THE_PAGER = new Clock_Pager();
    }
    else if (algo == "e")
    {
        THE_PAGER = new ESC_Pager();
    }
    else if (algo == "a")
    {
        THE_PAGER = new Aging_Pager();
    }
    else if (algo == "w")
    {
        THE_PAGER = new WorkingSet_Pager();
    }
    else
    {
        cerr << "undefined algorithm";
        exit(1);
    }

    // initialize free frame
    for (int i = 0; i < num_frames; i++)
    {
        frame_table[i].frame_index = i;
        free_frames.push_back(i);
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

    // read random file
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

    // read input file
    ifstream inputFile(inputfile);
    string line;
    int proc_count;
    while (getline(inputFile, line))
    {
        if (line[0] != '#')
        {
            proc_count = stoi(line);
            break;
        }
    }
    for (int i = 0; i < proc_count; i++)
    {
        int vma_count;
        Process *process = new Process();
        process->pid = i;
        for (int j = 0; j < MAX_VPAGES; j++)
        {
            process->page_table[j] = new_PTE();
        }
        while (getline(inputFile, line))
        {
            if (line[0] != '#')
            {
                vma_count = stoi(line);
                break;
            }
        }
        for (int j = 0; j < vma_count; j++)
        {
            vma *new_v = new vma;
            getline(inputFile, line);
            istringstream iss(line);
            iss >> new_v->starting_virtual_page >> new_v->ending_virtual_page >> new_v->write_protected >> new_v->filemapped;
            process->vmas.push_back(new_v);
        }
        processes.push_back(process);
    }

    simulation(inputFile);

    for (auto proc : processes)
    {
        print_pgtable(proc->page_table, proc->pid);
    }
    print_ftable();
    unsigned long long cost = 0;
    for (auto proc : processes)
    {
        cout << "PROC[" << proc->pid << "]:"
             << " U=" << proc->unmaps << " M=" << proc->maps << " I=" << proc->ins
             << " O=" << proc->outs << " FI=" << proc->fins << " FO=" << proc->fouts
             << " Z=" << proc->zeros << " SV=" << proc->segv << " SP=" << proc->segprot << endl;
        cost += proc->unmaps * 410 + proc->maps * 350 + proc->ins * 3200 +
                proc->outs * 2750 + proc->fins * 2350 + proc->fouts * 2800 + proc->zeros * 150 +
                proc->segv * 440 + proc->segprot * 410;
    }
    cost += inst_count - ctx_swiches - process_exits + ctx_swiches * 130 + process_exits * 1230;
    cout << "TOTALCOST " << inst_count << " " << ctx_swiches
         << " " << process_exits << " " << cost << " " << sizeof(pte_t) << endl;
    cout << "num of processes: " << processes.size() << endl;
    for (auto proc : processes)
    {
        cout << proc->pid << endl;
        for (auto vma : proc->vmas)
        {
            cout << vma->starting_virtual_page << " " << vma->ending_virtual_page << " " << vma->write_protected << " " << vma->filemapped << endl;
            delete vma;
        }
        proc->vmas.clear();
        delete proc;
    }
    processes.clear();
    inputFile.close();
    delete THE_PAGER;
    cout << "Simulation finished" << endl;
    return 0;
}