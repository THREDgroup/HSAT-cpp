#include "../../include/agent_team/agent.hpp"

//// Necessary definitions of static vectors for sharing between agents.
vector<long double> Agent::all_fx_current;
vector< vector<long double> > Agent::all_xx_current;

//// Inline Agent constructor for accessing static vectors
Agent::Agent(void){}

//// Normal agent constructor, take one int
Agent::Agent(int ID, Parameters x){
    // Save the list of parameters
    p = x;

    // Remember your damn name, agent
    id = ID;

    // The beginning is usually the best place to start
    iteration_number = 0;
    Ti = p.temp_init;
}

//// A function that selects a random starting point, and pushes it to other agents.
void Agent::new_start(void){

    // Select a random starting point and evaluate it
    x_current = random_vector(p.D, p.ub, p.lb);
    fx_current = p.obj(x_current);

    // Share hte information
    all_fx_current[id] = fx_current;
    all_xx_current[id] = x_current;
}

//// Generated a candidate solution using Cauchy distribution.
vector<long double> Agent::candidate_solution(void){
    // Make some variable for use in this function
    vector<long double> candidate; // stores the candidate solution
    vector<long double> urv;       // A uniform random vector
    vector<long double> w;         // Vector of weights across agents
    long double wmax;              // Maximum in weight vector
    int j;                    // Index for random draw

    // Random draw to get candidate starting position
    //   IF Interacting: Check out other solutions
    //   ELSE: Start from where you are
    if(p.interacting) {
        w = all_fx_current;
        wmax = vector_max(w);
        for (int i = 0; i < w.size(); i++) {
            w[i] = wmax - w[i];
        }
        j = weighted_choice(w);
        candidate = all_xx_current[j];
    } else{
        candidate = x_current;
    }

    // Draw a uniform random vector for passing to tan function
    urv = random_vector(p.D, M_PI/2.0, -M_PI/2.0);

    // Cycle through, updating dimensions as you go. Analyze each to see if its in the domain
    int NOT_IN_DOMAIN = 0;
    for(int i=0; i<p.D; i++){
        candidate[i] += 0.5*Ti*tan(urv[i]);
        if(candidate[i] < p.lb || candidate[i] > p.ub){
            NOT_IN_DOMAIN += 1;
        }
    }

    // Recursive.
    //   IF: any dimension was outside the bounds, repeat this function.
    //   ELSE: return candidate
    if(NOT_IN_DOMAIN==0){
        return candidate;
    } else {
        return candidate_solution();
    }
}

//// A function to perform an iteration of SA.
void Agent::iterate(int iter){
    // Save the god-given iteration as the current iteration
    iteration_number = iter;

    // Vector for saving new candidate solution
    vector<long double> x_cand(p.D, 0.0);

    // Objective function value of current solution, and probability of accepting new solution
    long double fx_cand, p_accept;

    // Generate a new solution
    x_cand = candidate_solution();
    fx_cand = p.obj(x_cand);

    if(p.adaptive && p.history_length < 0) {
        history.push_back(fx_cand);
    }

    // If it is better, accept it
    if(fx_cand < fx_current){
        // Save locally
        x_current = x_cand;
        fx_current = fx_cand;
    } else {
        // If not, accept with some probability
        p_accept = exp((fx_current - fx_cand)/Ti);
        if(uniform(0.0, 1.0) < p_accept){
            // Save locally
            x_current = x_cand;
            fx_current = fx_cand;
        }
    }

    if(p.adaptive && p.history_length > 0) {
        history.push_back(fx_current);
    }

    //Update the temperature
    update_temp();
}

//// Updates temperature using simple stretched Cauchy schedule.
void Agent::update_temp(void) {

    if(p.n_reps == 1)
        cout << Ti << ", ";

    // If history_length is greater than 0, use a sliding window for the update
    if(p.history_length > 0) {
        if (p.adaptive) {
            // If the quality history is too long, pop one out and calculate the update
            if (history.size() > p.history_length) {
                history.pop_front();
                Ti = update_triki();
            }
        } else {
            Ti = update_cauchy();
        }
    }

    // If history length is less than 0, use an absolute stepping scheme
    if(p.history_length < 0) {
        bool UPDATE = false;

        // See if its time to compute an update
        if(iteration_number % p.history_length == 0) {
            UPDATE = true;
        }

        // If adaptive and time to update, update triki and clear the cache
        if(p.adaptive && UPDATE){
            Ti = update_triki();
            history.clear();
        }

        // If not adaptive, but time to update, update cauchy.
        if(!p.adaptive && UPDATE){
            Ti = update_cauchy();
        }
    }
}


// This function updates Triki
long double Agent::update_triki(void){
    long double q_std = stdev(history);
    long double update_factor = p.delt * Ti / pow(q_std, 2);
    if (q_std > 0.0) {
        if (update_factor > 1.0) {
            // Update delt and update_factor
            p.delt /= 2.0;
            update_factor /= 2.0;
        }
        if(update_factor > 1.0){
            return Ti;
        } else {
            return Ti * (1 - update_factor);
        }

    } else {
        return Ti;
    }
}

long double Agent::update_cauchy(void){
     return p.temp_init / (1.0 + p.delt * iteration_number);
}