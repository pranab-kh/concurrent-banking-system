#include "BackendClient.h"

#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>
#include <QUrl>
#include <QDebug>

BackendClient::BackendClient(QObject *parent)
    : QObject(parent)
{
    connect(&m_loginSocket, &QWebSocket::connected,
            this, &BackendClient::onLoginConnected);
    connect(&m_loginSocket, &QWebSocket::disconnected,
            this, &BackendClient::onLoginDisconnected);
    connect(&m_loginSocket, &QWebSocket::textMessageReceived,
            this, &BackendClient::onLoginMessage);

    connect(&m_accountSocket, &QWebSocket::connected,
            this, &BackendClient::onAccountConnected);
    connect(&m_accountSocket, &QWebSocket::textMessageReceived,
            this, &BackendClient::onAccountMessage);

    connect(&m_transactionSocket, &QWebSocket::connected,
            this, &BackendClient::onTransactionConnected);
    connect(&m_transactionSocket, &QWebSocket::textMessageReceived,
            this, &BackendClient::onTransactionMessage);
    connect(&m_loginSocket,
        &QWebSocket::errorOccurred,
        this,
        [this](QAbstractSocket::SocketError error)
        {
            qDebug()
                << "Login WebSocket error:"
                << error
                << m_loginSocket.errorString();

            emit loginResult(
                false,
                "WebSocket error: " +
                m_loginSocket.errorString()
            );
        });
}

void BackendClient::connectToServer()
{
    const QString base = QStringLiteral("ws://%1:%2").arg(m_serverHost).arg(m_serverPort);

    if (m_loginSocket.state() != QAbstractSocket::ConnectedState)
        m_loginSocket.open(QUrl(base + "/login"));

    if (m_accountSocket.state() != QAbstractSocket::ConnectedState)
        m_accountSocket.open(QUrl(base + "/create_account"));

    if (m_transactionSocket.state() != QAbstractSocket::ConnectedState)
        m_transactionSocket.open(QUrl(base + "/transaction"));
}

// ---------------------------------------------------------------------
// Login
// ---------------------------------------------------------------------

void BackendClient::login(int userId, const QString &password)
{
    if (m_loginSocket.state() != QAbstractSocket::ConnectedState)
    {
        qDebug() << "Login socket not connected";
        emit loginResult(false, "Not connected to server");
        return;
    }

    QJsonObject json;
    json["user_id"] = userId;
    json["password"] = password;

    const QString message = QString::fromUtf8(
        QJsonDocument(json).toJson(QJsonDocument::Compact));

    qDebug() << "Sending login:" << message;
    m_loginSocket.sendTextMessage(message);
}

void BackendClient::refresh(int userId)
{
    if (m_loginSocket.state() != QAbstractSocket::ConnectedState)
    {
        qDebug() << "Login socket not connected";
        emit loginResult(false, "Not connected to server");
        return;
    }

    QJsonObject json;
    json["type"] = "refresh";
    json["user_id"] = userId;

    const QString message = QString::fromUtf8(
        QJsonDocument(json).toJson(QJsonDocument::Compact));

    qDebug() << "Sending refresh:" << message;
    m_loginSocket.sendTextMessage(message);
}

void BackendClient::logout()
{
    qDebug() << "Logging out locally (no backend message sent)";

    m_accountName.clear();
    m_accountId = 0;
    m_accountBalanceCents = 0;
    m_accountInfoAvailable = false;
    m_lastTransactionTargetAccountId = 0;

    emit accountInfoChanged();
}

void BackendClient::onLoginConnected()
{
    qDebug() << "Login socket connected";
    emit connected();
}

void BackendClient::onLoginDisconnected()
{
    qDebug() << "Login socket disconnected";
    emit disconnected();
}

void BackendClient::onLoginMessage(const QString &message)
{
    qDebug() << "Login response:" << message;

    const bool success = interpretSuccess(message);
    if (success)
        updateAccountInfoFromLoginMessage(message);

    emit loginResult(success, message);
}

// ---------------------------------------------------------------------
// Account creation
// ---------------------------------------------------------------------

