#ifndef HAVOCCLIENT_API_ENGINE_H
#define HAVOCCLIENT_API_ENGINE_H

#include <Common.h>

#include <api/HcCore.h>
#include <api/HcScriptManager.h>
#include <api/HcAgent.h>
#include <api/HcListener.h>

#define HcPythonAcquire() py11::gil_scoped_acquire gil

class HcPyEngine {
    PyThreadState* state = { 0 };

public:
    py11::scoped_interpreter* guard{};

    explicit HcPyEngine();
    ~HcPyEngine();
};

#endif
