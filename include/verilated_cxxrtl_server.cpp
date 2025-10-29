// -*- mode: C++; c-file-style: "cc-mode" -*-
//*************************************************************************
// DESCRIPTION: Verilator: CXXRTL debug server protocol implementation (POC)
//
// Code available from: https://verilator.org
//
//*************************************************************************
//
// Copyright 2003-2025 by Wilson Snyder. This program is free software; you can
// redistribute it and/or modify it under the terms of either the GNU
// Lesser General Public License Version 3 or the Perl Artistic License
// Version 2.0.
// SPDX-License-Identifier: LGPL-3.0-only OR Artistic-2.0
//
//*************************************************************************

#include "verilated_cxxrtl_server.h"

#ifndef _WIN32
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#else
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "Ws2_32.lib")
#endif

#include <cstring>
#include <iostream>
#include <sstream>

//=============================================================================
// JSON Implementation

std::string VlCxxrtlJson::dump() const {
    std::ostringstream oss;
    switch (m_type) {
    case TYPE_NULL: oss << "null"; break;
    case TYPE_BOOL: oss << (m_bool ? "true" : "false"); break;
    case TYPE_NUMBER: {
        // Check if it's an integer
        if (m_number == static_cast<int>(m_number)) {
            oss << static_cast<int>(m_number);
        } else {
            oss << m_number;
        }
        break;
    }
    case TYPE_STRING: {
        oss << '"';
        for (char c : m_string) {
            switch (c) {
            case '"': oss << "\\\""; break;
            case '\\': oss << "\\\\"; break;
            case '\n': oss << "\\n"; break;
            case '\r': oss << "\\r"; break;
            case '\t': oss << "\\t"; break;
            default: oss << c; break;
            }
        }
        oss << '"';
        break;
    }
    case TYPE_ARRAY: {
        oss << '[';
        for (size_t i = 0; i < m_array.size(); ++i) {
            if (i > 0) oss << ',';
            oss << m_array[i].dump();
        }
        oss << ']';
        break;
    }
    case TYPE_OBJECT: {
        oss << '{';
        bool first = true;
        for (const auto& pair : m_object) {
            if (!first) oss << ',';
            first = false;
            oss << '"' << pair.first << "\":";
            oss << pair.second.dump();
        }
        oss << '}';
        break;
    }
    }
    return oss.str();
}

// Minimal JSON parser (for POC - production should use proper library)
VlCxxrtlJson VlCxxrtlJson::parse(const std::string& str, std::string& error) {
    // This is a minimal parser for POC - just enough to parse protocol messages
    // Production code should use json11 or similar
    error = "";

    // Find the opening brace
    size_t pos = str.find('{');
    if (pos == std::string::npos) {
        error = "No JSON object found";
        return VlCxxrtlJson();
    }

    // Very simple object parser - assumes well-formed input
    std::map<std::string, VlCxxrtlJson> obj;
    pos++; // Skip '{'

    while (pos < str.length() && str[pos] != '}') {
        // Skip whitespace
        while (pos < str.length() && std::isspace(str[pos])) pos++;
        if (pos >= str.length() || str[pos] == '}') break;

        // Parse key
        if (str[pos] != '"') {
            error = "Expected string key";
            return VlCxxrtlJson();
        }
        pos++; // Skip opening quote
        size_t keyStart = pos;
        while (pos < str.length() && str[pos] != '"') pos++;
        std::string key = str.substr(keyStart, pos - keyStart);
        pos++; // Skip closing quote

        // Skip whitespace and colon
        while (pos < str.length() && (std::isspace(str[pos]) || str[pos] == ':')) pos++;

        // Parse value
        if (str[pos] == '"') {
            // String value
            pos++;
            size_t valStart = pos;
            while (pos < str.length() && str[pos] != '"') pos++;
            obj[key] = VlCxxrtlJson(str.substr(valStart, pos - valStart));
            pos++;
        } else if (std::isdigit(str[pos]) || str[pos] == '-') {
            // Number value
            size_t valStart = pos;
            while (pos < str.length() && (std::isdigit(str[pos]) || str[pos] == '.' || str[pos] == '-')) pos++;
            obj[key] = VlCxxrtlJson(std::stoi(str.substr(valStart, pos - valStart)));
        }

        // Skip comma
        while (pos < str.length() && (std::isspace(str[pos]) || str[pos] == ',')) pos++;
    }

    return VlCxxrtlJson(obj);
}

//=============================================================================
// CXXRTL Server Implementation

VlCxxrtlServer::~VlCxxrtlServer() { stop(); }

