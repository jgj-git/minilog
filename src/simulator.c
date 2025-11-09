#include <pthread.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

#define MAX_SIM_TIME 10
#define ACTIVE_REGION_INDEX 0
#define NBA_REGION_INDEX 1
#define NUM_REGIONS 2
#define DYN_ARRAY_SECTION_SIZE 10

///////////////////////////////////////////////////////
///////////////// TYPE DEFINITIONS ////////////////////
///////////////////////////////////////////////////////
typedef enum { LOW = 0, HIGH = 1, X, Z } logic_e;
typedef enum { BLOCKING = 0, NON_BLOCKING } assign_type_e;
typedef int unsigned wire_id_t;

// wire type
// NOTE: this makes it so a wire cannot be on the LHS of blocking and non 
// blocking assignments at the same type, which technically breaks SV spec
// but you never really want to do that anyway
struct logic_t {
    wire_id_t *to;
    int unsigned to_num;
    int unsigned id;
    logic_e value;
    assign_type_e assign_type;
};

// event type
typedef enum { CONSTANT_ASSIGNMENT, CONTINUOUS_ASSIGNMENT, NBA_EVAL, NBA_UPDATE } event_type_e;

struct sim_event_t {
    struct logic_t *lhs;
    struct logic_t *rhs;
    struct sim_event_t *next_event;
    event_type_e type;
    logic_e rhs_const_val;
};

struct sim_event_t *create_sim_event(
    struct logic_t *lhs,
    struct logic_t *rhs,
    event_type_e type
) {
    struct sim_event_t *new_event = (struct sim_event_t*) malloc(sizeof(struct sim_event_t));
    new_event->type = type;
    new_event->next_event = NULL;
    switch (type) {
        case CONTINUOUS_ASSIGNMENT : {
            new_event->rhs = rhs;
            break;
        }
        case CONSTANT_ASSIGNMENT : {
            new_event->rhs_const_val = rhs->value;
            break;
        }
        case NBA_EVAL : {
            new_event->rhs = rhs;
            break;
        }
        case NBA_UPDATE : {
            new_event->rhs_const_val = rhs->value;
            break;
        }
        default : {
            printf("[create_sim_event] ERROR: INVALID EVENT TYPE\n");
            exit(1);
        }
    }
    new_event->lhs = lhs;
    return new_event;
}

// time slot type
struct sim_time_slot_t {
    struct sim_time_slot_t *next_time_slot;
    int unsigned sim_time;
    struct sim_event_t *regions[NUM_REGIONS];
};

struct sim_time_slot_t *create_sim_time_slot(int unsigned new_sim_time) {
    struct sim_time_slot_t *new_time_slot = malloc(sizeof(struct sim_time_slot_t));
    new_time_slot->sim_time = new_sim_time;
    for (int i = 0; i < NUM_REGIONS; ++i) new_time_slot->regions[i] = NULL;
    new_time_slot->next_time_slot = NULL;
    return new_time_slot;
}

void debug_sim_time_slot(struct sim_time_slot_t *time_slot) {
    if (time_slot == NULL) {
        return;
    }
    printf("Time slot time: %d. Next:\n",
           time_slot->sim_time);
    debug_sim_time_slot(time_slot->next_time_slot);
};

///////////////////////////////////////////////////////
///////////////// GLOBAL VARIABLES ////////////////////
///////////////////////////////////////////////////////
struct sim_time_slot_t *sim_time_slots;
struct sim_event_t *ini_events;
struct logic_t *wires;
int unsigned nwires;
pthread_t main_thread_id;
pthread_t *process_pool;

///////////////////////////////////////////////////////
///////////////// GLOBAL FUNCTIONS ////////////////////
///////////////////////////////////////////////////////
struct logic_t *get_wire(wire_id_t id) {
    return &(wires[id]);
}

void initialize_sim() {
    printf("Initializing simulation globals...\n");
    // time slots
    sim_time_slots = create_sim_time_slot(0);
    // wires
    nwires = 0;
}

void add_event_to_list(struct sim_event_t **event_list,
                       struct sim_event_t *event) {
    event->next_event = *event_list;
    *event_list = event;
}


