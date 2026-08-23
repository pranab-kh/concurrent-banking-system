#include "StressTestRunner.h"

#include <QDir>
#include <QFile>
#include <QStandardPaths>
#include <QDebug>

StressTestRunner::StressTestRunner(QObject *parent)
    : QObject(parent)
{
    // stress_test lives in the BACKEND project's build directory
    // (~/concurrent-banking-system/build/stress_test), not this frontend
    // project's own build folder. Resolved relative to the home directory
    // rather than hardcoded, so this doesn't break if the repo moves.
    const QString home = QDir::homePath();
    m_executablePath = home + "/concurrent-banking-system/build/stress_test";

    connect(&m_process, &QProcess::readyReadStandardOutput,
            this, &StressTestRunner::onReadyRead);
    connect(&m_process, &QProcess::readyReadStandardError,
            this, &StressTestRunner::onReadyRead);
    connect(&m_process,
            QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, &StressTestRunner::onProcessFinished);
    connect(&m_process, &QProcess::errorOccurred,
            this, &StressTestRunner::onErrorOccurred);

    // Merge stdout/stderr into one stream so output appears in the order
    // it was actually printed, rather than stderr arriving in its own
    // separate batch.
    m_process.setProcessChannelMode(QProcess::MergedChannels);
}

void StressTestRunner::run()
{
    if (isRunning())
    {
        qDebug() << "stress_test already running, ignoring duplicate run() call";
        return;
    }

    if (!QFile::exists(m_executablePath))
    {
        emit outputReceived(
            QStringLiteral("ERROR: stress_test executable not found at %1\n"
                            "Build it first: cmake --build build --target stress_test\n")
                .arg(m_executablePath));
        emit finished(-1, false);
        return;
    }

    emit outputReceived(QStringLiteral("Starting stress_test...\n\n"));

    // stress_test reads BANK_DB_URL from its own environment, same as
    // bank_server — QProcess inherits the parent (appbank) process's
    // environment by default, so as long as appbank itself was launched
    // with BANK_DB_URL set, this picks it up automatically.
    m_process.start(m_executablePath, QStringList());

    emit runningChanged();
}

void StressTestRunner::stop()
{
    if (!isRunning())
        return;

    m_process.kill();
}

void StressTestRunner::onReadyRead()
{
    const QByteArray chunk = m_process.readAll();
    emit outputReceived(QString::fromUtf8(chunk));
}

void StressTestRunner::onProcessFinished(int exitCode, QProcess::ExitStatus exitStatus)
{
    const bool crashed = (exitStatus == QProcess::CrashExit);
    emit outputReceived(
        QStringLiteral("\n--- stress_test %1 (exit code %2) ---\n")
            .arg(crashed ? "crashed" : "finished")
            .arg(exitCode));
    emit finished(exitCode, crashed);
    emit runningChanged();
}

void StressTestRunner::onErrorOccurred(QProcess::ProcessError error)
{
    emit outputReceived(
        QStringLiteral("ERROR launching stress_test: %1\n").arg(m_process.errorString()));
    emit runningChanged();
}
