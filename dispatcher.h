#ifndef DISPATCHER_H
#define DISPATCHER_H

/*
 * File: dispatcher.h
 * Purpose: Declares the central DispatchManager class that orchestrates DSA modules.
 * DSA Concepts:
 *   - Priority Queue (MinHeap) & Queue: Multi-tier triage.
 *   - Graph & Dijkstra: Network routing and shortest travel time calculation.
 *   - Merge Sort: Ranking eligible candidate hospitals.
 *   - Stack: Tracking history for undo operations.
 *   - Binary Search Tree: Bed capacity organization.
 * Time Complexity:
 *   - Intake: O(log n) for critical/high, O(1) for medium/low.
 *   - Dispatch: O((V + E) log V) for Dijkstra + O(H log H) for Merge Sort.
 *   - Undo: O(1) stack pop and hash map updates.
 */

#include "models.h"
#include "structures.h"
#include "graph.h"
#include "sorting.h"
#include "loader.h"
#include <string>
#include <vector>
#include <unordered_map>
#include <chrono>

// Tracks an ambulance currently en route and when it becomes free again.
struct ActiveTrip {
    int ambulance_id;
    int hospital_id;
    long long return_time_ms;
};

// Central controller managing data, triage queues, routing, and dispatches.
class DispatchManager {
public:
    std::vector<Hospital> hospitals;
    std::unordered_map<int, Hospital> hospital_map;

    std::vector<Ambulance> ambulances;
    std::unordered_map<int, Ambulance> ambulance_map;

    std::vector<Driver> drivers;
    std::unordered_map<int, Driver> driver_map;

    RoadGraph graph;

    PatientHeap critical_heap;
    PatientQueue normal_queue;
    DispatchStack dispatch_history;
    HospitalBST bed_bst;

    std::vector<ActiveTrip> active_trips;

    int patient_seq = 0;
    int dispatch_seq = 0;

    // Initializes system and loads datasets from the data folder.
    bool initialize(const std::string& data_dir);

    // Releases ambulances whose simulated trip time has finished.
    void updateAmbulanceAvailability();

    // Enqueues an incoming patient into either MinHeap or Queue based on severity.
    Patient registerPatient(std::string name, int age, std::string gender, std::string pickup, std::string emergency_type, std::string severity);

    // Processes the next available patient in priority order.
    DispatchRecord processNextPatient(bool& success, std::string& status_msg, std::vector<CandidateHospital>& top_alternatives);

    // Undoes the most recent dispatch operation from the stack.
    bool undoLastDispatch(std::string& feedback);

    // Blocks or unblocks a road and uses DFS to check network components.
    bool toggleRoadBlock(const std::string& from, const std::string& to, bool& is_blocked, int& component_count);

    // Helper to find the node name in the graph for an ambulance base.
    std::string getAmbulanceLocationNode(const Ambulance& amb);

    // Helper to check if a hospital provides the required specialty.
    bool matchesSpecialty(const Hospital& h, const std::string& emergency_type);
};

#endif
