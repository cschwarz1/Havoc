#ifndef HAVOCCLIENT_HCDIALOGBUILDER_H
#define HAVOCCLIENT_HCDIALOGBUILDER_H

#include <QtCore/QVariant>
#include <QtWidgets/QApplication>
#include <QtWidgets/QComboBox>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QListWidget>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QSpacerItem>
#include <QtWidgets/QSplitter>
#include <QtWidgets/QStackedWidget>
#include <QtWidgets/QTextEdit>
#include <QtWidgets/QWidget>

QT_BEGIN_NAMESPACE

struct Builder {
    std::string  name;
    py11::object instance;
    QWidget*     widget;
};

class HcDialogBuilder : public QDialog
{
public:
    QGridLayout*    gridLayout        = nullptr;
    QGridLayout*    gridLayout_2      = nullptr;
    QLabel*         LabelPayload      = nullptr;
    QComboBox*      ComboPayload      = nullptr;
    QSpacerItem*    horizontalSpacer  = nullptr;
    QSplitter*      SplitterTopBottom = nullptr;
    QSplitter*      SplitterLeftRight = nullptr;
    QStackedWidget* StackedBuilders   = nullptr;
    QWidget*        ProfileWidget     = nullptr;
    QPushButton*    ButtonGenerate    = nullptr;
    QPushButton*    ButtonSaveProfile = nullptr;
    QPushButton*    ButtonLoadProfile = nullptr;
    QListWidget*    ListProfiles      = nullptr;
    QTextEdit*      TextBuildLog      = nullptr;
    bool            ErrorReceived     = false;

    std::vector<Builder> Builders = {};

    explicit HcDialogBuilder( QWidget* parent = nullptr );
    ~HcDialogBuilder();

    auto retranslateUi() -> void;

    auto AddBuilder(
        const std::string&  name,
        const py11::object& object
    ) -> void;

private:
    auto BuilderObject(
        const std::string& name
    ) -> std::optional<py11::object>;

private slots:
    auto clickedGenerate(
        void
    ) -> void;

    auto clickedProfileSave(
        void
    ) -> void;

    auto clickedProfileLoad(
        void
    ) -> void;

    auto itemSelectProfile(
        QListWidgetItem *item
    ) -> void;

    auto contextMenuProfile(
        const QPoint& pos
    ) -> void;

Q_SIGNALS:
    auto EventBuildLog(
        const QString& log
    ) -> void;
};

QT_END_NAMESPACE

#endif //HAVOCCLIENT_HCDIALOGBUILDER_H
