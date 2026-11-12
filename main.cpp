/*
 * File: main.cpp
 * Purpose: Entry point for the Emergency Response and Ambulance Routing System.
 * DSA Concepts:
 *   - Orchestrates MinHeap, Queue, Stack, Graph, Dijkstra, BFS, DFS, and Merge Sort.
 *   - Serves minimal web frontend via embedded HTTP server.
 *   - Runs 3 automated console demo scenarios showing live DSA traces.
 * Time Complexity:
 *   - System boot: O(N) where N is CSV rows.
 *   - Demo execution: O((V + E) log V) per routing decision.
 */

#include "dispatcher.h"
#include "httplib.h"
#include <iostream>
#include <fstream>
#include <sstream>

// Simple helper to extract a string value by key from a JSON payload.
static std::string extractJsonString(const std::string& json, const std::string& key) {
    std::string search = "\"" + key + "\"";
    size_t pos = json.find(search);
    if (pos == std::string::npos) return "";
    size_t colon = json.find(':', pos);
    if (colon == std::string::npos) return "";
    size_t quote1 = json.find('"', colon);
    if (quote1 == std::string::npos) return "";
    size_t quote2 = json.find('"', quote1 + 1);
    if (quote2 == std::string::npos) return "";
    return json.substr(quote1 + 1, quote2 - quote1 - 1);
}

// Simple helper to extract an integer value by key from a JSON payload.
static int extractJsonInt(const std::string& json, const std::string& key, int default_val = 0) {
    std::string search = "\"" + key + "\"";
    size_t pos = json.find(search);
    if (pos == std::string::npos) return default_val;
    size_t colon = json.find(':', pos);
    if (colon == std::string::npos) return default_val;
    size_t num_start = json.find_first_of("0123456789-", colon);
    if (num_start == std::string::npos) return default_val;
    size_t num_end = json.find_first_not_of("0123456789-", num_start);
    std::string num_str = json.substr(num_start, num_end - num_start);
    try {
        return std::stoi(num_str);
    } catch (...) {
        return default_val;
    }
}

// Reads the single-page HTML frontend into memory.
static std::string loadHtmlFile(const std::string& path) {
    std::ifstream in(path);
    if (!in.is_open()) {
        return "<html><body><h1>index.html not found</h1></body></html>";
    }
    std::stringstream ss;
    ss << in.rdbuf();
    return ss.str();
}

