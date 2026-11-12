/*
 * File: dispatcher.cpp
 * Purpose: Implements the central dispatch decision logic and DSA workflows.
 * DSA Concepts:
 *   - Priority Queue extraction: Serves highest urgency patients first.
 *   - Dijkstra's Algorithm: Finds nearest ambulance and shortest route to hospital.
 *   - Merge Sort: Orders candidate hospitals by travel time and bed availability.
 *   - Stack Push/Pop: Records history and handles rollback (undo).
 *   - DFS: Traverses graph components when edges are blocked.
 * Time Complexity:
 *   - Patient registration: O(log n) for heap, O(1) for queue.
 *   - Dispatch resolution: O((V + E) log V) for Dijkstra + O(H log H) for Merge Sort.
 *   - History Undo: O(1) stack pop and hash map updates.
 */

#include "dispatcher.h"
#include <iostream>

// Helper to get current monotonic time in milliseconds.
static long long getCurrentMillis() {
    auto now = std::chrono::steady_clock::now();
    return std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();
}

// Loads hospitals, drivers, ambulances, and roads from the data directory.
bool DispatchManager::initialize(const std::string& data_dir) {
    if (!loadHospitals(data_dir + "/hospitals.csv", hospitals, hospital_map)) return false;
    if (!loadDrivers(data_dir + "/drivers.csv", drivers, driver_map)) return false;
    if (!loadAmbulances(data_dir + "/ambulances.csv", ambulances, ambulance_map)) return false;
    if (!loadRoads(data_dir + "/roads.csv", graph)) return false;

    // Populate BST with initial hospital bed capacities
    bed_bst.clear();
    for (const auto& h : hospitals) {
        bed_bst.insert(h.id, h.free_beds);
    }
    return true;
}

// Frees ambulances whose simulated trip time has completed (1 sec = 1 min).
void DispatchManager::updateAmbulanceAvailability() {
    long long now = getCurrentMillis();
    std::vector<ActiveTrip> remaining;
    for (const auto& trip : active_trips) {
        if (now >= trip.return_time_ms) {
            if (ambulance_map.find(trip.ambulance_id) != ambulance_map.end()) {
                ambulance_map[trip.ambulance_id].status = "available";
                for (auto& a : ambulances) {
                    if (a.id == trip.ambulance_id) a.status = "available";
                }
            }
        } else {
            remaining.push_back(trip);
        }
    }
    active_trips = remaining;
}

// Returns the graph node where an ambulance is stationed.
std::string DispatchManager::getAmbulanceLocationNode(const Ambulance& amb) {
    if (hospital_map.find(amb.base_hospital_id) != hospital_map.end()) {
        return hospital_map[amb.base_hospital_id].name;
    }
    return "Connaught Place";
}

// Checks if a hospital offers the specialty matching the patient condition.
bool DispatchManager::matchesSpecialty(const Hospital& h, const std::string& emergency_type) {
    if (emergency_type == "general") return true;
    std::string needed = emergency_type;
    if (emergency_type == "stroke") needed = "neuro";
    if (emergency_type == "pregnancy") needed = "maternity";
    if (emergency_type == "breathing") needed = "cardiac";
    return h.specialties.find(needed) != std::string::npos;
}

// Enqueues patient into MinHeap if Critical/High, or FIFO Queue if Medium/Low.
Patient DispatchManager::registerPatient(std::string name, int age, std::string gender, std::string pickup, std::string type, std::string sev) {
    Patient p;
    p.id = ++patient_seq;
    p.name = name;
    p.age = age;
    p.gender = gender;
    p.pickup_loc = pickup;
    p.emergency_type = type;
    p.severity = sev;
    p.arrival_order = patient_seq;

    if (sev == "Critical") p.severity_rank = 1;
    else if (sev == "High") p.severity_rank = 2;
    else if (sev == "Medium") p.severity_rank = 3;
    else p.severity_rank = 4;

    if (p.severity_rank <= 2) {
        critical_heap.insert(p);
    } else {
        normal_queue.enqueue(p);
    }
    return p;
}

// Helper to choose the nearest available ambulance using Dijkstra times.
static int selectBestAmbulance(const std::vector<Ambulance>& ambulances, const std::unordered_map<int, Ambulance>& amb_map,
                              const std::vector<double>& dists, RoadGraph& graph, const std::string& pref_type,
                              const std::unordered_map<int, Hospital>& h_map) {
    int best_id = -1;
    double best_time = 1e9;
    bool found_pref = false;

    for (const auto& a : ambulances) {
        auto it = amb_map.find(a.id);
        if (it == amb_map.end() || it->second.status != "available") continue;

        std::string base_name = "Connaught Place";
        auto hit = h_map.find(a.base_hospital_id);
        if (hit != h_map.end()) base_name = hit->second.name;

        int node_id = graph.addOrGetNode(base_name);
        double time_val = dists[node_id];
        if (time_val >= 1e8) continue;

        bool is_pref = (a.type == pref_type);
        if (is_pref && (!found_pref || time_val < best_time)) {
            best_id = a.id;
            best_time = time_val;
            found_pref = true;
        } else if (!found_pref && time_val < best_time) {
            best_id = a.id;
            best_time = time_val;
        }
    }
    return best_id;
}