void BackendClient::createAccount(const QString &userId,
                                   const QString &fullName,
                                   const QString &address,
                                   const QString &mobile,
                                   const QString &email,
                                   const QString &gender,
                                   const QString &nid,
                                   const QString &accountType,
                                   const QString &password)
{
    if (m_accountSocket.state() != QAbstractSocket::ConnectedState)
    {
        qDebug() << "Account socket not connected";
        emit createAccountResult(false, "Not connected to server");
        return;
    }

    QJsonObject json;

    // user_id is optional on the backend: send it only if the field was
    // filled in (i.e. an existing user adding another account). Leave it
    // out entirely for brand-new users.
    bool ok = false;
    int userIdInt = userId.toInt(&ok);
    if (ok)
        json["user_id"] = userIdInt;

    json["full_name"] = fullName;
    json["address"] = address;
    json["mobile"] = mobile;
    json["email"] = email;
    json["gender"] = gender;
    json["nid"] = nid;
    json["account_type"] = accountType;
    json["password"] = password;

    const QString message = QString::fromUtf8(
        QJsonDocument(json).toJson(QJsonDocument::Compact));

    qDebug() << "Sending create_account:" << message;
    m_accountSocket.sendTextMessage(message);
}

void BackendClient::onAccountConnected()
{
    qDebug() << "Account socket connected";
}

void BackendClient::onAccountMessage(const QString &message)
{
    qDebug() << "Account creation response:" << message;
    emit createAccountResult(interpretSuccess(message), message);
}

// ---------------------------------------------------------------------
// Transactions
// ---------------------------------------------------------------------

void BackendClient::sendMoney(int accountId,
                               int toAccount,
                               qint64 amountCents,
                               const QString &remarks)
{
    if (m_transactionSocket.state() != QAbstractSocket::ConnectedState)
    {
        qDebug() << "Transaction socket not connected";
        emit transactionResult(false, "Not connected to server");
        return;
    }

    QJsonObject json;
    json["account_id"] = accountId;
    json["transaction_type"] = "TRANSFER";
    json["to_account"] = toAccount;
    json["transaction_amount"] = amountCents;
    json["remarks"] = remarks;

    m_lastTransactionTargetAccountId = accountId;

    const QString message = QString::fromUtf8(
        QJsonDocument(json).toJson(QJsonDocument::Compact));

    qDebug() << "Sending transaction:" << message;
    m_transactionSocket.sendTextMessage(message);
}

void BackendClient::deposit(int accountId,
                             qint64 amountCents,
                             const QString &remarks)
{
    if (m_transactionSocket.state() != QAbstractSocket::ConnectedState)
    {
        qDebug() << "Transaction socket not connected";
        emit transactionResult(false, "Not connected to server");
        return;
    }

    QJsonObject json;
    json["account_id"] = accountId;
    json["transaction_type"] = "DEPOSIT";
    json["transaction_amount"] = amountCents;
    json["remarks"] = remarks;

    m_lastTransactionTargetAccountId = accountId;

    const QString message = QString::fromUtf8(
        QJsonDocument(json).toJson(QJsonDocument::Compact));

    qDebug() << "Sending deposit:" << message;
    m_transactionSocket.sendTextMessage(message);
}

void BackendClient::withdraw(int accountId,
                              qint64 amountCents,
                              const QString &remarks)
{
    if (m_transactionSocket.state() != QAbstractSocket::ConnectedState)
    {
        qDebug() << "Transaction socket not connected";
        emit transactionResult(false, "Not connected to server");
        return;
    }

    QJsonObject json;
    json["account_id"] = accountId;
    json["transaction_type"] = "WITHDRAW";
    json["transaction_amount"] = amountCents;
    json["remarks"] = remarks;

    m_lastTransactionTargetAccountId = accountId;

    const QString message = QString::fromUtf8(
        QJsonDocument(json).toJson(QJsonDocument::Compact));

    qDebug() << "Sending withdraw:" << message;
    m_transactionSocket.sendTextMessage(message);
}