// Serializes system state (ambulances, hospitals, road status) into JSON.
static std::string buildStatusJson(DispatchManager& dm) {
    dm.updateAmbulanceAvailability();
    int avail_amb = 0;
    for (const auto& a : dm.ambulances) {
        if (dm.ambulance_map[a.id].status == "available") avail_amb++;
    }

    std::ostringstream ss;
    ss << "{";
    ss << "\"total_ambulances\":" << dm.ambulances.size() << ",";
    ss << "\"available_ambulances\":" << avail_amb << ",";
    ss << "\"busy_ambulances\":" << (dm.ambulances.size() - avail_amb) << ",";
    ss << "\"pending_critical\":" << dm.critical_heap.size() << ",";
    ss << "\"pending_normal\":" << dm.normal_queue.size() << ",";
    ss << "\"history_count\":" << dm.dispatch_history.size() << ",";
    ss << "\"components\":" << dm.graph.checkConnectedComponents() << ",";

    // Ambulances
    ss << "\"ambulances\":[";
    for (size_t i = 0; i < dm.ambulances.size(); i++) {
        const auto& a = dm.ambulance_map[dm.ambulances[i].id];
        if (i > 0) ss << ",";
        ss << "{\"id\":" << a.id
           << ",\"vehicle_no\":\"" << a.vehicle_no << "\""
           << ",\"type\":\"" << a.type << "\""
           << ",\"status\":\"" << a.status << "\""
           << ",\"lat\":" << a.lat
           << ",\"lng\":" << a.lng
           << ",\"base\":\"" << dm.getAmbulanceLocationNode(a) << "\"}";
    }
    ss << "],";

    // Waiting patients list
    ss << "\"waiting_patients\":[";
    bool first_wp = true;
    for (size_t i = 0; i < dm.critical_heap.data.size(); i++) {
        const auto& p = dm.critical_heap.data[i];
        if (!first_wp) ss << ",";
        ss << "{\"name\":\"" << p.name << "\",\"severity\":\"" << p.severity
           << "\",\"type\":\"" << p.emergency_type << "\",\"queue\":\"Critical (Min-Heap)\"}";
        first_wp = false;
    }
    for (int i = 0; i < dm.normal_queue.count; i++) {
        int idx = (dm.normal_queue.head + i) % 200;
        const auto& p = dm.normal_queue.items[idx];
        if (!first_wp) ss << ",";
        ss << "{\"name\":\"" << p.name << "\",\"severity\":\"" << p.severity
           << "\",\"type\":\"" << p.emergency_type << "\",\"queue\":\"Standard (FIFO Queue)\"}";
        first_wp = false;
    }
    ss << "],";

    // Hospitals
    ss << "\"hospitals\":[";
    for (size_t i = 0; i < dm.hospitals.size(); i++) {
        const auto& h = dm.hospital_map[dm.hospitals[i].id];
        if (i > 0) ss << ",";
        ss << "{\"id\":" << h.id
           << ",\"name\":\"" << h.name << "\""
           << ",\"area\":\"" << h.area << "\""
           << ",\"free_beds\":" << h.free_beds
           << ",\"icu_free\":" << h.icu_free
           << ",\"lat\":" << h.lat
           << ",\"lng\":" << h.lng
           << ",\"specialties\":\"" << h.specialties << "\"}";
    }
    ss << "],";

    // Busy units currently en route
    ss << "\"busy_units\":[";
    bool first_busy = true;
    for (const auto& trip : dm.active_trips) {
        if (dm.ambulance_map.find(trip.ambulance_id) != dm.ambulance_map.end()) {
            const auto& a = dm.ambulance_map[trip.ambulance_id];
            std::string hname = "Hospital";
            if (dm.hospital_map.find(trip.hospital_id) != dm.hospital_map.end()) {
                hname = dm.hospital_map[trip.hospital_id].name;
            }
            if (!first_busy) ss << ",";
            ss << "{\"vehicle_no\":\"" << a.vehicle_no << "\",\"type\":\"" << a.type
               << "\",\"destination\":\"" << hname << "\"}";
            first_busy = false;
        }
    }
    ss << "],";

    // Localities list
    ss << "\"localities\":[";
    bool first_loc = true;
    for (const auto& name : dm.graph.node_names) {
        if (dm.hospital_map.find(dm.graph.name_to_id[name]) == dm.hospital_map.end()) {
            if (!first_loc) ss << ",";
            ss << "\"" << name << "\"";
            first_loc = false;
        }
    }
    ss << "]}";
    return ss.str();
}

// Serializes the dispatch record and alternative choices into JSON response.
static std::string buildDispatchJson(const DispatchRecord& rec, bool success, const std::string& msg, const std::vector<CandidateHospital>& alts) {
    std::ostringstream ss;
    ss << "{";
    ss << "\"success\":" << (success ? "true" : "false") << ",";
    ss << "\"message\":\"" << msg << "\",";
    if (success) {
        ss << "\"dispatch_id\":" << rec.dispatch_id << ",";
        ss << "\"patient_name\":\"" << rec.patient_name << "\",";
        ss << "\"ambulance_id\":" << rec.ambulance_id << ",";
        ss << "\"vehicle_no\":\"" << rec.vehicle_no << "\",";
        ss << "\"driver_name\":\"" << rec.driver_name << "\",";
        ss << "\"driver_phone\":\"" << rec.driver_phone << "\",";
        ss << "\"hospital_name\":\"" << rec.hospital_name << "\",";
        ss << "\"route1_time\":" << rec.route1_time << ",";
        ss << "\"route1_km\":" << rec.route1_km << ",";
        ss << "\"route2_time\":" << rec.route2_time << ",";
        ss << "\"route2_km\":" << rec.route2_km << ",";
        ss << "\"total_eta\":" << rec.total_eta << ",";
        ss << "\"rationale\":\"" << rec.rationale << "\",";

        // Route 1 path
        ss << "\"route1_path\":[";
        for (size_t i = 0; i < rec.route1_path.size(); i++) {
            if (i > 0) ss << ",";
            ss << "\"" << rec.route1_path[i] << "\"";
        }
        ss << "],";

        // Route 2 path
        ss << "\"route2_path\":[";
        for (size_t i = 0; i < rec.route2_path.size(); i++) {
            if (i > 0) ss << ",";
            ss << "\"" << rec.route2_path[i] << "\"";
        }
        ss << "],";

        // Alternatives ranked by Merge Sort
        ss << "\"alternatives\":[";
        for (size_t i = 0; i < alts.size(); i++) {
            if (i > 0) ss << ",";
            ss << "{\"name\":\"" << alts[i].name << "\""
               << ",\"travel_time\":" << alts[i].travel_time
               << ",\"free_beds\":" << alts[i].free_beds
               << ",\"specialty\":\"" << alts[i].specialty_match << "\"}";
        }
        ss << "]";
    }
    ss << "}";
    return ss.str();
}

