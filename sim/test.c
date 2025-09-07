#include "../src/simulator.c"
int main() {
    initialize_sim();
    // scheduling of simulation events
    struct logic_t *a, *b, *c, *aclone;
    a = declare_wire(X);
    b = declare_wire(X);
    c = declare_wire(X);
    aclone = declare_wire(X);
    assign_const_to_wire(a, HIGH, 1);
    assign_const_to_wire(b, LOW, 1);
    assign_const_to_wire(c, X, 2);
    cont_assign_wires(aclone, a);
    start_simulation();
}
