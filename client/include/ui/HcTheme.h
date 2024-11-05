#ifndef HAVOCCLIENT_HCTHEME_H
#define HAVOCCLIENT_HCTHEME_H

#include <Common.h>

#define HcDefineColorMethod( color )                                                       \
        private:                                                                           \
            QColor color;                                                                  \
            Q_PROPERTY(QColor color READ get ## color WRITE set ## color DESIGNABLE true); \
        public:                                                                            \
            auto get ## color() const -> QColor;                                           \
            auto set ## color( QColor c ) -> void;

#define HcDeclareColorMethod( color )                               \
    auto HcTheme::get ## color() const -> QColor { return color; }  \
    auto HcTheme::set ## color( QColor c ) -> void { color = c; }

//
// TODO: rewrite the HcTheme system and include
//       more descriptive methods for specific
//       themes such as.
//       - AgentTableEntryHiddenBackground
//       - AgentTableEntryBackground
//       - AgentConnectionArrowDirect
//       - AgentConnectionArrowPivot
//       - AgentConnectionArrowDisconnected
//       - AgentConnectionArrowUnresponsive
//       - AgentConnectionArrowDirectPulsate
//       - AgentConnectionArrowPivotPulsate
//       - AgentConsoleBackground
//       - AgentConsoleForeground
//       - AgentConsole{Colors}
//       and more options to style the UI
//

class HcTheme : public QWidget {
    Q_OBJECT

public:
    explicit HcTheme( QWidget* parent = nullptr );
    ~HcTheme();

    HcDefineColorMethod( Background )
    HcDefineColorMethod( CurrentLine )
    HcDefineColorMethod( Foreground )
    HcDefineColorMethod( Comment )
    HcDefineColorMethod( Cyan )
    HcDefineColorMethod( Green )
    HcDefineColorMethod( Orange )
    HcDefineColorMethod( Pink )
    HcDefineColorMethod( Purple )
    HcDefineColorMethod( Red )
    HcDefineColorMethod( Yellow )

    static auto getHtmlColorStart( const QColor& color ) -> QString;
    static auto getHtmlBold() -> QString;
    static auto getHtmlUnderline() -> QString;
    static auto getHtmlEnd() -> QString;
};

#endif //HAVOCCLIENT_HCTHEME_H
