#ifndef HAVOCCLIENT_COMMON_H
#define HAVOCCLIENT_COMMON_H

#define CPPHTTPLIB_OPENSSL_SUPPORT
#define PYBIND11_NO_ASSERT_GIL_HELD_INCREF_DECREF

/* std libraries */
#include <memory>
#include <format>
#include <unordered_map>

/* external libs */
#include <toml.hpp>
#include <nlohmann/json.hpp>
#include <spdlog/spdlog.h>
#include <pybind11/pybind11.h>
#include <pybind11/embed.h>
#include <pybind11/stl.h>
#include <pybind11/complex.h>
#include <pybind11/functional.h>
#include <pybind11_json/pybind11_json.hpp>

/* Qt libraries */
#include <QApplication>
#include <QMainWindow>
#include <QWidget>
#include <QMessageBox>
#include <QToolButton>
#include <QFileDialog>
#include <QThread>
#include <QMenu>
#include <QtConcurrent/QtConcurrent>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QSplashScreen>
#include <QtCore5Compat/QTextCodec>
#include <QTimer>

/* define namepsace */
using     json = nlohmann::json;
namespace py11 = pybind11;

/* typedefs */
typedef uint8_t  u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

typedef void*  C_PTR;
typedef size_t U_PTR;

#endif //HAVOCCLIENT_COMMON_H