void schedule_event(
    struct sim_event_t *event, int unsigned region_index, int unsigned _sim_time
) {
    if (sim_time_slots == NULL)
        return;
    struct sim_time_slot_t *iter = sim_time_slots;
    struct sim_time_slot_t *prev_iter = NULL;
    // find time slot
    while (iter != NULL) {
        if (iter->sim_time == _sim_time) {
            add_event_to_list(&(iter->regions[region_index]), event);
            return;
        } 
        else if (iter->sim_time > _sim_time) {
            prev_iter->next_time_slot = create_sim_time_slot(_sim_time);
            prev_iter->next_time_slot->next_time_slot = iter;
            prev_iter->next_time_slot->sim_time = _sim_time;
            add_event_to_list(
                &(prev_iter->next_time_slot->regions[region_index]), event
            );
            return;
        }
        prev_iter = iter;
        iter = iter->next_time_slot;
    }
    prev_iter->next_time_slot = create_sim_time_slot(_sim_time);
    add_event_to_list(&(prev_iter->next_time_slot->regions[region_index]), event);
}

// NBA by default, this is what is normally used in TB to stimulate DUT
void _assign_const_to_wire(struct logic_t *lhs, logic_e rhs_val,
                          int unsigned sim_time, assign_type_e type) {
    struct logic_t rhs;
    rhs.value = rhs_val;
    struct sim_event_t *event = create_sim_event(lhs, &rhs, CONSTANT_ASSIGNMENT);
    schedule_event(event, ACTIVE_REGION_INDEX, sim_time);
    lhs->assign_type = type;
}


void ini_wire(struct logic_t *lhs, logic_e rhs_val) {
    struct logic_t rhs;
    rhs.value = rhs_val;
    struct sim_event_t *event = create_sim_event(lhs, &rhs, CONSTANT_ASSIGNMENT);
    add_event_to_list(&ini_events, event);
}

wire_id_t declare_wire(logic_e init_value) {
    ++nwires;
    wires = realloc(wires, nwires * sizeof(struct logic_t));
    wires[nwires - 1].to = NULL;
    wires[nwires - 1].to_num = 0;
    wires[nwires - 1].id = nwires-1;
    ini_wire(&(wires[nwires - 1]), init_value);
    return nwires - 1;
}

// NBA by default, this is what is normally used in TB to stimulate DUT
void assign_const_to_wire(wire_id_t lhs_id, logic_e rhs_val,
                          int unsigned sim_time) {
    _assign_const_to_wire(get_wire(lhs_id), rhs_val, sim_time, NON_BLOCKING);
}


void blocking_assign_const_to_wire(wire_id_t lhs_id, logic_e rhs_val,
                          int unsigned sim_time) {
    _assign_const_to_wire(get_wire(lhs_id), rhs_val, sim_time, BLOCKING);
}


void cont_assign_wires(wire_id_t lhs_id, wire_id_t rhs_id) {
    struct logic_t *lhs = get_wire(lhs_id);
    struct logic_t *rhs = get_wire(rhs_id);
    ++rhs->to_num;
    rhs->to = (wire_id_t *)realloc(rhs->to, rhs->to_num*sizeof(wire_id_t));
    rhs->to[rhs->to_num-1] = lhs->id;
    lhs->assign_type = BLOCKING;
}

void nb_assign_wires(wire_id_t lhs_id, wire_id_t rhs_id) {
    struct logic_t *lhs = get_wire(lhs_id);
    struct logic_t *rhs = get_wire(rhs_id);
    ++rhs->to_num;
    rhs->to = (wire_id_t *) 
        realloc(rhs->to, rhs->to_num*sizeof(wire_id_t));
    rhs->to[rhs->to_num-1] = lhs->id;
    lhs->assign_type = NON_BLOCKING;
}

