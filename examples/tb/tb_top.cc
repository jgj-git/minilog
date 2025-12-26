#include "simulator.hh"

int main() {
    Simulator sim{};
    // scheduling of simulation events
    net_id_t a, b, c, aclone, creg, cregreg, cfinal;
    a = sim.declare_net(X);
    b = sim.declare_net(X);
    c = sim.declare_net(X);
    aclone = sim.declare_net(X);
    creg = sim.declare_net(LOW);
    cregreg = sim.declare_net(HIGH);
    cfinal = sim.declare_net(X);
    // initial values
    sim.assign_const_to_net(a, HIGH, 1);
    sim.assign_const_to_net(b, LOW, 1);
    sim.assign_const_to_net(c, X, 2);
    // continuous assignments
    sim.cont_assign_nets(aclone, a);
    sim.cont_assign_nets(cfinal, cregreg);
    // non-blocking assignments
    sim.nb_assign_nets(creg, c);
    sim.nb_assign_nets(cregreg, creg);
    // future values
    sim.assign_const_to_net(a, LOW, 8);
    sim.assign_const_to_net(c, HIGH, 10);
    sim.sim_loop();
}
