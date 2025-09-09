#include <pthread.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

#define MAX_SIM_TIME 10
///////////////////////////////////////////////////////
///////////////// TYPE DEFINITIONS ////////////////////
///////////////////////////////////////////////////////
typedef enum { LOW = 0, HIGH = 1, X, Z } logic_e;
// wire type
struct logic_t {
    // TODO: from has to be array of prt, for combi logic
    struct logic_t *from;
    struct logic_t *to;
    logic_e value;
};
// event type
typedef enum { CONSTANT_ASSIGNMENT, CONTINUOUS_ASSIGNMENT } event_type_e;
struct sim_event_t {
    struct logic_t *lhs;
    struct logic_t *rhs;
    struct sim_event_t *next_event;
    event_type_e type;
};
struct sim_event_t *create_sim_event(
    struct logic_t *lhs,
    struct logic_t *rhs,
    event_type_e type
) {
    struct sim_event_t *new_event = malloc(sizeof(struct sim_event_t));
    new_event->lhs = lhs;
    new_event->rhs = rhs;
    new_event->type = type;
    new_event->next_event = NULL;
    if (type == CONTINUOUS_ASSIGNMENT) {
        if (lhs != NULL)
            lhs->from = rhs;
        if (rhs != NULL)
            rhs->to = lhs;
    }
    return new_event;
}
// time slot type
struct sim_time_slot_t {
    struct sim_time_slot_t *next_time_slot;
    struct sim_event_t *events;
    struct sim_event_t *active_region_events;
    struct sim_event_t *nba_region_events;
    int unsigned sim_time;
};
struct sim_time_slot_t *create_sim_time_slot(int unsigned new_sim_time) {
    struct sim_time_slot_t *new_time_slot = malloc(sizeof(struct sim_time_slot_t));
    new_time_slot->sim_time = new_sim_time;
    new_time_slot->events = NULL;
    new_time_slot->active_region_events = NULL;
    new_time_slot->nba_region_events = NULL;
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
    sim_time_slots = create_sim_time_slot(0);
    // wires
    nwires = 0;
}

void add_event_to_list(struct sim_event_t **event_list,
                       struct sim_event_t *event) {
    event->next_event = *event_list;
    *event_list = event;
}

void schedule_event(struct sim_event_t *event, int unsigned _sim_time) {
    if (sim_time_slots == NULL)
        return;
    struct sim_time_slot_t *iter = sim_time_slots;
    struct sim_time_slot_t *prev_iter = NULL;
    // find time slot
    while (iter != NULL) {
        if (iter->sim_time == _sim_time) {
            add_event_to_list(&(iter->active_region_events), event);
            return;
        } 
        else if (iter->sim_time > _sim_time) {
            prev_iter->next_time_slot = create_sim_time_slot(_sim_time);
            prev_iter->next_time_slot->next_time_slot = iter;
            prev_iter->next_time_slot->sim_time = _sim_time;
            add_event_to_list(&(prev_iter->next_time_slot->active_region_events), event);
            return;
        }
        prev_iter = iter;
        iter = iter->next_time_slot;
    }
    prev_iter->next_time_slot = create_sim_time_slot(_sim_time);
    add_event_to_list(&(prev_iter->next_time_slot->active_region_events), event);
}

struct logic_t *declare_wire(logic_e init_value) {
    ++nwires;
    wires = realloc(wires, nwires * sizeof(struct logic_t));
    wires[nwires - 1].value = init_value;
    wires[nwires - 1].from = NULL;
    wires[nwires - 1].to = NULL;
    return &(wires[nwires - 1]);
}

void assign_const_to_wire(struct logic_t *lhs, logic_e rhs_val,
                          int unsigned sim_time) {
    struct logic_t *rhs = declare_wire(rhs_val);
    struct sim_event_t *event = create_sim_event(lhs, rhs, CONSTANT_ASSIGNMENT);
    schedule_event(event, sim_time);
}

void cont_assign_wires(struct logic_t *lhs, struct logic_t *rhs) {
    struct sim_event_t *event = create_sim_event(lhs, rhs, CONTINUOUS_ASSIGNMENT);
    schedule_event(event, 1);
}

void nba_wires(struct logic_t *lhs, struct logic_t *rhs) {

}

void execute_event(struct sim_event_t *event) {
    printf("\tEvent executed in timeslot %d: from %d to %d\n", sim_time,
           event->lhs->value, event->rhs->value);
    switch (event->type) {
        case CONSTANT_ASSIGNMENT: {
            event->lhs->value = event->rhs->value;
            break;
        }
        case CONTINUOUS_ASSIGNMENT: {
            event->lhs->value = event->rhs->value;
            struct sim_event_t *new_event = 
                create_sim_event(event->lhs, event->rhs, CONTINUOUS_ASSIGNMENT);
            schedule_event(new_event, sim_time + 1);
            break;
        }
        default: {
            printf("Event execution error: invalid event type!\n");
            break;
        }
    }
}

// TODO: figure out how to make simulation end when continuous assignments
// are used -> changes in lhs of cont assignment schedule event on rhs using
// to pointer
void start_simulation() {
    printf("Starting simulation...\n");
    struct sim_time_slot_t *prev_time_slot;
    while (sim_time_slots != NULL && sim_time <= MAX_SIM_TIME) {
        printf("\nTimeslot %d\n", sim_time_slots->sim_time);
        debug_sim_time_slot(sim_time_slots);
        sim_time_slots->events = sim_time_slots->active_region_events;
        struct sim_event_t *current_event = sim_time_slots->events;
        struct sim_event_t *prev_event;
        while (current_event != NULL) {
            execute_event(current_event);
            prev_event = current_event;
            current_event = current_event->next_event;
            free(prev_event);
        }
        prev_time_slot = sim_time_slots;
        sim_time_slots = sim_time_slots->next_time_slot;
        free(prev_time_slot);
        ++sim_time;
    }
}