void VlCxxrtlServer::start(int port, VerilatedModel* modelp, VerilatedSyms* symsp) {
    if (m_running) return;

    m_modelp = modelp;
    m_symsp = symsp;

    // Build scope map from symbol table
    buildScopeMap();

#ifdef _WIN32
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        std::cerr << "WSAStartup failed\n";
        return;
    }
#endif

    // Create socket
    m_listenFd = socket(AF_INET, SOCK_STREAM, 0);
    if (m_listenFd < 0) {
        std::cerr << "Failed to create socket\n";
        return;
    }

    // Set socket options
    int opt = 1;
    setsockopt(m_listenFd, SOL_SOCKET, SO_REUSEADDR, (const char*)&opt, sizeof(opt));

    // Bind
    struct sockaddr_in addr;
    std::memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(port);

    if (bind(m_listenFd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        std::cerr << "Failed to bind to port " << port << "\n";
#ifdef _WIN32
        closesocket(m_listenFd);
#else
        close(m_listenFd);
#endif
        return;
    }

    // Listen
    if (listen(m_listenFd, 1) < 0) {
        std::cerr << "Failed to listen\n";
#ifdef _WIN32
        closesocket(m_listenFd);
#else
        close(m_listenFd);
#endif
        return;
    }

    std::cout << "CXXRTL debug server listening on port " << port << "\n";

    m_running = true;
    m_serverThread = std::thread([this]() { serverLoop(); });
}

void VlCxxrtlServer::stop() {
    if (!m_running) return;
    m_running = false;

    if (m_clientFd >= 0) {
#ifdef _WIN32
        closesocket(m_clientFd);
#else
        close(m_clientFd);
#endif
        m_clientFd = -1;
    }

    if (m_listenFd >= 0) {
#ifdef _WIN32
        closesocket(m_listenFd);
        WSACleanup();
#else
        close(m_listenFd);
#endif
        m_listenFd = -1;
    }

    if (m_serverThread.joinable()) {
        m_serverThread.join();
    }
}

void VlCxxrtlServer::serverLoop() {
    while (m_running) {
        struct sockaddr_in clientAddr;
        socklen_t clientLen = sizeof(clientAddr);

        int clientFd = accept(m_listenFd, (struct sockaddr*)&clientAddr, &clientLen);
        if (clientFd < 0) {
            if (m_running) {
                std::cerr << "Accept failed\n";
            }
            break;
        }

        std::cout << "Client connected\n";
        m_clientFd = clientFd;
        handleClient(clientFd);

#ifdef _WIN32
        closesocket(clientFd);
#else
        close(clientFd);
#endif
        m_clientFd = -1;
        std::cout << "Client disconnected\n";
    }
}

void VlCxxrtlServer::handleClient(int clientFd) {
    while (m_running) {
        std::string msg = receiveMessage(clientFd);
        if (msg.empty()) break;

        std::cout << "Received: " << msg << "\n";

        std::string error;
        VlCxxrtlJson cmd = VlCxxrtlJson::parse(msg, error);
        if (!error.empty()) {
            std::cerr << "JSON parse error: " << error << "\n";
            break;
        }

        VlCxxrtlJson response = handleCommand(cmd);
        std::string responseStr = response.dump() + '\0';
        sendMessage(clientFd, responseStr);
    }
}

std::string VlCxxrtlServer::receiveMessage(int fd) {
    std::string msg;
    char buf[4096];

    while (true) {
        ssize_t n = recv(fd, buf, sizeof(buf) - 1, 0);
        if (n <= 0) return "";

        buf[n] = '\0';
        for (ssize_t i = 0; i < n; ++i) {
            if (buf[i] == '\0') {
                return msg;
            }
            msg += buf[i];
        }
    }
}

void VlCxxrtlServer::sendMessage(int fd, const std::string& msg) {
    send(fd, msg.c_str(), msg.length(), 0);
}

VlCxxrtlJson VlCxxrtlServer::handleCommand(const VlCxxrtlJson& cmd) {
    std::string type = cmd["type"].stringValue();

    if (type == "greeting") {
        return cmdGreeting(cmd);
    } else if (type == "command") {
        std::string cmdName = cmd["command"].stringValue();
        if (cmdName == "list_scopes") {
            return cmdListScopes(cmd);
        } else if (cmdName == "list_items") {
            return cmdListItems(cmd);
        } else if (cmdName == "reference_items") {
            return cmdReferenceItems(cmd);
        } else if (cmdName == "get_simulation_status") {
            return cmdGetSimulationStatus(cmd);
        }
    }

    // Error response
    std::map<std::string, VlCxxrtlJson> resp;
    resp["type"] = VlCxxrtlJson("error");
    resp["error"] = VlCxxrtlJson("Unknown command");
    return VlCxxrtlJson(resp);
}

