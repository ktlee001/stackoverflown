// https://github.com/KubaO/stackoverflown/tree/master/tooling
#pragma once

#include "tooling.h"

#if QT_VERSION >= QT_VERSION_CHECK(5, 0, 0)
#define TOOLING_NOSTANDARDPATHS
#include <QStandardPaths>
#endif
#if QT_VERSION >= QT_VERSION_CHECK(5, 1, 0)
#define TOOLING_NOSAVEFILE
#include <QSaveFile>
#endif
#include <QAbstractEventDispatcher>
#include <QCoreApplication>
#include <QFile>
#include <QStringList>
#include <QTimer>
#include <type_traits>

#ifndef QT_HAS_INCLUDE
#ifdef __has_include
#define QT_HAS_INCLUDE(x) __has_include(x)
#else
#define QT_HAS_INCLUDE(x) 0
#endif
#endif

QT_BEGIN_NAMESPACE
void qDeleteInEventHandler(QObject *o);
QT_END_NAMESPACE

namespace tooling {

using QT_PREPEND_NAMESPACE(qDeleteInEventHandler);

#if QT_VERSION < QT_VERSION_CHECK(5, 7, 0)
template <typename T>
constexpr typename std::add_const<T>::type &qAsConst(T &t) noexcept {
   return t;
}
template <typename T>
void qAsConst(const T &&) = delete;
#endif

template <class Fun>
struct SingleShotHelper : QObject {
   QBasicTimer timer;
   Fun function;
   template <class F>
   SingleShotHelper(int msec, QObject *context, F &&fun)
       : QObject(context ? context : QAbstractEventDispatcher::instance()),
         function(std::forward<F>(fun)) {
      timer.start(msec, this);
      if (!context) connect(qApp, SIGNAL(aboutToQuit()), this, SLOT(deleteLater()));
   }
   void timerEvent(QTimerEvent *ev) override {
      if (ev->timerId() != timer.timerId()) return;
      timer.stop();
      function();
      qDeleteInEventHandler(this);
   }
};

using Q_QTimer = QT_PREPEND_NAMESPACE(QTimer);
class QTimerImpl : public Q_QTimer {
  public:
   QTimerImpl(QObject *parent = {});
   ~QTimerImpl() override;
   template <class Fun>
   inline static void singleShot(int msec, QObject *context, Fun &&fun) {
      new SingleShotHelper<Fun>(msec, context, std::forward<Fun>(fun));
   }
   template <class Fun>
   inline static void singleShot(int msec, Fun &&fun) {
      auto *context = QAbstractEventDispatcher::instance();
      Q_ASSERT(context);
      new SingleShotHelper<Fun>(msec, context, std::forward<Fun>(fun));
   }
};

using Q_QFile = QT_PREPEND_NAMESPACE(QFile);
class QSaveFileImpl : public Q_QFile {
  public:
   using Q_QFile::Q_QFile;
   bool commit();
};

class QStandardPathsImpl {
  public:
   static QString findExecutable(const QString &executableName,
                                 const QStringList &paths = {});
};

#ifdef TOOLING_NOSAVEFILE
using QT_PREPEND_NAMESPACE(QSaveFile);
#else
using QSaveFile = QSaveFileImpl;
#endif

#ifdef TOOLING_NOSTANDARDPATHS
using QT_PREPEND_NAMESPACE(QStandardPaths);
#else
using QStandardPaths = QStandardPathsImpl;
#endif

#if QT_VERSION >= QT_VERSION_CHECK(5, 4, 0)
using QT_PREPEND_NAMESPACE(QTimer);
#else
using QTimer = QTimerImpl;
#endif

#ifdef Q_OS_WIN
inline QChar listSeparator() { return ';'; }
#else
inline QChar listSeparator() { return ':'; }
#endif

}  // namespace tooling
