// Minimal single-threaded HTTP server (Linux) to handle two endpoints:
// GET /npc/random and POST /npc with optional JSON body {"seed":...,"world":{...}}

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#include <cstring>
#include <filesystem>
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <nlohmann/json.hpp>
#include "GenerationContext.h"
#include "generator/NPCGenerator.h"

using json = nlohmann::json;
using namespace exob;

static json loadData(const std::string& path) {
    std::ifstream in(path);
    if (!in) return json{};
    json j; in >> j; return j;
}

static std::string resolveOccupationTable(const std::string& base) {
    namespace fs = std::filesystem;
    fs::path root("data/occupations");
    fs::path candidate = root / base;
    if (fs::exists(candidate)) return candidate.string();
    fs::path alt = root / (base + "s");
    if (fs::exists(alt)) return alt.string();
    fs::path altJson = root / (base + ".json");
    if (fs::exists(altJson)) return altJson.string();
    fs::path altJsonPlural = root / (base + "s.json");
    if (fs::exists(altJsonPlural)) return altJsonPlural.string();
    return {};
}

static void send_json_response(int client_fd, const std::string& body) {
    std::ostringstream oss;
    oss << "HTTP/1.1 200 OK\r\n";
    oss << "Content-Type: application/json\r\n";
    oss << "Content-Length: " << body.size() << "\r\n";
    oss << "Connection: close\r\n\r\n";
    oss << body;
    std::string resp = oss.str();
    send(client_fd, resp.c_str(), resp.size(), 0);
}

static void send_html_response(int client_fd, const std::string& body) {
    std::ostringstream oss;
    oss << "HTTP/1.1 200 OK\r\n";
    oss << "Content-Type: text/html; charset=utf-8\r\n";
    oss << "Content-Length: " << body.size() << "\r\n";
    oss << "Connection: close\r\n\r\n";
    oss << body;
    std::string resp = oss.str();
    send(client_fd, resp.c_str(), resp.size(), 0);
}

