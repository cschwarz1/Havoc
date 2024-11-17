##
## import havoc specific libs
##
import shiboken6
from _pyhavoc import core
from _pyhavoc import ui
import ctypes

##
## import qt ui library
##
from PySide6.QtCore import *
from PySide6.QtGui import *
from PySide6.QtWidgets import *

from shiboken6 import Shiboken


class HcPayloadView:

    def __init__(
        self,
        builder_widget: ctypes.c_size_t,
        builder_dialog: ctypes.c_size_t,
        builder_name  : str
    ):
        self._builder_widget: QWidget = Shiboken.wrapInstance( builder_widget, QWidget )
        self._builder_dialog: QDialog = Shiboken.wrapInstance( builder_dialog, QDialog )
        self._builder_name  : str     = builder_name

    ##
    ## main entrypoint what the
    ## Havoc client is going to call
    ##
    def _hc_main( self ) -> None:
        self.main( self._builder_widget )

    def set_builder_size( self, width, height ):
        return self._builder_dialog.resize( width, height )

    ##
    ## main function to be executed
    ## should create the widgets inputs
    ## for the payload builder
    ##
    def main( self, widget: QWidget ):
        pass

    ##
    ## sanity check the given input
    ## return:
    ##  true  -> successful checked the input and nothing is wrong
    ##  false -> failed to check and something went wrong
    ##
    def sanity_check( self ) -> bool:
        return True

    ##
    ## pressing "generate" action or
    ## while saving a profile
    ##
    def generate( self, profile_save: bool = False ) -> dict:
        pass

    ##
    ## load profile
    ##
    def profile_load( self, profile: dict ) -> None:
        pass

    ##
    ## save profile
    ##
    def profile_save( self ) -> dict:
        pass


class HcListenerView:

    def __init__(
        self,
        listener_widget: ctypes.c_size_t,
        listener_name  : str
    ):
        self._listener_widget : QWidget = Shiboken.wrapInstance( listener_widget, QWidget )
        self._listener_name   : str     = listener_name

    ##
    ## main entrypoint what the
    ## Havoc client is going to call
    ##
    def _hc_main( self ):
        self.main( self.listener_widget() )

    ##
    ## return given listener protocol name
    ##
    def listener_name( self ) -> str:
        return self._listener_name

    ##
    ## get our protocol widget
    ##
    def listener_widget( self ) -> QWidget:
        return self._listener_widget

    ##
    ## main function to be executed
    ## should create the widgets inputs
    ## for the listener
    ##
    def main( self, widget: QWidget ):
        pass

    ##
    ## method invoked once the operator wants to edit the listener
    ##
    def edit( self, config: dict ):
        pass

    ##
    ## sanity check the given input
    ## return:
    ##  true  -> successful checked the input and nothing is wrong
    ##  false -> failed to check and something went wrong
    ##
    def sanity_check( self ) -> bool:
        return True

    ##
    ## pressing "save" action
    ##
    def save( self ) -> dict:
        pass


def HcUiGetStyleSheet() -> str:
    return ui.HcUiGetStyleSheet()


def HcListenerPopupSelect( protocol: str ) -> dict:
    return ui.HcListenerPopupSelect( protocol )


def HcUiMessageBox( icon: QMessageBox.Icon, title: str, text: str ) -> None:
    ##
    ## heh a dirty fix. maybe there is a better way to
    ## do it but this is enough for now.
    ##
    i = 0
    if icon is QMessageBox.Information:
        i = 1
    elif icon is QMessageBox.Warning:
        i = 2
    elif icon is QMessageBox.Critical:
        i = 3
    elif icon is QMessageBox.Question:
        i = 4

    ui.HcUiMessageBox( i, title, text )

    return


def HcUiListenerRegisterView( protocol: str ):

    def _register( listener_view ):
        ui.HcUiListenerRegisterView( protocol, listener_view )

    return _register


def HcUiBuilderRegisterView( payload: str ):

    def _register( builder_view ):
        ui.HcUiBuilderRegisterView( payload, builder_view )

    return _register