// Runs the 3 required automated console demo scenarios.
static void runConsoleDemos(DispatchManager& dm) {
    std::cout << "\n=======================================================\n";
    std::cout << " EMERGENCY RESPONSE & AMBULANCE ROUTING SYSTEM (DSA LAB)\n";
    std::cout << "=======================================================\n\n";

    // Scenario 1: Two critical patients at once (Min-Heap Priority Queue)
    std::cout << "--- SCENARIO 1: Two Critical Patients at Once (Min-Heap Demo) ---\n";
    std::cout << "Step 1: Patient 1 arrives: Rahul Sharma (Cardiac, Critical, Connaught Place)\n";
    dm.registerPatient("Rahul Sharma", 58, "M", "Connaught Place", "cardiac", "Critical");

    std::cout << "Step 2: Patient 2 arrives: Sneha Kapoor (Trauma, High, Saket)\n";
    dm.registerPatient("Sneha Kapoor", 26, "F", "Saket", "trauma", "High");

    std::cout << "Step 3: Patient 3 arrives: Amit Jain (General, Medium, Dwarka)\n";
    dm.registerPatient("Amit Jain", 34, "M", "Dwarka", "general", "Medium");

    std::cout << "Current Min-Heap Size (Critical/High): " << dm.critical_heap.size() << "\n";
    std::cout << "Current FIFO Queue Size (Medium/Low): " << dm.normal_queue.size() << "\n";

    bool ok1 = false;
    std::string msg1;
    std::vector<CandidateHospital> alts1;
    DispatchRecord rec1 = dm.processNextPatient(ok1, msg1, alts1);
    if (ok1) {
        std::cout << "\nDispatched First from Min-Heap: " << rec1.patient_name << " (Severity: Critical)\n";
        std::cout << "  Vehicle: " << rec1.vehicle_no << " | Driver: " << rec1.driver_name << "\n";
        std::cout << "  Assigned Hospital: " << rec1.hospital_name << "\n";
        std::cout << "  Route 1 (Base -> Patient): " << rec1.route1_time << " min (" << rec1.route1_km << " km)\n";
        std::cout << "  Route 2 (Patient -> Hospital): " << rec1.route2_time << " min (" << rec1.route2_km << " km)\n";
        std::cout << "  Total ETA: " << rec1.total_eta << " min\n";
        std::cout << "  Rationale: " << rec1.rationale << "\n";
    }

    bool ok2 = false;
    std::string msg2;
    std::vector<CandidateHospital> alts2;
    DispatchRecord rec2 = dm.processNextPatient(ok2, msg2, alts2);
    if (ok2) {
        std::cout << "\nDispatched Second from Min-Heap: " << rec2.patient_name << " (Severity: High)\n";
        std::cout << "  Vehicle: " << rec2.vehicle_no << " | Assigned Hospital: " << rec2.hospital_name << "\n";
        std::cout << "  Total ETA: " << rec2.total_eta << " min\n";
    }

    // Scenario 2: Nearby Ambulances Busy (Nearest Available Search & BFS Fallback)
    std::cout << "\n--- SCENARIO 2: All Nearby Ambulances Busy (Dijkstra/BFS Demo) ---\n";
    std::cout << "Marking Sector 62 and local Noida ambulances as busy...\n";
    for (auto& a : dm.ambulances) {
        if (a.base_hospital_id == 24 || a.base_hospital_id == 27) { // Fortis Noida, Metro Noida
            dm.ambulance_map[a.id].status = "busy";
        }
    }
    std::cout << "Patient arrives: Priya Verma (Pregnancy, High, Noida Sec 62)\n";
    dm.registerPatient("Priya Verma", 31, "F", "Noida Sec 62", "pregnancy", "High");

    bool ok3 = false;
    std::string msg3;
    std::vector<CandidateHospital> alts3;
    DispatchRecord rec3 = dm.processNextPatient(ok3, msg3, alts3);
    if (ok3) {
        std::cout << "System routed next closest available ambulance across NCR:\n";
        std::cout << "  Dispatched: " << rec3.vehicle_no << " | Driver: " << rec3.driver_name << "\n";
        std::cout << "  Destination: " << rec3.hospital_name << "\n";
        std::cout << "  Total ETA: " << rec3.total_eta << " min\n";
        std::cout << "  Rationale: " << rec3.rationale << "\n";
    }

    // Scenario 3: Blocked Road Forcing a Reroute (DFS Connectivity + Dijkstra Rerouting)
    std::cout << "\n--- SCENARIO 3: Blocked Road Forcing Reroute (DFS + Dijkstra) ---\n";
    std::string road_u = "Lajpat Nagar";
    std::string road_v = "Noida Sec 18";
    std::cout << "Blocking primary expressway corridor: " << road_u << " <---> " << road_v << "\n";
    bool is_blocked = false;
    int comps = 0;
    dm.toggleRoadBlock(road_u, road_v, is_blocked, comps);
    std::cout << "DFS Connectivity Check: Road blocked = " << (is_blocked ? "YES" : "NO")
              << " | Total Connected Components: " << comps << "\n";

    std::cout << "Dispatching patient from Noida Sec 18 to Central Delhi...\n";
    dm.registerPatient("Vikram Malhotra", 62, "M", "Noida Sec 18", "cardiac", "Critical");
    bool ok4 = false;
    std::string msg4;
    std::vector<CandidateHospital> alts4;
    DispatchRecord rec4 = dm.processNextPatient(ok4, msg4, alts4);
    if (ok4) {
        std::cout << "Dijkstra computed alternate detour route avoiding blocked corridor:\n";
        std::cout << "  Assigned Hospital: " << rec4.hospital_name << "\n";
        std::cout << "  Rerouted Path: ";
        for (size_t i = 0; i < rec4.route2_path.size(); i++) {
            if (i > 0) std::cout << " -> ";
            std::cout << rec4.route2_path[i];
        }
        std::cout << "\n  Route 2 Time with Detour: " << rec4.route2_time << " min\n";
    }

    // Unblock the road to restore state
    dm.toggleRoadBlock(road_u, road_v, is_blocked, comps);
    std::cout << "\nRestored road status. DFS Components: " << comps << "\n";

    // Undo demonstration
    std::cout << "\nTesting Dispatch Undo (Stack LIFO):\n";
    std::string undo_msg;
    dm.undoLastDispatch(undo_msg);
    std::cout << "  " << undo_msg << "\n";
    std::cout << "=======================================================\n\n";
}

