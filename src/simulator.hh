#include "defines.hh"
#include <forward_list>
// TODO: make stuff virtual ??

class Simulator {
private:
    // TODO: change to tree
    vector<net_t> nets;
    forward_list<time_slot_t> time_slots;
    vector<event_t> initial_events;
    // TODO: add max sim time

    void move_region_to_active(region_e index);
    auto first_non_empty_region();
    void execute_event(const event_t &event);
    void execute_active_region();
public:
    // TODO: pass top module as argument to automatically initialize wires
    Simulator();
    void schedule_event(const event_t &ev, region_e region_index, sim_time_t time);
    net_id_t declare_net(logic_e init_val = X);
    void assign_const_to_net(
        net_id_t lhs_id,
        logic_e rhs_val,
        sim_time_t time,
        assign_type_e type = NON_BLOCKING
    );
    void blocking_assign_const_to_net(
        net_id_t lhs_id,
        logic_e rhs_val,
        sim_time_t time
    );
    void cont_assign_nets(net_id_t lhs_id, net_id_t rhs_id);
    void nb_assign_nets(net_id_t lhs_id, net_id_t rhs_id);
    void sim_loop();
};
