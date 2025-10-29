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
///
/// \file
/// \brief CXXRTL debug server protocol for remote simulation control
///
/// This is a proof-of-concept implementation of the CXXRTL debug server
/// protocol for Verilator models. It allows remote clients to query
/// the design hierarchy, access signal values, and control simulation.
///
/// NOTE: This is an early POC and the API will change.
///
//*************************************************************************

#ifndef VERILATOR_VERILATED_CXXRTL_SERVER_H_
#define VERILATOR_VERILATED_CXXRTL_SERVER_H_

#include "verilated.h"
#include "verilated_syms.h"

#include <functional>
#include <map>
#include <memory>
#include <string>
#include <thread>
#include <vector>

//=============================================================================
// Minimal JSON utilities for protocol implementation

class VlCxxrtlJson final {
public:
    enum Type { TYPE_NULL, TYPE_BOOL, TYPE_NUMBER, TYPE_STRING, TYPE_ARRAY, TYPE_OBJECT };

private:
    Type m_type = TYPE_NULL;
    bool m_bool = false;
    double m_number = 0.0;
    std::string m_string;
    std::vector<VlCxxrtlJson> m_array;
    std::map<std::string, VlCxxrtlJson> m_object;

public:
    // Constructors
    VlCxxrtlJson() = default;
    explicit VlCxxrtlJson(std::nullptr_t) : m_type{TYPE_NULL} {}
    explicit VlCxxrtlJson(bool value) : m_type{TYPE_BOOL}, m_bool{value} {}
    explicit VlCxxrtlJson(int value) : m_type{TYPE_NUMBER}, m_number{static_cast<double>(value)} {}
    explicit VlCxxrtlJson(double value) : m_type{TYPE_NUMBER}, m_number{value} {}
    explicit VlCxxrtlJson(const char* value) : m_type{TYPE_STRING}, m_string{value} {}
    explicit VlCxxrtlJson(const std::string& value) : m_type{TYPE_STRING}, m_string{value} {}
    explicit VlCxxrtlJson(const std::vector<VlCxxrtlJson>& value)
        : m_type{TYPE_ARRAY}, m_array{value} {}
    explicit VlCxxrtlJson(const std::map<std::string, VlCxxrtlJson>& value)
        : m_type{TYPE_OBJECT}, m_object{value} {}

    // Type checks
    bool isNull() const { return m_type == TYPE_NULL; }
    bool isBool() const { return m_type == TYPE_BOOL; }
    bool isNumber() const { return m_type == TYPE_NUMBER; }
    bool isString() const { return m_type == TYPE_STRING; }
    bool isArray() const { return m_type == TYPE_ARRAY; }
    bool isObject() const { return m_type == TYPE_OBJECT; }

    // Value accessors
    bool boolValue() const { return m_bool; }
    double numberValue() const { return m_number; }
    int intValue() const { return static_cast<int>(m_number); }
    const std::string& stringValue() const { return m_string; }
    const std::vector<VlCxxrtlJson>& arrayValue() const { return m_array; }
    const std::map<std::string, VlCxxrtlJson>& objectValue() const { return m_object; }

    // Object access
    const VlCxxrtlJson& operator[](const std::string& key) const {
        static const VlCxxrtlJson null_value;
        if (!isObject()) return null_value;
        auto it = m_object.find(key);
        return (it != m_object.end()) ? it->second : null_value;
    }

    // Serialization
    std::string dump() const;
    static VlCxxrtlJson parse(const std::string& str, std::string& error);
};

//=============================================================================
// CXXRTL Debug Server

class VlCxxrtlServer final {
    // TYPES
    struct ItemInfo {
        std::string name;
        std::string type;  // "value" or "wire" or "memory"
        int width;
        int depth;
        uint32_t flags;  // CXXRTL flags (input, output, driven_sync, etc.)
        void* curr;      // Pointer to current value
        void* next;      // Pointer to next value (if applicable)
    };

    // STATE
    int m_listenFd = -1;
    int m_clientFd = -1;
    std::thread m_serverThread;
    bool m_running = false;
    VerilatedModel* m_modelp = nullptr;
    VerilatedSyms* m_symsp = nullptr;
    std::map<std::string, std::vector<ItemInfo>> m_scopeItems;
    std::map<int, ItemInfo> m_itemRefs;  // Reference ID -> item
    int m_nextRefId = 1;

    // METHODS
    void serverLoop();
    void handleClient(int clientFd);
    std::string receiveMessage(int fd);
    void sendMessage(int fd, const std::string& msg);
    VlCxxrtlJson handleCommand(const VlCxxrtlJson& cmd);
    VlCxxrtlJson cmdGreeting(const VlCxxrtlJson& cmd);
    VlCxxrtlJson cmdListScopes(const VlCxxrtlJson& cmd);
    VlCxxrtlJson cmdListItems(const VlCxxrtlJson& cmd);
    VlCxxrtlJson cmdReferenceItems(const VlCxxrtlJson& cmd);
    VlCxxrtlJson cmdGetSimulationStatus(const VlCxxrtlJson& cmd);
    void buildScopeMap();
    std::vector<std::string> getScopeHierarchy();

public:
    VlCxxrtlServer() = default;
    ~VlCxxrtlServer();

    // Start server on given port
    void start(int port, VerilatedModel* modelp, VerilatedSyms* symsp);

    // Stop server
    void stop();

    // Check if server is running
    bool isRunning() const { return m_running; }

    // Get current simulation time from model
    uint64_t time() const;
};

#endif  // VERILATOR_VERILATED_CXXRTL_SERVER_H_
