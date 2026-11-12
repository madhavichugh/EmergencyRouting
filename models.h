#ifndef MODELS_H
#define MODELS_H

/*
 * File: models.h
 * Purpose: Defines core data models for the emergency dispatch system.
 * DSA Concept: Plain Old Data structures (Records/Structs) and state modeling.
 * Time Complexity: All field reads and writes are O(1).
 */

#include <string>
#include <vector>

// Represents a hospital node in the emergency network.
struct Hospital {
    int id;
    std::string name;
    std::string area;
    double lat;
    double lng;
    int total_beds;
    int free_beds;
    int icu_total;
    int icu_free;
    std::string specialties;
    int has_emergency;
};

// Represents an emergency vehicle in the fleet.
struct Ambulance {
    int id;
    std::string vehicle_no;
    std::string type;
    int base_hospital_id;
    double lat;
    double lng;
    std::string status;
    int driver_id;
};

// Represents an ambulance driver.
struct Driver {
    int id;
    std::string name;
    std::string phone;
    int exp_years;
    std::string shift;
};

// Represents an emergency patient intake record.
struct Patient {
    int id;
    std::string name;
    int age;
    std::string gender;
    std::string pickup_loc;
    std::string emergency_type;
    std::string severity;
    int severity_rank;
    int arrival_order;
};

// Represents a completed dispatch action, stored in the history stack.
struct DispatchRecord {
    int dispatch_id;
    int patient_id;
    std::string patient_name;
    int ambulance_id;
    std::string vehicle_no;
    int driver_id;
    std::string driver_name;
    std::string driver_phone;
    int hospital_id;
    std::string hospital_name;
    double route1_time;
    double route1_km;
    double route2_time;
    double route2_km;
    double total_eta;
    std::vector<std::string> route1_path;
    std::vector<std::string> route2_path;
    std::string rationale;
    bool is_critical;
};

// Represents a directed edge in the road network graph.
struct RoadEdge {
    int to_node;
    double dist_km;
    double travel_min;
    double traffic_factor;
    bool blocked;
};

// Stores the outcome of a shortest-path search.
struct RouteResult {
    bool found;
    double total_time;
    double total_dist;
    std::vector<int> path_nodes;
    std::vector<std::string> path_names;
};

// Used during hospital triage to rank options with Merge Sort.
struct CandidateHospital {
    int hospital_id;
    std::string name;
    double travel_time;
    int free_beds;
    int icu_free;
    std::string specialty_match;
};

#endif