// we only execute events from the active region
void execute_event(struct sim_event_t *event) {
    logic_e old_lhs_val = event->lhs->value;
    switch (event->type) {
        case CONSTANT_ASSIGNMENT: {
            event->lhs->value = event->rhs_const_val;
            break;
        }
        case CONTINUOUS_ASSIGNMENT: {
            event->lhs->value = event->rhs->value;
            break;
        }
        case NBA_EVAL: {
            struct sim_event_t *nba_update_ev = 
                create_sim_event(event->lhs, event->rhs, NBA_UPDATE);
            schedule_event(nba_update_ev, NBA_REGION_INDEX, sim_time_slots->sim_time);
            return;
        }
        case NBA_UPDATE: {
            event->lhs->value = event->rhs_const_val;
            break;
        }
        default: {
            printf("Event execution error: invalid event type!\n");
            return;
        }
    }
    printf("\tEvent executed in timeslot %d: wire %d went from %d to %d\n", 
           sim_time_slots->sim_time, event->lhs->id, old_lhs_val, event->lhs->value);
    if (event->lhs->to != NULL){ 
        int lhs_changed = old_lhs_val != event->lhs->value;
        for (int i = 0; i < event->lhs->to_num; ++i) {
            if ((!lhs_changed) && event->lhs->value == get_wire(event->lhs->to[i])->value) 
                continue;
            struct sim_event_t *new_event;
            if (get_wire(event->lhs->to[i])->assign_type == BLOCKING) { 
                new_event = create_sim_event(get_wire(event->lhs->to[i]), 
                                             event->lhs, CONTINUOUS_ASSIGNMENT
                                             );
            }
            else {
                new_event = create_sim_event(get_wire(event->lhs->to[i]),
                                             event->lhs, NBA_EVAL);
            }
            schedule_event(new_event, ACTIVE_REGION_INDEX, sim_time_slots->sim_time);
        }
    }
}

void execute_event_list(struct sim_event_t **event_list) {
    struct sim_event_t *current_event;
    while (*event_list != NULL) {
        current_event = *event_list;
        *event_list = (*event_list)->next_event;
        execute_event(current_event);
        free(current_event);
    }
}

void execute_active_region() {
    execute_event_list(&(sim_time_slots->regions[ACTIVE_REGION_INDEX]));
}

int all_regions_empty(struct sim_time_slot_t* time_slot) {
    for (int i = 0; i < NUM_REGIONS; ++i) {
        if (time_slot->regions[i]) return 0;
    }
    return 1;
}

void move_region_to_active(struct sim_time_slot_t* time_slot, int unsigned index) {
    time_slot->regions[0] = time_slot->regions[index];
    time_slot->regions[index] = NULL;
}

void *main_sim_thread() {
    struct sim_time_slot_t *prev_time_slot;
    int unsigned delta_cycle_counter;
    execute_event_list(&ini_events);
    while (sim_time_slots != NULL && sim_time_slots->sim_time <= MAX_SIM_TIME) {
        printf("\nTimeslot %d\n", sim_time_slots->sim_time);
        delta_cycle_counter = 0;
        while(!all_regions_empty(sim_time_slots)) {
            printf("Starting delta cycle %d\n", delta_cycle_counter);
            for (int unsigned i = 0; i < NUM_REGIONS; ++i) {
                if (i != 0) move_region_to_active(sim_time_slots, i);
                execute_active_region();
            }
            delta_cycle_counter++;
        }
        prev_time_slot = sim_time_slots;
        sim_time_slots = sim_time_slots->next_time_slot;
        free(prev_time_slot);
    }
    return NULL;
}

void start_simulation() {
    printf("Starting simulation...\n");
    if (pthread_create(&main_thread_id, NULL, main_sim_thread(), NULL)) {
        printf("The simulation thread failed. Panic!!!\n");
        exit(1);
    }
}

// TODO: every "always_comb" block is defined as a function in the testbench
// void *foo(wire_id_t *in_wires, wire_id_t *out_wires). The simulator maintains 
// a dynamic array of the following struct:
// struct comb_process_item_t {
//      pthread_t thread_id;
//      wire_id_t *in_wires;
//      wire_id_t *out_wires;
// }
// The array index of this item is added to the to_process field in each of the 
// in_wires.
// Have to account for intersection between in_wires and out_wires, or define
// inout wire array.
//
// Every "always_ff" block is defined as a function in the testbench:
// void *foo_ff(wire_id_t *posedge_wires, wire_id_t *negedge_wires,  
//      wire_id_t *out_wires)
// The simulator maintatins a dynamic array of the following struct:
// struct ff_process_item_t {
//      pthread_t thread_id;
//      wire_id_t *posedge_wires;
//      wire_id_t *negedge_wires;
//      wire_id_t *out_wires;
// }
// The array index of this item is added to the to_process field in each of the 
// posedge_wires and negedge_wires.
//
// For general "always", that can contain waits and delays, need additional
// pthread_mutex_t * parameters.

void add_process_to_pool(pthread_t process_id) {
    process_pool = (pthread_t *)realloc(process_pool, (sizeof(pthread_t *)));
}

