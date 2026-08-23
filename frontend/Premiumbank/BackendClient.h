#ifndef BACKENDCLIENT_H
#define BACKENDCLIENT_H

#include <QObject>
#include <QWebSocket>

// Handles all WebSocket communication with the Drogon backend.
//
// The backend registers three separate WebSocket paths:
//   /login           -> Authentication_Controller (login)
//   /create_account  -> Authentication_Controller (account creation)
//   /transaction     -> Transaction_Controller (deposit/withdraw/transfer)
//
// Each path gets its own QWebSocket connection below since Drogon's
// WebSocketController routes by path at connection time, not per-message.
class BackendClient : public QObject
{
    Q_OBJECT

    // Populated from the login response *only* if the backend includes
    // these fields in a JSON reply (e.g. {"status":"success","full_name":"...",
    // "balance_cents": 274497}). Stay at their defaults ("" / 0) otherwise —
    // QML should treat 0/empty as "not available yet" and fall back to
    // placeholder text, since this data isn't guaranteed until the backend
    // actually sends it.
    Q_PROPERTY(QString accountName READ accountName NOTIFY accountInfoChanged)
    Q_PROPERTY(int accountId READ accountId NOTIFY accountInfoChanged)
    Q_PROPERTY(qint64 accountBalanceCents READ accountBalanceCents NOTIFY accountInfoChanged)
    Q_PROPERTY(bool accountInfoAvailable READ accountInfoAvailable NOTIFY accountInfoChanged)

public:
    explicit BackendClient(QObject *parent = nullptr);

    // Call once at startup (or before first use) to open all three sockets.
    Q_INVOKABLE void connectToServer();

    Q_INVOKABLE void login(int userId, const QString &password);

    // Re-fetches this user's account/balance from the DB on demand, over
    // the same /login socket. No password required — the server accepts
    // this only because the client already authenticated at the original
    // login. Reuses loginResult/accountInfoChanged, same as login().
    Q_INVOKABLE void refresh(int userId);

    // Clears cached account state and drops back to "not logged in".
    // Purely local — no message is sent to the backend, so the server's
    // Login_Status row is left as-is until the row naturally goes stale
    // or a future logout protocol message is added.
    Q_INVOKABLE void logout();

    Q_INVOKABLE void createAccount(const QString &userId,      // optional, empty string if new user
                                    const QString &fullName,
                                    const QString &address,
                                    const QString &mobile,
                                    const QString &email,
                                    const QString &gender,
                                    const QString &nid,
                                    const QString &accountType,
                                    const QString &password);

    Q_INVOKABLE void sendMoney(int accountId,
                                int toAccount,
                                qint64 amountCents,
                                const QString &remarks);

    // Deposits into accountId. Backend can only be funded by hand-editing
    // the DB right now; this is the actual UI path for it, sent over the
    // same /transaction socket sendMoney() uses, just with
    // transaction_type "DEPOSIT" instead of "TRANSFER".
    Q_INVOKABLE void deposit(int accountId,
                              qint64 amountCents,
                              const QString &remarks);

    // Withdraws from accountId. Same socket/pattern as deposit(), with
    // transaction_type "WITHDRAW". Backend's Bank::withdraw already
    // supports this (see bank.hpp) — this just exposes it to QML.
    Q_INVOKABLE void withdraw(int accountId,
                               qint64 amountCents,
                               const QString &remarks);

    QString accountName() const { return m_accountName; }
    int accountId() const { return m_accountId; }
    qint64 accountBalanceCents() const { return m_accountBalanceCents; }
    bool accountInfoAvailable() const { return m_accountInfoAvailable; }

signals:
    void connected();
    void disconnected();

    // Generic result signals. success is best-effort: derived from parsing
    // the backend's response (JSON {"status":...} if present, otherwise a
    // few known plain-text markers like "SUCCESS" / "ERROR" / "Invalid...").
    void loginResult(bool success, const QString &message);
    void createAccountResult(bool success, const QString &message);
    void transactionResult(bool success, const QString &message);

    // Fires whenever accountName/accountId/accountBalanceCents change
    // (currently only after a login response that includes those fields).
    void accountInfoChanged();

private slots:
    void onLoginConnected();
    void onLoginDisconnected();
    void onLoginMessage(const QString &message);

    void onAccountConnected();
    void onAccountMessage(const QString &message);

    void onTransactionConnected();
    void onTransactionMessage(const QString &message);

private:
    // Interprets a raw backend reply (JSON or plain string) as success/fail.
    static bool interpretSuccess(const QString &message);

    // Pulls full_name/account_id/balance_cents out of a login JSON reply,
    // if present. No-op (leaves cached values untouched) if the message
    // isn't JSON or doesn't contain these fields.
    void updateAccountInfoFromLoginMessage(const QString &message);

    // Pulls balance_cents out of a successful deposit/withdraw/transfer
    // reply so the cached balance (and anything bound to it in QML)
    // stays current without requiring a re-login.
    void updateBalanceFromTransactionMessage(const QString &message);

    QWebSocket m_loginSocket;
    QWebSocket m_accountSocket;
    QWebSocket m_transactionSocket;

    QString m_serverHost = QStringLiteral("localhost");
    int m_serverPort = 8080;

    QString m_accountName;
    int m_accountId = 0;
    qint64 m_accountBalanceCents = 0;
    bool m_accountInfoAvailable = false;

    // account_id of the most recently sent deposit/withdraw/transfer
    // request. The backend's response doesn't echo account_id back, so
    // this is how updateBalanceFromTransactionMessage() knows whether an
    // incoming balance_cents belongs to *this* logged-in user's account
    // or some other account (e.g. an admin depositing into someone
    // else's account) — see updateBalanceFromTransactionMessage.
    int m_lastTransactionTargetAccountId = 0;
};

#endif