int main(int argc, char* argv[]) {
    DispatchManager dm;
    if (!dm.initialize("data")) {
        std::cerr << "Error: Failed to load dataset from data/ directory.\n";
        return 1;
    }

    bool only_demo = false;
    for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];
        if (arg == "--demo" || arg == "--console") {
            only_demo = true;
        }
    }

    if (only_demo) {
        runConsoleDemos(dm);
        return 0;
    }

    // Run demo in console first to demonstrate functionality
    runConsoleDemos(dm);

    // Re-initialize to a clean fresh state for the live web server
    dm = DispatchManager();
    dm.initialize("data");

    // Setup HTTP endpoints for minimal web interface
    httplib::Server svr;

    svr.Get("/", [](const httplib::Request&, httplib::Response& res) {
        res.set_content(loadHtmlFile("index.html"), "text/html");
    });

    svr.Get("/status", [&dm](const httplib::Request&, httplib::Response& res) {
        res.set_content(buildStatusJson(dm), "application/json");
    });

    svr.Get("/history", [&dm](const httplib::Request&, httplib::Response& res) {
        std::ostringstream ss;
        ss << "[";
        for (size_t i = 0; i < dm.dispatch_history.data.size(); i++) {
            const auto& r = dm.dispatch_history.data[i];
            if (i > 0) ss << ",";
            ss << "{\"id\":" << r.dispatch_id
               << ",\"patient\":\"" << r.patient_name << "\""
               << ",\"ambulance\":\"" << r.vehicle_no << "\""
               << ",\"driver\":\"" << r.driver_name << "\""
               << ",\"hospital\":\"" << r.hospital_name << "\""
               << ",\"eta\":" << r.total_eta << "}";
        }
        ss << "]";
        res.set_content(ss.str(), "application/json");
    });

    svr.Post("/patient", [&dm](const httplib::Request& req, httplib::Response& res) {
        std::string name = extractJsonString(req.body, "name");
        int age = extractJsonInt(req.body, "age", 30);
        std::string gender = extractJsonString(req.body, "gender");
        std::string loc = extractJsonString(req.body, "location");
        std::string type = extractJsonString(req.body, "emergency_type");
        std::string sev = extractJsonString(req.body, "severity");

        if (name.empty()) name = "Emergency Patient";
        if (loc.empty()) loc = "Connaught Place";
        if (type.empty()) type = "general";
        if (sev.empty()) sev = "High";

        dm.registerPatient(name, age, gender, loc, type, sev);
        bool success = false;
        std::string status_msg;
        std::vector<CandidateHospital> alts;
        DispatchRecord rec = dm.processNextPatient(success, status_msg, alts);

        res.set_content(buildDispatchJson(rec, success, status_msg, alts), "application/json");
    });

    svr.Post("/enqueue", [&dm](const httplib::Request& req, httplib::Response& res) {
        std::string name = extractJsonString(req.body, "name");
        int age = extractJsonInt(req.body, "age", 30);
        std::string gender = extractJsonString(req.body, "gender");
        std::string loc = extractJsonString(req.body, "location");
        std::string type = extractJsonString(req.body, "emergency_type");
        std::string sev = extractJsonString(req.body, "severity");

        if (name.empty()) name = "Emergency Patient";
        if (loc.empty()) loc = "Connaught Place";
        if (type.empty()) type = "general";
        if (sev.empty()) sev = "High";

        dm.registerPatient(name, age, gender, loc, type, sev);
        std::ostringstream ss;
        ss << "{\"success\":true,\"message\":\"Patient " << name << " added to waiting queue ("
           << sev << ").\"}";
        res.set_content(ss.str(), "application/json");
    });

    svr.Post("/dispatch-next", [&dm](const httplib::Request&, httplib::Response& res) {
        bool success = false;
        std::string status_msg;
        std::vector<CandidateHospital> alts;
        DispatchRecord rec = dm.processNextPatient(success, status_msg, alts);

        res.set_content(buildDispatchJson(rec, success, status_msg, alts), "application/json");
    });

    svr.Post("/undo", [&dm](const httplib::Request&, httplib::Response& res) {
        std::string msg;
        bool ok = dm.undoLastDispatch(msg);
        std::ostringstream ss;
        ss << "{\"success\":" << (ok ? "true" : "false") << ",\"message\":\"" << msg << "\"}";
        res.set_content(ss.str(), "application/json");
    });

    svr.Post("/block-road", [&dm](const httplib::Request& req, httplib::Response& res) {
        std::string from = extractJsonString(req.body, "from");
        std::string to = extractJsonString(req.body, "to");
        bool blocked = false;
        int comps = 1;
        dm.toggleRoadBlock(from, to, blocked, comps);
        std::ostringstream ss;
        ss << "{\"is_blocked\":" << (blocked ? "true" : "false")
           << ",\"components\":" << comps
           << ",\"message\":\"Road between " << from << " and " << to << (blocked ? " blocked." : " unblocked.") << "\"}";
        res.set_content(ss.str(), "application/json");
    });

    std::cout << "Starting web server on port 8080...\n";
    svr.listen("0.0.0.0", 8080);
    return 0;
}
