#include <pthread.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

#define MAX_SIM_TIME 10
#define ACTIVE_REGION_INDEX 0
#define NBA_REGION_INDEX 1
#define NUM_REGIONS 2

///////////////////////////////////////////////////////
///////////////// TYPE DEFINITIONS ////////////////////
///////////////////////////////////////////////////////
typedef enum { LOW = 0, HIGH = 1, X, Z } logic_e;
typedef enum { BLOCKING = 0, NON_BLOCKING } assign_type_e;

// wire type
// NOTE: this makes it so a wire cannot be on the LHS of blocking and non 
// blocking assignments at the same type, which technically breaks SV spec
// but you never really want to do that anyway
struct logic_t {
    struct logic_t **from;
    struct logic_t **to;
    int unsigned from_num;
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
    struct sim_event_t *new_event = malloc(sizeof(struct sim_event_t));
    new_event->type = type;
    new_event->next_event = NULL;
    switch (type) {
        case CONTINUOUS_ASSIGNMENT : {
            new_event->rhs = rhs;
            break;
        }
        case CONSTANT_ASSIGNMENT : {
            // TODO: prob should delete this line
            lhs->from = NULL;
            new_event->rhs = NULL;
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
struct logic_t *wires;
int unsigned nwires;

///////////////////////////////////////////////////////
///////////////// GLOBAL FUNCTIONS ////////////////////
///////////////////////////////////////////////////////
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
            // TODO: not always to active region
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

struct logic_t *declare_wire(logic_e init_value) {
    ++nwires;
    wires = realloc(wires, nwires * sizeof(struct logic_t));
    wires[nwires - 1].value = init_value;
    wires[nwires - 1].from = NULL;
    wires[nwires - 1].from_num = 0;
    wires[nwires - 1].to = NULL;
    wires[nwires - 1].to_num = 0;
    wires[nwires - 1].id = nwires-1;
    return &(wires[nwires - 1]);
}

// NBA by default, this is what is normally used in TB to stimulate DUT
void assign_const_to_wire(struct logic_t *lhs, logic_e rhs_val,
                          int unsigned sim_time) {
    struct logic_t rhs;
    rhs.value = rhs_val;
    struct sim_event_t *event = create_sim_event(lhs, &rhs, CONSTANT_ASSIGNMENT);
    schedule_event(event, ACTIVE_REGION_INDEX, sim_time);
    lhs->assign_type = NON_BLOCKING;
}


void blocking_assign_const_to_wire(struct logic_t *lhs, logic_e rhs_val,
                          int unsigned sim_time) {
    struct logic_t rhs;
    rhs.value = rhs_val;
    struct sim_event_t *event = create_sim_event(lhs, &rhs, CONSTANT_ASSIGNMENT);
    schedule_event(event, ACTIVE_REGION_INDEX, sim_time);
    lhs->assign_type = BLOCKING;
}


void cont_assign_wires(struct logic_t *lhs, struct logic_t *rhs) {
    ++lhs->from_num;
    lhs->from = realloc(lhs->from, lhs->from_num*sizeof(struct logic_t*));
    lhs->from[lhs->from_num-1] = rhs;
    ++rhs->to_num;
    rhs->to = realloc(rhs->to, rhs->to_num*sizeof(struct logic_t*));
    rhs->to[rhs->to_num-1] = lhs;
    lhs->assign_type = BLOCKING;
}

void nb_assign_wires(struct logic_t *lhs, struct logic_t *rhs) {
    ++lhs->from_num;
    lhs->from = realloc(lhs->from, lhs->from_num*sizeof(struct logic_t*));
    lhs->from[lhs->from_num-1] = rhs;
    ++rhs->to_num;
    rhs->to = realloc(rhs->to, rhs->to_num*sizeof(struct logic_t*));
    rhs->to[rhs->to_num-1] = lhs;
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
            if ((!lhs_changed) && event->lhs->value == event->lhs->to[i]->value) 
                continue;
            struct sim_event_t *new_event;
            if (event->lhs->to[i]->assign_type == BLOCKING) { 
                new_event = create_sim_event(event->lhs->to[i], event->lhs, CONTINUOUS_ASSIGNMENT);
            }
            else {
                new_event = create_sim_event(event->lhs->to[i], event->lhs, NBA_EVAL);
            }
            schedule_event(new_event, ACTIVE_REGION_INDEX, sim_time_slots->sim_time);
        }
    }
}

void execute_active_region() {
    struct sim_event_t *current_event;
    while (sim_time_slots->regions[0] != NULL) {
        current_event = sim_time_slots->regions[0];
        sim_time_slots->regions[0] = sim_time_slots->regions[0]->next_event;
        execute_event(current_event);
        free(current_event);
    }
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

void start_simulation() {
    printf("Starting simulation...\n");
    struct sim_time_slot_t *prev_time_slot;
    int unsigned delta_cycle_counter;
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
}