void BackendClient::onTransactionConnected()
{
    qDebug() << "Transaction socket connected";
}void BackendClient::onTransactionMessage(const QString &message)
{
    qDebug() << "Transaction response:" << message;
    updateBalanceFromTransactionMessage(message);
    emit transactionResult(interpretSuccess(message), message);
}

// ---------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------

bool BackendClient::interpretSuccess(const QString &message)
{
    // Try JSON first: {"status": "success", ...} or {"status": "error", ...}
    QJsonParseError err;
    const QJsonDocument doc = QJsonDocument::fromJson(message.toUtf8(), &err);
    if (err.error == QJsonParseError::NoError && doc.isObject())
    {
        const QJsonValue status = doc.object().value("status");
        if (status.isString())
            return status.toString().compare("success", Qt::CaseInsensitive) == 0;
    }

    // Fall back to the plain-text markers the backend currently sends.
    const QString upper = message.trimmed().toUpper();
    if (upper == "SUCCESS")
        return true;

    // Everything else (ERROR, "Invalid username or password", "SERVER_BUSY",
    // "ERROR_BAD_STRUCTURE", connection errors, etc.) is treated as failure.
    return false;
}

void BackendClient::updateAccountInfoFromLoginMessage(const QString &message)
{
    QJsonParseError err;
    const QJsonDocument doc = QJsonDocument::fromJson(message.toUtf8(), &err);
    if (err.error != QJsonParseError::NoError || !doc.isObject())
        return; // Plain-string reply (e.g. "SUCCESS") — no account info to extract yet.

    const QJsonObject obj = doc.object();
    bool changed = false;

    if (obj.contains("full_name") && obj.value("full_name").isString())
    {
        m_accountName = obj.value("full_name").toString();
        changed = true;
    }

    if (obj.contains("account_id") && obj.value("account_id").isDouble())
    {
        m_accountId = obj.value("account_id").toInt();
        changed = true;
    }

    // Accept either "balance_cents" or "balance" (whichever the backend ends
    // up naming it) so this doesn't need another round of changes later.
    if (obj.contains("balance_cents") && obj.value("balance_cents").isDouble())
    {
        m_accountBalanceCents = static_cast<qint64>(obj.value("balance_cents").toDouble());
        changed = true;
    }
    else if (obj.contains("balance") && obj.value("balance").isDouble())
    {
        m_accountBalanceCents = static_cast<qint64>(obj.value("balance").toDouble());
        changed = true;
    }

    if (changed)
    {
        m_accountInfoAvailable = true;
        emit accountInfoChanged();
    }
}

// A deposit/withdraw/transfer reply from the backend
// (Bank::process()) always includes "balance_cents" for the account this
// connection acted on, whether it succeeded or not (0 if the account
// couldn't be found). Pull it in so the dashboard reflects the deposit
// immediately instead of requiring a re-login.
//
// The backend's reply doesn't echo back which account_id it was for, so
// we rely on m_lastTransactionTargetAccountId (set right before sending)
// to know whether this response is about the logged-in user's own
// account or some other account — e.g. an admin depositing into a
// different user's account via the Admin Panel. Only the former should
// update the cached balance; otherwise the logged-in admin's own
// Dashboard would start showing a stranger's balance.
void BackendClient::updateBalanceFromTransactionMessage(const QString &message)
{
    if (m_lastTransactionTargetAccountId != m_accountId)
        return; // this response was about a different account — don't touch our cache

    QJsonParseError err;
    const QJsonDocument doc = QJsonDocument::fromJson(message.toUtf8(), &err);
    if (err.error != QJsonParseError::NoError || !doc.isObject())
        return;

    const QJsonObject obj = doc.object();
    const QJsonValue status = obj.value("status");
    if (!status.isString() || status.toString().compare("success", Qt::CaseInsensitive) != 0)
        return; // don't overwrite the cached balance with a failed attempt's 0

    if (obj.contains("balance_cents") && obj.value("balance_cents").isDouble())
    {
        m_accountBalanceCents = static_cast<qint64>(obj.value("balance_cents").toDouble());
        m_accountInfoAvailable = true;
        emit accountInfoChanged();
    }
}