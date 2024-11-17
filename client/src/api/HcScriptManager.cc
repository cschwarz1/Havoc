#include <Havoc.h>
#include <api/HcScriptManager.h>

auto HcIoConsoleWriteStdOut(
    const std::string& text
) -> void {
    emit Havoc->ui->PageScripts->SignalConsoleWrite( text.c_str() );
}

auto HcIoScriptLoadCallback(
    const py11::object& callback
) -> void {
    Havoc->ui->PageScripts->LoadCallback = callback;
}