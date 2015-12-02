/*
Copyright (c) 2014-15 Ableton AG, Berlin

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
*/

#include "LogUtils.hpp"

#include "estd/memory.hpp"
#include "Warnings.hpp"

SUPPRESS_WARNINGS
#include <boost/assert.hpp>
#if defined(USE_BOOST_LOG)
#include <boost/log/core.hpp>
#include <boost/log/sinks/sync_frontend.hpp>
#include <boost/log/trivial.hpp>
#else
#include <QtCore/QDebug>
#endif
RESTORE_WARNINGS

#include <algorithm>

namespace aqt
{
namespace log_utils
{

LogTrackerImpl::~LogTrackerImpl() = default;

#if defined(USE_BOOST_LOG)
namespace
{
class BoostLogTracker : public LogTrackerImpl
{
  class TrackingSink : public boost::log::sinks::sink
  {
    LogTracker& mTracker;

  public:
    TrackingSink(LogTracker& tracker)
      : sink(false)
      , mTracker(tracker)
    {
    }

    // public member functions
    bool will_consume(boost::log::attribute_value_set const&) override
    {
      return true;
    }

    void consume(boost::log::record_view const& rec) override
    {
      struct LevelVisitor {
        LogTracker::Severity& mTarget;

        LevelVisitor(LogTracker::Severity& target)
          : mTarget(target)
        {
        }

        using result_type = void;
        result_type operator()(boost::log::trivial::severity_level level) const
        {
          mTarget = trivialToSeverity(level);
        }

        LogTracker::Severity trivialToSeverity(
          boost::log::trivial::severity_level level) const
        {
          switch (level) {
          case boost::log::trivial::trace:
            return LogTracker::kTrace;
          case boost::log::trivial::debug:
            return LogTracker::kDebug;
          case boost::log::trivial::info:
            return LogTracker::kInfo;
          case boost::log::trivial::warning:
            return LogTracker::kWarn;
          case boost::log::trivial::error:
            return LogTracker::kError;
          case boost::log::trivial::fatal:
            return LogTracker::kFatal;
          }

          BOOST_ASSERT(false);
          return LogTracker::kFatal;
        }
      };

      struct MsgVisitor {
        std::string& mTarget;
        MsgVisitor(std::string& target)
          : mTarget(target)
        {
        }

        using result_type = void;
        result_type operator()(std::string msg) const
        {
          mTarget = msg;
        }
      };

      LogTracker::Severity severity;
      std::string msg;

      boost::log::visit<boost::log::trivial::severity_level>(
        "Severity", rec, LevelVisitor(severity));
      boost::log::visit<std::string>("Message", rec, MsgVisitor(msg));

      mTracker.mMessages.emplace_back(std::make_tuple(severity, msg));
    }

    void flush() override
    {
    }
  };
  boost::shared_ptr<TrackingSink> mpSink;

public:
  BoostLogTracker(LogTracker& tracker)
  {
    mpSink = boost::make_shared<TrackingSink>(tracker);
    boost::log::core::get()->add_sink(mpSink);
  }

  ~BoostLogTracker()
  {
    boost::log::core::get()->remove_sink(mpSink);
  }
};
} // anon namespace

#else

namespace
{

class QtLogTracker;

static QtLogTracker* sLogTracker;

class QtLogTracker : public LogTrackerImpl
{
  QtMessageHandler mPreviousHandler;
  LogTracker& mTracker;

public:
  QtLogTracker(LogTracker& tracker)
    : mTracker(tracker)
  {
    mPreviousHandler = qInstallMessageHandler(&qtMessageHandler);
    sLogTracker = this;
  }

  ~QtLogTracker()
  {
    sLogTracker = nullptr;
    qInstallMessageHandler(mPreviousHandler);
  }

  static void qtMessageHandler(QtMsgType type,
                               const QMessageLogContext& context,
                               const QString& msg)
  {
    sLogTracker->messageHandler(type, context, msg);
  }

  LogTracker::Severity qtMsgTypeToSeverity(QtMsgType type)
  {
    switch (type) {
    case QtDebugMsg:
      return LogTracker::kDebug;
    case QtInfoMsg:
      return LogTracker::kDebug;
    case QtWarningMsg:
      return LogTracker::kWarn;
    case QtCriticalMsg:
      return LogTracker::kError;
    case QtFatalMsg:
      return LogTracker::kFatal;
#if QT_VERSION >= 0x050500
    case QtInfoMsg:
      return LogTracker::kInfo;
#endif
    }

    BOOST_ASSERT(false);
    return LogTracker::kError;
  }

  void messageHandler(QtMsgType type,
                      const QMessageLogContext& /*context*/,
                      const QString& msg)
  {
    mTracker.mMessages.emplace_back(
      std::make_tuple(qtMsgTypeToSeverity(type), msg.toStdString()));
  }
};
} // anon namespace

#endif

//----------------------------------------------------------------------------------------

LogTracker::LogTracker()
#if defined(USE_BOOST_LOG)
  : mImpl(estd::make_unique<BoostLogTracker>(*this))
#else
  : mImpl(estd::make_unique<QtLogTracker>(*this))
#endif
{
}

size_t LogTracker::messageCount(Severity severity) const
{
  return static_cast<size_t>(
    std::count_if(mMessages.begin(), mMessages.end(),
                  [severity](const std::tuple<Severity, std::string>& tup) {
                    return std::get<0>(tup) == severity;
                  }));
}

} // namespace log_utils
} // namespace aqt
