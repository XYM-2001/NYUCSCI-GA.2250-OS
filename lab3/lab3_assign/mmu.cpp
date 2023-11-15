#include <iostream>
#include <vector>
#include <fstream>
#include <sstream>
#include <getopt.h>

using namespace std;

// Define constants
const int MAX_FRAMES = 128;
const int MAX_VPAGES = 64;
vector<int> randvals;
int num_frames;

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
    unsigned int file_mapped : 1;
    unsigned int valid_vma : 1;
    unsigned int searched : 1;
    unsigned int custom_bits : 18;
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

    newPTE.searched = 0;
    newPTE.valid_vma = 0;
    newPTE.file_mapped = 0;
    newPTE.custom_bits = 0;
    return newPTE;
}

// Define a structure for a frame in the frame table
typedef struct
{
    int process_id;
    int virtual_page;
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
};

// Define a class for the pager (base class for replacement algorithms)
class Pager
{
public:
    virtual frame_t *select_victim_frame() = 0;
};

class FIFO_Pager : public Pager
{
public:
    frame_t *select_victim_frame() override
    {
    }
};

frame_t frame_table[MAX_FRAMES];
vector<Process *> processes;
Process *current_process = nullptr;
Pager *THE_PAGER;

frame_t *allocate_frame_from_free_list()
{
}

frame_t *get_frame()
{
    frame_t *frame = allocate_frame_from_free_list();
    if (frame == NULL)
        frame = THE_PAGER->select_victim_frame();
    return frame;
}

void pgfault_handler(pte_t *pte, int vpage)
{
    if (!pte->searched)
    {
        for (auto vma : current_process->vmas)
        {
            if (vpage >= vma->starting_virtual_page && vpage <= vma->ending_virtual_page)
            {
                pte->valid_vma = 1;
                break;
            }
        }
        pte->searched = 1;
    }
}

void Simulation(ifstream &inputFile)
{
    string line;
    char operation;
    int vpage;
    int ins = 0;
    while (getline(inputFile, line))
    {
        if (line[0] == '#')
        {
            continue;
        }

        istringstream iss(line);
        iss >> operation >> vpage;
        cout << ins << ": ==> " << operation << " " << vpage << endl;
        // handle special case of “c” and “e” instruction
        // now the real instructions for read and write
        if (operation == 'c')
        {
            current_process = processes[vpage];
            for (auto vma : current_process->vmas)
            {
                for (int i = vma->starting_virtual_page; i <= vma->ending_virtual_page; i++)
                {
                    current_process->page_table[i].write_protect = vma->write_protected;
                    current_process->page_table[i].file_mapped = vma->filemapped;
                }
            }
        }
        pte_t *pte = &current_process->page_table[vpage];
        if (!pte->present)
        {
            pgfault_handler(pte, vpage);
            // this in reality generates the page fault exception and now you execute
            // verify this is actually a valid page in a vma if not raise error and next inst
            frame_t *newframe = get_frame();
            //-> figure out if/what to do with old frame if it was mapped
            // see general outline in MM-slides under Lab3 header and writeup below
            // see whether and how to bring in the content of the access page.
        }

        ins++;
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

    // initialize frame table
    for (int i = 0; i < num_frames; i++)
    {
        frame_table[i].process_id = i;
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
        Process *process = new Process;
        process->pid = i;
        for (int j = 0; j < MAX_VPAGES; j++)
        {
            process->page_table[j] = new_PTE();
        }
        processes.push_back(process);
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
    }
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
    Simulation(inputFile);
    inputFile.close();
    delete THE_PAGER;
    return 0;
}