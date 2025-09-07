#include "../src/simulator.c"
int main() {
    initialize_sim();
    // scheduling of simulation events
    struct logic_t *a, *b, *c;
    a = declare_wire(X);
    b = declare_wire(X);
    c = declare_wire(X);
    assign_const_to_wire(a, HIGH, 1, sim_time_slots);
    assign_const_to_wire(b, LOW, 1, sim_time_slots);
    assign_const_to_wire(c, X, 2, sim_time_slots);
    start_simulation();
}
