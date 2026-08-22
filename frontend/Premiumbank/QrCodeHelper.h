#ifndef QRCODEHELPER_H
#define QRCODEHELPER_H

#include <QObject>
#include <QString>

// Exposes QR code generation to QML. Wraps Project Nayuki's qrcodegen
// library (MIT licensed, vendored as qrcodegen.hpp/.cpp in this same
// directory) so ReceiveMoneyPage.qml can render a real, scannable QR
// code for the logged-in user's actual account number instead of a
// static placeholder image.
class QrCodeHelper : public QObject
{
    Q_OBJECT

public:
    explicit QrCodeHelper(QObject *parent = nullptr) : QObject(parent) {}

    // Returns a self-contained "data:image/svg+xml;utf8,..." URL that can
    // be assigned directly to an Image element's `source` property in QML,
    // e.g.: Image { source: qrCodeHelper.accountQrDataUrl(backend.accountId) }
    //
    // Encodes the text as a plain string (the account ID). If you want scans
    // to open a specific app/action instead of just showing the number,
    // change payloadForAccountId() below to build a URI scheme your app
    // (or another banking app) recognizes.
    Q_INVOKABLE QString accountQrDataUrl(int accountId) const;

private:
    static QString payloadForAccountId(int accountId);
};

#endif