// Dispatches the next patient: runs Dijkstra, selects units, ranks hospitals, pushes stack.
DispatchRecord DispatchManager::processNextPatient(bool& success, std::string& status_msg, std::vector<CandidateHospital>& top_alternatives) {
    updateAmbulanceAvailability();
    success = false;
    top_alternatives.clear();
    DispatchRecord emptyRec{};

    Patient patient{};
    if (!critical_heap.isEmpty()) {
        patient = critical_heap.extractTop();
    } else if (!normal_queue.isEmpty()) {
        patient = normal_queue.dequeue();
    } else {
        status_msg = "No pending patients in intake queue";
        return emptyRec;
    }

    int pickup_id = graph.addOrGetNode(patient.pickup_loc);
    std::vector<double> dist_from_pickup;
    std::vector<int> parent_from_pickup;
    std::vector<double> km_from_pickup;
    graph.runDijkstra(pickup_id, dist_from_pickup, parent_from_pickup, km_from_pickup);

    std::string pref_type = "BLS";
    if (patient.severity_rank <= 2 || patient.emergency_type == "cardiac" || patient.emergency_type == "stroke") {
        pref_type = "ALS";
    } else if (patient.emergency_type == "pregnancy") {
        pref_type = "neonatal";
    }

    int amb_id = selectBestAmbulance(ambulances, ambulance_map, dist_from_pickup, graph, pref_type, hospital_map);
    if (amb_id == -1) {
        // Fallback: use BFS to find nearest available ambulance base by hops
        std::vector<int> candidate_bases;
        for (const auto& a : ambulances) {
            if (ambulance_map[a.id].status == "available") {
                candidate_bases.push_back(graph.addOrGetNode(getAmbulanceLocationNode(a)));
            }
        }
        int bfs_node = graph.findNearestByHops(pickup_id, candidate_bases);
        if (bfs_node != -1) {
            for (const auto& a : ambulances) {
                if (ambulance_map[a.id].status == "available" && graph.addOrGetNode(getAmbulanceLocationNode(a)) == bfs_node) {
                    amb_id = a.id;
                    break;
                }
            }
        }
    }

    if (amb_id == -1) {
        if (patient.severity_rank <= 2) critical_heap.insert(patient);
        else normal_queue.enqueue(patient);
        status_msg = "All ambulances are currently busy";
        return emptyRec;
    }

    // Identify candidate hospitals meeting emergency and specialty requirements
    std::vector<CandidateHospital> candidates;
    bool relaxed_specialty = false;

    for (const auto& h : hospitals) {
        if (!h.has_emergency) continue;
        if (patient.severity_rank == 1 && h.icu_free <= 0) continue;
        if (h.free_beds <= 0) continue;
        if (matchesSpecialty(h, patient.emergency_type)) {
            int h_node = graph.addOrGetNode(h.name);
            double t = dist_from_pickup[h_node];
            if (t < 1e8) {
                candidates.push_back({h.id, h.name, t, h.free_beds, h.icu_free, "Exact Specialty"});
            }
        }
    }

    if (candidates.empty()) {
        relaxed_specialty = true;
        for (const auto& h : hospitals) {
            if (!h.has_emergency || h.free_beds <= 0) continue;
            if (patient.severity_rank == 1 && h.icu_free <= 0) continue;
            int h_node = graph.addOrGetNode(h.name);
            double t = dist_from_pickup[h_node];
            if (t < 1e8) {
                candidates.push_back({h.id, h.name, t, h.free_beds, h.icu_free, "General Emergency (Relaxed)"});
            }
        }
    }

    if (candidates.empty()) {
        if (patient.severity_rank <= 2) critical_heap.insert(patient);
        else normal_queue.enqueue(patient);
        status_msg = "No hospital with open beds is currently reachable";
        return emptyRec;
    }

    // Rank candidate hospitals using Merge Sort
    sortCandidateHospitals(candidates);
    CandidateHospital chosen = candidates[0];

    for (size_t i = 1; i < candidates.size() && i <= 3; i++) {
        top_alternatives.push_back(candidates[i]);
    }

    Ambulance best_amb = ambulance_map[amb_id];
    int amb_node = graph.addOrGetNode(getAmbulanceLocationNode(best_amb));
    RouteResult route1 = graph.getRoute(amb_node, pickup_id);
    RouteResult route2 = graph.getRoute(pickup_id, graph.addOrGetNode(chosen.name));

    // Update system state
    ambulance_map[amb_id].status = "busy";
    for (auto& a : ambulances) {
        if (a.id == amb_id) a.status = "busy";
    }

    hospital_map[chosen.hospital_id].free_beds--;
    if (patient.severity_rank == 1) {
        hospital_map[chosen.hospital_id].icu_free--;
    }
    for (auto& h : hospitals) {
        if (h.id == chosen.hospital_id) {
            h.free_beds = hospital_map[chosen.hospital_id].free_beds;
            h.icu_free = hospital_map[chosen.hospital_id].icu_free;
        }
    }

    double total_eta = route1.total_time + route2.total_time;
    // Keep unit busy for a realistic demo duration (10 seconds per trip minute, min 60s)
    long long trip_duration_ms = (long long)(total_eta * 10000);
    if (trip_duration_ms < 60000) trip_duration_ms = 60000;
    active_trips.push_back({amb_id, chosen.hospital_id, getCurrentMillis() + trip_duration_ms});

    Driver d = driver_map[best_amb.driver_id];

    DispatchRecord record;
    record.dispatch_id = ++dispatch_seq;
    record.patient_id = patient.id;
    record.patient_name = patient.name;
    record.ambulance_id = best_amb.id;
    record.vehicle_no = best_amb.vehicle_no;
    record.driver_id = d.id;
    record.driver_name = d.name;
    record.driver_phone = d.phone;
    record.hospital_id = chosen.hospital_id;
    record.hospital_name = chosen.name;
    record.route1_time = route1.total_time;
    record.route1_km = route1.total_dist;
    record.route2_time = route2.total_time;
    record.route2_km = route2.total_dist;
    record.total_eta = total_eta;
    record.route1_path = route1.path_names;
    record.route2_path = route2.path_names;
    record.is_critical = (patient.severity_rank == 1);

    record.rationale = "Selected " + best_amb.type + " ambulance " + best_amb.vehicle_no +
                       " (ETA " + std::to_string((int)route1.total_time) + " min); assigned to " +
                       chosen.name + " (" + chosen.specialty_match + ", " +
                       std::to_string(hospital_map[chosen.hospital_id].free_beds) + " beds left).";
    if (relaxed_specialty) {
        record.rationale += " Note: specialty relaxed to general emergency due to bed availability.";
    }

    dispatch_history.push(record);
    success = true;
    status_msg = "Dispatched successfully";
    return record;
}

