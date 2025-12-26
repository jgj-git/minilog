#include <iostream>
#include <cstdint>
#include <vector>
#include <array>

///////////////////////////////////////////////////////
//////////////////// NAMESPACES  //////////////////////
///////////////////////////////////////////////////////

using namespace std;

///////////////////////////////////////////////////////
///////////////////// DEFINES  ////////////////////////
///////////////////////////////////////////////////////

#define MAX_TIME 10
#define NUM_REGIONS 2
#define DYN_ARRAY_SECTION_SIZE 10

///////////////////////////////////////////////////////
////////////////////// TYPES  /////////////////////////
///////////////////////////////////////////////////////

typedef enum { LOW = 0, HIGH = 1, X, Z } logic_e;

typedef enum { BLOCKING = 0, NON_BLOCKING } assign_type_e;

// TODO: make iterable (enum class??)
typedef enum { ACTIVE_REGION = 0, NBA_REGION } region_e;

typedef enum {
    CONSTANT_ASSIGNMENT,
    CONTINUOUS_ASSIGNMENT,
    NBA_EVAL,
    NBA_UPDATE
} event_type_e;

typedef uint64_t net_id_t;

typedef uint64_t sim_time_t; 

typedef uint32_t delta_cycle_t;

// NOTE: this makes it so a net cannot be on the LHS of blocking and non 
// blocking assignments at the same type, which technically breaks SV spec
struct net_t {
    vector<net_id_t> to;
    int unsigned id;
    logic_e value;
    assign_type_e assign_type;
    net_t(logic_e v) : value{v} {};
};

struct event_t {
    net_id_t lhs_id;
    net_id_t rhs_id;
    event_type_e type;
    logic_e rhs_const_val;
    // TODO: use assert for lhs=rhs??
    event_t(net_id_t _lhs, net_id_t _rhs, event_type_e _type) {
        if (_type != CONTINUOUS_ASSIGNMENT && _type != NBA_EVAL) {
            cout << "[event_t constructor] ERROR: INVALID EVENT TYPE\n";
            exit(1);
        }
        type = _type;
        lhs_id = _lhs;
        rhs_id = _rhs;
    };
    event_t(net_id_t _lhs, logic_e _rhs_val, event_type_e _type) {
        type = _type;
        lhs_id = _lhs;
        rhs_const_val = _rhs_val;
    }
};

struct time_slot_t {
    sim_time_t time;
    // TODO: make region a type
    array<vector<event_t>, NUM_REGIONS> regions;
    time_slot_t(sim_time_t _time) : time(_time) {};
};

