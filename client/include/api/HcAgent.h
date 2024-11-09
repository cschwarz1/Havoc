#ifndef HAVOCCLIENT_API_HCAGENT_H
#define HAVOCCLIENT_API_HCAGENT_H

#include <Common.h>
#include <api/Engine.h>

//
// meta agent api functions
//

auto HcAgentRegisterInterface(
    const std::string&  type,
    const py11::object& object
) -> void;

auto HcAgentRegisterMenuAction(
    const std::string&  agent_type,
    const std::string&  name,
    const std::string&  icon_path,
    const py11::object& callback
) -> void;

auto HcAgentRegisterInitialize(
    const std::string&  type,
    const py11::object& callback
) -> void;

//
// agent instance api functions
//

auto HcAgentRegisterIcon(
    const std::string& uuid,
    const std::string& icon
) -> void;

auto HcAgentRegisterIconName(
    const std::string& uuid,
    const std::string& name
) -> void;

auto HcAgentSetElevated(
    const std::string& uuid,
    const bool         elevated
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

//
// agent profile & generation functions
//

auto HcAgentProfileBuild(
    const std::string& agent_type,
    const json&        profile
) -> py::bytes;

auto HcAgentProfileSelect(
    const std::string& agent_type = ""
) -> std::optional<json>;

auto HcAgentProfileQuery(
    const std::string& profile
) -> json;

auto HcAgentProfileList(
    void
) -> std::vector<json>;

#endif //HAVOCCLIENT_API_HCAGENT_H
