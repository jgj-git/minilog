#include "../src/simulator.c"
int main() {
    initialize_sim();
    // scheduling of simulation events
    struct logic_t *a, *b, *c, *aclone, *creg, *cregreg, *cfinal;
    a = declare_wire(X);
    b = declare_wire(X);
    c = declare_wire(X);
    aclone = declare_wire(X);
    creg = declare_wire(0);
    cregreg = declare_wire(1);
    cfinal = declare_wire(X);
    // initial values
    assign_const_to_wire(a, HIGH, 1);
    assign_const_to_wire(b, LOW, 1);
    assign_const_to_wire(c, X, 2);
    // continuous assignments
    cont_assign_wires(aclone, a);
    cont_assign_wires(cfinal, cregreg);
    // non-blocking assignments
    nb_assign_wires(creg, c);
    nb_assign_wires(cregreg, creg);
    // future values
    assign_const_to_wire(a, LOW, 8);
    assign_const_to_wire(c, 1, 10);
    start_simulation();
}
