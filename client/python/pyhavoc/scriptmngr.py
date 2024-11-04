from _pyhavoc import core

import os
import sys
import importlib.util

##
## Callback function once the user decides
## to load a script into the client
##
@core.HcIoScriptLoadCallback
def _HcPyScriptLoad(
    _script_path: str
) -> None:
    print( f"[*] loading script: {_script_path}" )

    script_name = os.path.splitext( os.path.basename( _script_path ) )[ 0 ]

    module_name = "HcScript." + script_name
    script_spec = importlib.util.spec_from_file_location( module_name, _script_path )
    module      = importlib.util.module_from_spec( script_spec )

    script_spec.loader.exec_module( module )

    return


class HcPyScriptMngrStdOutErrHandler:
    def __init__( self ):
        self.buffer = ''
        return

    def write( self, data ):
        self.buffer += data
        if '\n' in data:
            self.flush()

    def flush( self ):
        if len( self.buffer ) > 0:
            core.HcIoConsoleWriteStdOut( self.buffer.rstrip( '\n' ).encode( 'utf-8' ) )
            self.buffer = ""


##
## redirect StdOut and StdErr
##
## TODO: make own class or add param for
##       StdErr to pretty print it as red
##
sys.stdout = HcPyScriptMngrStdOutErrHandler()
sys.stderr = HcPyScriptMngrStdOutErrHandler()