##
## https://stackoverflow.com/questions/14780517/toggle-switch-in-qt
##
class HcSwitch( QAbstractButton ):
    def __init__(self, parent=None, track_radius=10, thumb_radius=8):
        super().__init__(parent=parent)
        self.setCheckable(True)
        self.setSizePolicy(QSizePolicy.Fixed, QSizePolicy.Fixed)

        self._track_radius = track_radius
        self._thumb_radius = thumb_radius

        self._margin = max(0, self._thumb_radius - self._track_radius)
        self._base_offset = max(self._thumb_radius, self._track_radius)
        self._end_offset = {
            True: lambda: self.width() - self._base_offset,
            False: lambda: self._base_offset,
        }
        self._offset = self._base_offset

        palette = self.palette()
        if self._thumb_radius > self._track_radius:
            self._track_color = {
                True: palette.highlight(),
                False: palette.dark(),
            }
            self._thumb_color = {
                True: palette.highlight(),
                False: palette.light(),
            }
            self._text_color = {
                True: palette.highlightedText().color(),
                False: palette.dark().color(),
            }
            self._thumb_text = {
                True: '',
                False: '',
            }
            self._track_opacity = 0.5
        else:
            self._thumb_color = {
                True: palette.highlightedText(),
                False: palette.light(),
            }
            self._track_color = {
                True: palette.highlight(),
                False: palette.dark(),
            }
            self._text_color = {
                True: palette.highlight().color(),
                False: palette.dark().color(),
            }
            self._thumb_text = {
                True: '✔',
                False: '✕',
            }
            self._track_opacity = 1

    @Property(int)
    def offset(self):
        return self._offset

    @offset.setter
    def offset(self, value):
        self._offset = value
        self.update()

    def sizeHint(self):  # pylint: disable=invalid-name
        return QSize(
            4 * self._track_radius + 2 * self._margin,
            2 * self._track_radius + 2 * self._margin,
            )

    def setChecked(self, checked):
        super().setChecked(checked)
        self.offset = self._end_offset[checked]()

    def resizeEvent(self, event):
        super().resizeEvent(event)
        self.offset = self._end_offset[self.isChecked()]()

    def paintEvent(self, event):  # pylint: disable=invalid-name, unused-argument
        p = QPainter(self)
        p.setRenderHint(QPainter.Antialiasing, True)
        p.setPen(Qt.NoPen)
        track_opacity = self._track_opacity
        thumb_opacity = 1.0
        text_opacity = 1.0
        if self.isEnabled():
            track_brush = self._track_color[self.isChecked()]
            thumb_brush = self._thumb_color[self.isChecked()]
            text_color = self._text_color[self.isChecked()]
        else:
            track_opacity *= 0.8
            track_brush = self.palette().shadow()
            thumb_brush = self.palette().mid()
            text_color = self.palette().shadow().color()

        p.setBrush(track_brush)
        p.setOpacity(track_opacity)
        p.drawRoundedRect(
            self._margin,
            self._margin,
            self.width() - 2 * self._margin,
            self.height() - 2 * self._margin,
            self._track_radius,
            self._track_radius,
            )
        p.setBrush(thumb_brush)
        p.setOpacity(thumb_opacity)
        p.drawEllipse(
            self.offset - self._thumb_radius,
            self._base_offset - self._thumb_radius,
            2 * self._thumb_radius,
            2 * self._thumb_radius,
            )
        p.setPen(text_color)
        p.setOpacity(text_opacity)
        font = p.font()
        font.setPixelSize(1.5 * self._thumb_radius)
        p.setFont(font)
        p.drawText(
            QRectF(
                self.offset - self._thumb_radius,
                self._base_offset - self._thumb_radius,
                2 * self._thumb_radius,
                2 * self._thumb_radius,
                ),
            Qt.AlignCenter,
            self._thumb_text[self.isChecked()],
        )

    def mouseReleaseEvent(self, event):  # pylint: disable=invalid-name
        super().mouseReleaseEvent(event)
        if event.button() == Qt.LeftButton:
            anim = QPropertyAnimation(self, b'offset', self)
            anim.setDuration(120)
            anim.setStartValue(self.offset)
            anim.setEndValue(self._end_offset[self.isChecked()]())
            anim.start()

    def enterEvent(self, event):  # pylint: disable=invalid-name
        self.setCursor(Qt.PointingHandCursor)
        super().enterEvent(event)
