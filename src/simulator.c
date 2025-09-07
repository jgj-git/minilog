#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

#define MAX_SIM_TIME 10

///////////////////////////////////////////////////////
///////////////// TYPE DEFINITIONS ////////////////////
///////////////////////////////////////////////////////
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
typedef enum {CONSTANT_ASSIGNMENT, CONTINUOUS_ASSIGNMENT} event_type_e;
struct sim_event_t{
    struct logic_t* lhs;
    struct logic_t* rhs;
    struct sim_event_t* prev_event;
    struct sim_event_t* next_event;
    event_type_e type;
};
// time slot type
struct sim_time_slot_t {
    struct sim_time_slot_t* next_time_slot;
    struct sim_time_slot_t* prev_time_slot;
    struct sim_event_t* events;
    int unsigned sim_time;
};
// continuous assignment type
struct sim_thread_t {
    pthread_t *thread;
    struct logic_t *rhs;
    struct logic_t *lhs;
};
struct sim_cont_assignment_t {
    struct sim_thread_t *prev_thread;
    struct sim_thread_t *next_thread;
    struct sim_thread_t thread;
};

///////////////////////////////////////////////////////
///////////////// GLOBAL VARIABLES ////////////////////
///////////////////////////////////////////////////////
struct sim_time_slot_t *sim_time_slots;
// TODO: change to doubly linked list so we can free pointer space when 
// assigning constants form tb
struct logic_t *wires;
int unsigned nwires;
int unsigned sim_time;

///////////////////////////////////////////////////////
///////////////// GLOBAL FUNCTIONS ////////////////////
///////////////////////////////////////////////////////
void initialize_sim() {
    printf("Initializing simulation globals...\n");
    sim_time = 0;
    // time slots
    sim_time_slots = malloc(sizeof(struct sim_time_slot_t));
    sim_time_slots->sim_time = 0;
    sim_time_slots->events = NULL; 
    sim_time_slots->next_time_slot = NULL;
    sim_time_slots->prev_time_slot = NULL;
    // wires
    nwires = 0;
}

void add_event_to_list(struct sim_event_t **event_list, 
                       struct sim_event_t* event) {
    event->prev_event = NULL;
    event->next_event = *event_list;
    if (*event_list != NULL)
        (*event_list)->prev_event = event;
    *event_list = event;
}

void schedule_event(struct sim_event_t* event, int unsigned sim_time) {
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

struct logic_t * declare_wire(logic_e init_value) {
    ++nwires;
    wires = realloc(wires, nwires*sizeof(struct logic_t));
    wires[nwires-1].value = init_value;
    wires[nwires-1].from = NULL;
    wires[nwires-1].to = NULL;
    return &(wires[nwires-1]);
}

void assign_const_to_wire(struct logic_t* lhs, logic_e rhs_val, 
                          int unsigned sim_time)
{
    struct sim_event_t* event = malloc(sizeof(struct sim_event_t));
    struct logic_t* rhs = declare_wire(rhs_val) ;
    //if (lhs->from != NULL) {
    //    free_wire(lhs->from);
    //}
    lhs->from = rhs;
    rhs->to = lhs;
    event->lhs = lhs;
    event->rhs = rhs;
    event->type = CONSTANT_ASSIGNMENT;
    schedule_event(event, sim_time);
}

void cont_assign_wires(
    struct logic_t *lhs, struct logic_t *rhs
) {
    struct sim_event_t *event = malloc(sizeof(struct sim_event_t));
    event->lhs = lhs;
    event->rhs = rhs;
    event->type = CONTINUOUS_ASSIGNMENT;
    schedule_event(event, 1);
}

void execute_event(struct sim_event_t *event) {
    printf("\tEvent scheduled in timeslot %d: from %s to %s\n",
        sim_time, 
        get_logic_name(event->lhs->value), 
        get_logic_name(event->rhs->value));
    switch(event->type) {
        case CONSTANT_ASSIGNMENT: {
            event->lhs->value = event->rhs->value;
            break;
        }
        case CONTINUOUS_ASSIGNMENT: {
            event->lhs->value = event->rhs->value;
            struct sim_event_t *new_event = malloc(sizeof(struct sim_event_t));
            new_event->rhs = event->rhs;
            new_event->lhs = event->lhs;
            new_event->type = CONTINUOUS_ASSIGNMENT;
            schedule_event(new_event, sim_time+1);
            break;
        }
    }
}

void start_simulation() {
    printf("Starting simulation...\n");
    struct sim_time_slot_t* current_time_slot;
    current_time_slot = sim_time_slots;
    // TODO: figure out how to make simulation end when continuous assignments 
    // are used
    while (current_time_slot != NULL && sim_time <= MAX_SIM_TIME) {
        printf("\nTimeslot %d\n", current_time_slot->sim_time);
        struct sim_event_t* current_event = current_time_slot->events;
        while (current_event != NULL) {
            execute_event(current_event);
            current_event = current_event->next_event;
            // free event
        } 
        current_time_slot = current_time_slot->next_time_slot;
        ++sim_time;
    }
    // free elements sim_time_slots
}
