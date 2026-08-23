#ifndef STRESSTESTRUNNER_H
#define STRESSTESTRUNNER_H

#include <QObject>
#include <QProcess>
#include <QString>

// Launches the backend's stress_test executable (a separate C++ binary,
// not something callable as a function) and streams its stdout live to
// QML as it runs. stress_test spawns its own worker threads, submits
// concurrent transfer requests directly against the database, and prints
// plain-text progress/results — this class just captures that output.
class StressTestRunner : public QObject
{
    Q_OBJECT

    Q_PROPERTY(bool running READ isRunning NOTIFY runningChanged)

public:
    explicit StressTestRunner(QObject *parent = nullptr);

    Q_INVOKABLE void run();
    Q_INVOKABLE void stop();

    bool isRunning() const { return m_process.state() != QProcess::NotRunning; }

signals:
    void runningChanged();

    // Emitted for each new chunk of output as it streams in (may be a
    // partial line — QML just appends it to a growing text buffer).
    void outputReceived(const QString &text);

    // Emitted once when the process exits, with the exit code and
    // whether it crashed vs exited normally.
    void finished(int exitCode, bool crashed);

private slots:
    void onReadyRead();
    void onProcessFinished(int exitCode, QProcess::ExitStatus exitStatus);
    void onErrorOccurred(QProcess::ProcessError error);

private:
    QProcess m_process;
    QString m_executablePath;
};

#endif
