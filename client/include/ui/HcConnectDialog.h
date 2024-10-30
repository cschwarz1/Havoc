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

class HcConnectDialog : public QDialog
{
    bool PressedConnect = false;

public:
    QGridLayout* gridLayout       = nullptr;
    QHBoxLayout* horizontalLayout = nullptr;
    QWidget*     ConnectionWidget = nullptr;
    QLabel*      LabelHavoc       = nullptr;
    HcLineEdit*  InputProfileName = nullptr;
    HcLineEdit*  InputHost        = nullptr;
    HcLineEdit*  InputPort        = nullptr;
    HcLineEdit*  InputUsername    = nullptr;
    HcLineEdit*  InputPassword    = nullptr;
    QPushButton* ButtonConnect    = nullptr;
    QPushButton* ButtonAdd        = nullptr;
    QListWidget* ListConnection   = nullptr;
    QAction*     ActionPassBlinder= nullptr;
    bool         PassBlinderToggle= false;

    explicit HcConnectDialog( );
    ~HcConnectDialog( ) override;

    auto sanityCheckInput() -> bool;
    auto retranslateUi() -> void;
    auto start() -> json;
    auto connected() const -> bool;
};

QT_END_NAMESPACE

#endif //HAVOCCLIENT_HCCONNECTDIALOG_H