// Undoes the last dispatch by restoring ambulance status and hospital beds.
bool DispatchManager::undoLastDispatch(std::string& feedback) {
    if (dispatch_history.isEmpty()) {
        feedback = "Dispatch history stack is empty";
        return false;
    }
    DispatchRecord rec = dispatch_history.pop();

    if (ambulance_map.find(rec.ambulance_id) != ambulance_map.end()) {
        ambulance_map[rec.ambulance_id].status = "available";
        for (auto& a : ambulances) {
            if (a.id == rec.ambulance_id) a.status = "available";
        }
    }

    if (hospital_map.find(rec.hospital_id) != hospital_map.end()) {
        hospital_map[rec.hospital_id].free_beds++;
        if (rec.is_critical) {
            hospital_map[rec.hospital_id].icu_free++;
        }
        for (auto& h : hospitals) {
            if (h.id == rec.hospital_id) {
                h.free_beds = hospital_map[rec.hospital_id].free_beds;
                h.icu_free = hospital_map[rec.hospital_id].icu_free;
            }
        }
    }

    std::vector<ActiveTrip> rem;
    for (const auto& t : active_trips) {
        if (t.ambulance_id != rec.ambulance_id) rem.push_back(t);
    }
    active_trips = rem;

    feedback = "Rolled back dispatch #" + std::to_string(rec.dispatch_id) + " for " + rec.patient_name;
    return true;
}

// Toggles road block status and uses DFS to report total connected components.
bool DispatchManager::toggleRoadBlock(const std::string& from, const std::string& to, bool& is_blocked, int& component_count) {
    int u = graph.addOrGetNode(from);
    int v = graph.addOrGetNode(to);
    bool current_state = false;
    for (const auto& edge : graph.adj[u]) {
        if (edge.to_node == v) {
            current_state = edge.blocked;
            break;
        }
    }
    bool new_state = !current_state;
    graph.setRoadBlocked(from, to, new_state);
    is_blocked = new_state;
    component_count = graph.checkConnectedComponents();
    return true;
}
