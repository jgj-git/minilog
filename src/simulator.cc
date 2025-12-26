#include "simulator.hh"
#include <algorithm>

Simulator::Simulator() : time_slots{time_slot_t{0}} {}

void Simulator::schedule_event(const event_t &ev, region_e region_index, sim_time_t time) {
    auto prev_it = time_slots.begin();
    for(auto it = time_slots.begin(); it != time_slots.end(); ++it) {
        if (it->time == time) {
            it->regions[region_index].push_back(ev);
            return;
        }
        else if (it->time > time) {
            time_slots.emplace_after(prev_it, time);
            (++prev_it)->regions[region_index].push_back(ev);
            return;
        }
        prev_it = it;
    };
    time_slots.emplace_after(prev_it, time);
    (++prev_it)->regions[region_index].push_back(ev);
}

void Simulator::assign_const_to_net(
    net_id_t lhs_id,
    logic_e rhs_val,
    sim_time_t time,
    assign_type_e type //= NON_BLOCKING
) {
    schedule_event(event_t{lhs_id, rhs_val, CONSTANT_ASSIGNMENT},
                   ACTIVE_REGION, time);
    nets[lhs_id].assign_type = type;
}

net_id_t Simulator::declare_net(logic_e init_val/* = X*/) {
    net_id_t new_net_id = nets.size();
    nets.push_back(net_t{init_val});
    // TODO: add new init event type for this?
    initial_events.emplace_back(new_net_id, init_val, CONSTANT_ASSIGNMENT);
    return new_net_id;
}

void Simulator::blocking_assign_const_to_net(
    net_id_t lhs_id,
    logic_e rhs_val,
    sim_time_t time
) {
    assign_const_to_net(lhs_id, rhs_val, time, BLOCKING);
}

void Simulator::cont_assign_nets(net_id_t lhs_id, net_id_t rhs_id) {
    nets[rhs_id].to.push_back(lhs_id);
    nets[lhs_id].assign_type = BLOCKING;
}

void Simulator::nb_assign_nets(net_id_t lhs_id, net_id_t rhs_id) {
    nets[rhs_id].to.push_back(lhs_id);
    nets[lhs_id].assign_type = NON_BLOCKING;
}

void Simulator::execute_event(const event_t &event) {
    logic_e old_lhs_val = nets[event.lhs_id].value;
    switch (event.type) {
        case CONSTANT_ASSIGNMENT: {
            nets[event.lhs_id].value = event.rhs_const_val;
            break;
        }
        case CONTINUOUS_ASSIGNMENT: {
            nets[event.lhs_id].value = nets[event.rhs_id].value;
            break;
        }
        case NBA_EVAL: {
            schedule_event(event_t{event.lhs_id, nets[event.rhs_id].value, NBA_UPDATE}, 
                           NBA_REGION, time_slots.front().time);
            return;
        }
        case NBA_UPDATE: {
            nets[event.lhs_id].value = event.rhs_const_val;
            break;
        }
        default: {
            cout << "Event execution error: invalid event type!\n";
            return;
        }
    }
    printf("\tEvent executed in timeslot %d: wire %d went from %d to %d\n", 
           time_slots.front().time, event.lhs_id, old_lhs_val, nets[event.lhs_id].value);
    for (auto to_id : nets[event.lhs_id].to) {
        if (nets[to_id].value == nets[event.lhs_id].value) continue;
        if (nets[to_id].assign_type == BLOCKING) { 
            schedule_event(event_t{to_id, event.lhs_id, CONTINUOUS_ASSIGNMENT},
                           ACTIVE_REGION, time_slots.front().time);
        }
        else {
            schedule_event(event_t{to_id, event.lhs_id, NBA_EVAL},
                           ACTIVE_REGION, time_slots.front().time);
        }
    }
}

void Simulator::execute_active_region() {
    unsigned i{0};
    // TODO: is this safe? can the compiler remove the size computation on each
    // iteration?
    while (i < time_slots.front().regions[ACTIVE_REGION].size()) {
        execute_event(time_slots.front().regions[ACTIVE_REGION][i]);
        ++i;
    }
    time_slots.front().regions[ACTIVE_REGION].clear();
}

void Simulator::move_region_to_active(region_e index) {
    time_slots.front().regions[ACTIVE_REGION] = 
        std::move(time_slots.front().regions[index]);
}

auto Simulator::first_non_empty_region() {
    return find_if_not(
        time_slots.front().regions.begin(),
        time_slots.front().regions.end(),
        [](const vector<event_t>& ev)->bool { return ev.empty(); }
    );
}

void Simulator::sim_loop() {
    for (auto ev : initial_events) execute_event(ev);
    initial_events.clear();
    while (!time_slots.empty()) {
        cout << "Starting timeslot " << time_slots.front().time << "\n";
        delta_cycle_t delta_cycle{0};
        auto it = first_non_empty_region();
        while (it != time_slots.front().regions.end()) {
            if (it != time_slots.front().regions.begin()) {
                move_region_to_active(
                    static_cast<region_e>(distance(time_slots.front().regions.begin(), it))
                );
                ++delta_cycle;
                cout << "Starting delta cycle " << delta_cycle << "\n";
            }
            execute_active_region();
            it = first_non_empty_region();
        }
        time_slots.pop_front();
    }
}

