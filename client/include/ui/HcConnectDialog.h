#ifndef HAVOCCLIENT_HCCONNECTDIALOG_H
#define HAVOCCLIENT_HCCONNECTDIALOG_H

#include <Common.h>
#include <ui/HcLineEdit.h>

#include <QtCore/QVariant>
#include <QtWidgets/QApplication>
#include <QtWidgets/QDialog>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QListWidget>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QWidget>

QT_BEGIN_NAMESPACE

class HcConnectDialog final : public QDialog
{
    bool PressedConnect = false;

public:
    QGridLayout* gridLayout        = {};
    QHBoxLayout* horizontalLayout  = {};
    QWidget*     ConnectionWidget  = {};
    QHBoxLayout* TitleLayout       = {};
    QLabel*      LabelImage        = {};
    QLabel*      LabelHavoc        = {};
    HcLineEdit*  InputProfileName  = {};
    HcLineEdit*  InputHost         = {};
    HcLineEdit*  InputPort         = {};
    HcLineEdit*  InputUsername     = {};
    HcLineEdit*  InputPassword     = {};
    QPushButton* ButtonConnect     = {};
    QPushButton* ButtonAdd         = {};
    QListWidget* ListConnection    = {};
    QAction*     ActionPassBlinder = {};
    bool         PassBlinderToggle = false;

    explicit HcConnectDialog( );
    ~HcConnectDialog( ) override;

    auto sanityCheckInput() -> bool;
    auto retranslateUi() -> void;
    auto start() -> json;
    auto connected() const -> bool;
};

QT_END_NAMESPACE

#endif //HAVOCCLIENT_HCCONNECTDIALOG_H