VlCxxrtlJson VlCxxrtlServer::cmdGreeting(const VlCxxrtlJson& cmd) {
    std::map<std::string, VlCxxrtlJson> resp;
    resp["type"] = VlCxxrtlJson("greeting");
    resp["version"] = VlCxxrtlJson(0);

    std::vector<VlCxxrtlJson> commands;
    commands.push_back(VlCxxrtlJson("list_scopes"));
    commands.push_back(VlCxxrtlJson("list_items"));
    commands.push_back(VlCxxrtlJson("reference_items"));
    commands.push_back(VlCxxrtlJson("get_simulation_status"));
    resp["commands"] = VlCxxrtlJson(commands);

    std::vector<VlCxxrtlJson> events;
    resp["events"] = VlCxxrtlJson(events);

    std::map<std::string, VlCxxrtlJson> features;
    std::vector<VlCxxrtlJson> encodings;
    encodings.push_back(VlCxxrtlJson("base64(u32)"));
    features["item_values_encoding"] = VlCxxrtlJson(encodings);
    resp["features"] = VlCxxrtlJson(features);

    return VlCxxrtlJson(resp);
}

VlCxxrtlJson VlCxxrtlServer::cmdListScopes(const VlCxxrtlJson& cmd) {
    std::map<std::string, VlCxxrtlJson> resp;
    resp["type"] = VlCxxrtlJson("response");

    std::vector<VlCxxrtlJson> scopes;
    for (const auto& pair : m_scopeItems) {
        scopes.push_back(VlCxxrtlJson(pair.first));
    }
    resp["scopes"] = VlCxxrtlJson(scopes);

    return VlCxxrtlJson(resp);
}

VlCxxrtlJson VlCxxrtlServer::cmdListItems(const VlCxxrtlJson& cmd) {
    std::string scope = cmd["scope"].stringValue();

    std::map<std::string, VlCxxrtlJson> resp;
    resp["type"] = VlCxxrtlJson("response");

    auto it = m_scopeItems.find(scope);
    if (it == m_scopeItems.end()) {
        resp["items"] = VlCxxrtlJson(std::vector<VlCxxrtlJson>());
        return VlCxxrtlJson(resp);
    }

    std::vector<VlCxxrtlJson> items;
    for (const auto& item : it->second) {
        std::map<std::string, VlCxxrtlJson> itemObj;
        itemObj["name"] = VlCxxrtlJson(item.name);
        itemObj["type"] = VlCxxrtlJson(item.type);
        itemObj["width"] = VlCxxrtlJson(item.width);
        items.push_back(VlCxxrtlJson(itemObj));
    }
    resp["items"] = VlCxxrtlJson(items);

    return VlCxxrtlJson(resp);
}

VlCxxrtlJson VlCxxrtlServer::cmdReferenceItems(const VlCxxrtlJson& cmd) {
    // TODO: Implement item referencing
    std::map<std::string, VlCxxrtlJson> resp;
    resp["type"] = VlCxxrtlJson("response");
    resp["references"] = VlCxxrtlJson(std::vector<VlCxxrtlJson>());
    return VlCxxrtlJson(resp);
}

VlCxxrtlJson VlCxxrtlServer::cmdGetSimulationStatus(const VlCxxrtlJson& cmd) {
    std::map<std::string, VlCxxrtlJson> resp;
    resp["type"] = VlCxxrtlJson("response");
    resp["time"] = VlCxxrtlJson(static_cast<int>(time()));
    resp["running"] = VlCxxrtlJson(false);
    return VlCxxrtlJson(resp);
}

void VlCxxrtlServer::buildScopeMap() {
    // POC: Just create a dummy scope with a couple of items
    // In real implementation, this would walk the symbol table
    std::vector<ItemInfo> topItems;

    ItemInfo clkItem;
    clkItem.name = "clk";
    clkItem.type = "value";
    clkItem.width = 1;
    clkItem.depth = 1;
    clkItem.flags = 0x01;  // INPUT
    clkItem.curr = nullptr;
    clkItem.next = nullptr;
    topItems.push_back(clkItem);

    ItemInfo rstItem;
    rstItem.name = "rst";
    rstItem.type = "value";
    rstItem.width = 1;
    rstItem.depth = 1;
    rstItem.flags = 0x01;  // INPUT
    rstItem.curr = nullptr;
    rstItem.next = nullptr;
    topItems.push_back(rstItem);

    m_scopeItems["top"] = topItems;
}

std::vector<std::string> VlCxxrtlServer::getScopeHierarchy() {
    std::vector<std::string> scopes;
    for (const auto& pair : m_scopeItems) {
        scopes.push_back(pair.first);
    }
    return scopes;
}

uint64_t VlCxxrtlServer::time() const {
    if (!m_modelp) return 0;
    return m_modelp->contextp()->time();
}
