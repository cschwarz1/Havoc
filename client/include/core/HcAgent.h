#ifndef HAVOCCLIENT_HCAGENT_H
#define HAVOCCLIENT_HCAGENT_H

class HcSessionGraphItem;
class HcAgentConsole;
struct HcAgent;

#include <Common.h>
#include <ui/HcPageAgent.h>
#include <ui/HcSessionGraph.h>

class HcAgentSignals final : public QObject {
    Q_OBJECT

signals:
    auto ConsoleWrite(
        const QString& uuid,
        const QString& text
    ) -> void;

    auto HeartBeat(
        const QString& uuid,
        const QString& time
    ) -> void;
};

struct HcAgent;

class HcAgentTableItem final : public QTableWidgetItem {

public:
    HcAgent* agent  = {};
    bool     ignore = true;

    explicit HcAgentTableItem(
        const QString&          value,
        HcAgent*                agent,
        const Qt::ItemFlag      flags = Qt::ItemIsEditable,
        const Qt::AlignmentFlag align = Qt::AlignCenter
    );
};

struct HcAgent {
    std::string                 uuid;
    std::string                 type;
    std::string                 parent;
    json                        data;
    std::optional<py11::object> interface;
    HcAgentConsole*             console;
    QString                     last;
    std::string                 status;
    bool                        hidden;

    struct {
        HcSessionGraphItem* node;

        struct {
            HcAgentTableItem* Uuid;
            HcAgentTableItem* Internal;
            HcAgentTableItem* Username;
            HcAgentTableItem* Hostname;
            HcAgentTableItem* ProcessPath;
            HcAgentTableItem* ProcessName;
            HcAgentTableItem* ProcessId;
            HcAgentTableItem* ThreadId;
            HcAgentTableItem* Arch;
            HcAgentTableItem* System;
            HcAgentTableItem* Note;
            HcAgentTableItem* Last;
        } table;

        HcAgentSignals signal = {};
    } ui;

    explicit HcAgent(
        const json& metadata
    );

    auto initialize() -> bool;
    auto post() -> void;
    auto remove() -> void;
    auto hide() const -> void;
    auto unhide() const -> void;
    auto disconnected() -> void;
    auto unresponsive() -> void;
    auto healthy() -> void;
};

#endif //HAVOCCLIENT_HCAGENT_H
