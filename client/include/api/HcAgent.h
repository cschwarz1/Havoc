#ifndef HAVOCCLIENT_API_HCAGENT_H
#define HAVOCCLIENT_API_HCAGENT_H

#include <Common.h>
#include <api/Engine.h>

auto HcAgentRegisterInterface(
    const std::string&  type,
    const py11::object& object
) -> void;

auto HcAgentConsoleWrite(
    const std::string& uuid,
    const std::string& content
) -> void;

auto HcAgentConsoleAddComplete(
    const std::string& uuid,
    const std::string& command,
    const std::string& description
) -> void;

auto HcAgentConsoleLabel(
    const std::string& uuid,
    const std::string& content
) -> void;

auto HcAgentConsoleHeader(
    const std::string& uuid,
    const std::string& header
) -> void;

auto HcAgentRegisterCallback(
    const std::string&  uuid,
    const py11::object& callback
) -> void;

auto HcAgentUnRegisterCallback(
    const std::string& uuid
) -> void;

auto HcAgentExecute(
    const std::string& uuid,
    const json&        data,
    const bool         wait
) -> json;

auto HcAgentRegisterMenuAction(
    const std::string&  agent_type,
    const std::string&  name,
    const std::string&  icon_path,
    const py11::object& callback
) -> void;

auto HcAgentProfileBuild(
    const json& profile
) -> py::bytes;

auto HcAgentProfileSelect(
    const std::string& agent_type = ""
) -> std::optional<json>;

#endif //HAVOCCLIENT_API_HCAGENT_H
