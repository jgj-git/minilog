#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

#define MAX_SIM_TIME 100

// useful enums
typedef enum {ACTIVE, NBA} sim_region_e;
typedef enum {LOW = 0, HIGH = 1, X, Z} logic_e;
char* get_logic_name(logic_e value) {
    switch (value) {
        case LOW : return "LOW";
        case HIGH : return "HIGH";
        case X : return "X";
        case Z : return "Z";
        default: break;
    }
    return "INVALID_LOGIC_VALUE";
}
// wire type
struct logic_t {
    struct logic_t* from;
    struct logic_t* to;
    logic_e value;
};
// event type
struct sim_event_t{
    struct logic_t* lhs;
    logic_e rhs;
    struct sim_event_t* prev_event;
    struct sim_event_t* next_event;
};
// time slot type
struct sim_time_slot_t {
    struct sim_time_slot_t* next_time_slot;
    struct sim_time_slot_t* prev_time_slot;
    struct sim_event_t* events;
    int unsigned sim_time;
};

///////////////////////////////////////////////////////
///////////////// GLOBAL VARIABLES ////////////////////
///////////////////////////////////////////////////////
struct sim_time_slot_t *sim_time_slots;
struct logic_t *wires;
int unsigned nwires;

// functions
void initialize_sim() {
    printf("Initializing simulation globals...\n");
    sim_time_slots = malloc(sizeof(struct sim_time_slot_t));
    nwires = 0;
    sim_time_slots->sim_time = 0;
    sim_time_slots->events = NULL; 
    sim_time_slots->next_time_slot = NULL;
    sim_time_slots->prev_time_slot = NULL;
}

void add_event_to_list(struct sim_event_t **event_list, 
                       struct sim_event_t* event) {
    event->prev_event = NULL;
    event->next_event = *event_list;
    if (*event_list != NULL)
        (*event_list)->prev_event = event;
    *event_list = event;
}

void schedule_event(struct sim_event_t* event, const int unsigned sim_time, 
                    struct sim_time_slot_t* sim_time_slots) {
    if (sim_time_slots == NULL) return;
    struct sim_time_slot_t* iter = sim_time_slots;
    struct sim_time_slot_t* prev_iter;
    // find time slot
    while (iter != NULL) {
        if (iter->sim_time == sim_time) {
            add_event_to_list(&(iter->events), event);
            return;
        }
        else if (iter->sim_time > sim_time) {
            iter->next_time_slot = malloc(sizeof(struct sim_time_slot_t));
            iter->next_time_slot->prev_time_slot = iter;
            iter->next_time_slot->next_time_slot = NULL;
            iter->next_time_slot->sim_time = sim_time;
            iter->next_time_slot->events = NULL;
            add_event_to_list(&(iter->next_time_slot->events), event);
            return;
        }
        prev_iter = iter;
        iter = iter->next_time_slot;
    }
    prev_iter->next_time_slot = malloc(sizeof(struct sim_time_slot_t));
    prev_iter->next_time_slot->sim_time = sim_time;
    prev_iter->next_time_slot->prev_time_slot = prev_iter;
    prev_iter->next_time_slot->next_time_slot = NULL;
    prev_iter->next_time_slot->events = NULL;
    add_event_to_list(&(prev_iter->next_time_slot->events), event);
}

void assign_const_to_wire(struct logic_t* lhs, logic_e rhs, int unsigned sim_time, 
                         struct sim_time_slot_t* sim_time_slots) {
    struct sim_event_t* event = malloc(sizeof(struct sim_event_t));
    event->lhs = lhs;
    event->rhs = rhs;
    schedule_event(event, sim_time, sim_time_slots);
}

struct logic_t * declare_wire(logic_e init_value) {
    ++nwires;
    wires = realloc(wires, nwires*sizeof(struct logic_t));
    wires[nwires-1].value = init_value;
    return &(wires[nwires-1]);
}

void start_simulation() {
    printf("Starting simulation...\n");
    struct sim_time_slot_t* current_time_slot;
    current_time_slot = sim_time_slots;
    while (current_time_slot != NULL) {
        printf("\nTimeslot %d\n", current_time_slot->sim_time);
        struct sim_event_t* current_event = current_time_slot->events;
        while (current_event != NULL) {
            printf("\tEvent scheduled in timeslot %d: from %s to %s\n",
                current_time_slot->sim_time, get_logic_name(current_event->lhs->value), 
                get_logic_name(current_event->rhs));
            current_event = current_event->next_event;
            // free event
        } 
        current_time_slot = current_time_slot->next_time_slot;
    }
    // free elements sim_time_slots, freeing
}