int main(int argc, char** argv) {
    GenerationContext ctx;
    ctx.dataRoot["names"]["male_first"] = loadData("data/names/male_first.json");
    ctx.dataRoot["names"]["female_first"] = loadData("data/names/female_first.json");
    ctx.dataRoot["names"]["surnames"] = loadData("data/names/surnames.json");
    ctx.dataRoot["clothing"]["details"] = loadData("data/clothing/details.json");
    ctx.dataRoot["clothing"]["men"] = loadData("data/clothing/men.json");
    ctx.dataRoot["clothing"]["women"] = loadData("data/clothing/women.json");
    ctx.dataRoot["personalities"] = loadData("data/personalities.json");
    ctx.dataRoot["secrets"] = loadData("data/secrets.json");
    ctx.dataRoot["races"] = loadData("data/races.json");
    ctx.dataRoot["wealth"] = loadData("data/wealth.json");
    ctx.dataRoot["age"] = loadData("data/age.json");
    ctx.dataRoot["occupations"] = loadData("data/occupations/occupations.json");
    if (ctx.dataRoot["occupations"].contains("categories")) {
        for (auto &category : ctx.dataRoot["occupations"]["categories"]) {
            std::string table = category.value("table", "");
            if (!table.empty()) {
                std::string path = resolveOccupationTable(table);
                if (!path.empty()) {
                    ctx.dataRoot["occupations"]["tables"][table] = loadData(path);
                }
            }
        }
    }

    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == -1) { perror("socket"); return 1; }
    int opt = 1; setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(8080);

    if (bind(server_fd, (sockaddr*)&addr, sizeof(addr)) < 0) { perror("bind"); return 1; }
    if (listen(server_fd, 10) < 0) { perror("listen"); return 1; }

    std::cout << "Server listening on http://0.0.0.0:8080" << std::endl;

    while (true) {
        int client = accept(server_fd, nullptr, nullptr);
        if (client < 0) continue;
        // read request
        char buf[8192];
        ssize_t r = recv(client, buf, sizeof(buf)-1, 0);
        if (r <= 0) { close(client); continue; }
        buf[r] = '\0';
        std::string req(buf);
        std::istringstream rs(req);
        std::string method, path, ver;
        rs >> method >> path >> ver;

        if (method == "GET" && path == "/") {
            std::ifstream htmlFile("frontend/index.html");
            std::ostringstream htmlStream;
            htmlStream << htmlFile.rdbuf();
            send_html_response(client, htmlStream.str());
        } else if (method == "GET" && path == "/npc/random") {
            int seed = ctx.world.seed ? ctx.world.seed : std::random_device{}();
            GenerationContext localCtx(seed);
            localCtx.dataRoot = ctx.dataRoot;
            NPCGenerator gen; NPC npc = gen.generate(localCtx);
            json personalityObj = {
                {"core_trait", npc.personality.coreTrait},
                {"social_behavior", npc.personality.socialBehavior},
                {"emotional_regulation", npc.personality.emotionalRegulation},
                {"cognitive_tendencies", npc.personality.cognitiveTendencies},
                {"behavioral_note", npc.personality.behavioralNote},
                {"stress_reaction", npc.personality.stressReaction}
            };
            json out = { 
                {"name", npc.name}, 
                {"occupation", npc.occupation}, 
                {"age", npc.age}, 
                {"gender", npc.gender},
                {"race", npc.race_full_str},
                {"sanity_points", npc.sanityPoints}, 
                {"clothing_style", npc.clothingStyle}, 
                {"personality", personalityObj}, 
                {"occultSensitivity", npc.occultSensitivity}, 
                {"secret", npc.secret}, 
                {"wealth", npc.wealth}, 
                {"log", localCtx.generationLog} };
            send_json_response(client, out.dump(2));
        } else if (method == "POST" && path == "/npc") {
            // find body (after \r\n\r\n)
            auto pos = req.find("\r\n\r\n");
            std::string body = (pos != std::string::npos) ? req.substr(pos+4) : std::string();
            try {
                auto j = json::parse(body.empty() ? "{}" : body);
                GenerationContext localCtx(j.value("seed", 0));
                if (j.contains("world")) {
                    auto w = j["world"];
                    localCtx.world.year = w.value("year", localCtx.world.year);
                    localCtx.world.coastal = w.value("coastal", localCtx.world.coastal);
                }
                localCtx.dataRoot = ctx.dataRoot;
                NPCGenerator gen; NPC npc = gen.generate(localCtx);
                json personalityObj = {
                    {"core_trait", npc.personality.coreTrait},
                    {"social_behavior", npc.personality.socialBehavior},
                    {"emotional_regulation", npc.personality.emotionalRegulation},
                    {"cognitive_tendencies", npc.personality.cognitiveTendencies},
                    {"behavioral_note", npc.personality.behavioralNote},
                    {"stress_reaction", npc.personality.stressReaction}
                };
                json out = { 
                    {"name", npc.name}, 
                    {"occupation", npc.occupation}, 
                    {"age", npc.age}, 
                    {"gender", npc.gender}, 
                    {"race", npc.race_full_str},
                    {"sanity_points", npc.sanityPoints}, 
                    {"clothing_style", npc.clothingStyle}, 
                    {"personality", personalityObj}, 
                    {"occultSensitivity", npc.occultSensitivity},
                    {"secret", npc.secret}, 
                    {"wealth", npc.wealth},
                    {"log", localCtx.generationLog} 
                };
                send_json_response(client, out.dump(2));
            } catch (...) {
                std::string err = "{\"error\":\"invalid json\"}";
                send_json_response(client, err);
            }
        } else {
            std::string notfound = "{\"error\":\"not found\"}";
            send_json_response(client, notfound);
        }
        close(client);
    }

    close(server_fd);
    return 0;
}
