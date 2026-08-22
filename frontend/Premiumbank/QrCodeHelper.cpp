#include "QrCodeHelper.h"
#include "qrcodegen.hpp"

#include <QByteArray>
#include <QUrl>
#include <sstream>

using qrcodegen::QrCode;

QString QrCodeHelper::payloadForAccountId(int accountId)
{
    // Plain numeric account ID as the QR payload. A scanning app (or a
    // "Scan to Pay" screen elsewhere in this same app) reads this back
    // as a plain string. If a URI scheme is introduced later
    // (e.g. "premiumbank://pay?account=15"), change only this function.
    return QString::number(accountId);
}

QString QrCodeHelper::accountQrDataUrl(int accountId) const
{
    if (accountId <= 0)
        return QString(); // no valid account yet; caller should show a placeholder

    const QString payload = payloadForAccountId(accountId);
    const std::string text = payload.toStdString();

    QrCode qr = QrCode::encodeText(text.c_str(), QrCode::Ecc::MEDIUM);
    const int size = qr.getSize();
    const int border = 4; // quiet zone, per QR spec recommendation
    const int fullSize = size + border * 2;

    // Build a crisp SVG: one <rect> per dark module, on a white background.
    // SVG scales cleanly to any display size, so this looks sharp at any
    // Image element dimensions in QML without needing a fixed pixel size.
    std::ostringstream svg;
    svg << "<svg xmlns=\"http://www.w3.org/2000/svg\" viewBox=\"0 0 "
        << fullSize << " " << fullSize << "\" shape-rendering=\"crispEdges\">";
    svg << "<rect width=\"" << fullSize << "\" height=\"" << fullSize << "\" fill=\"#ffffff\"/>";

    for (int y = 0; y < size; y++)
    {
        for (int x = 0; x < size; x++)
        {
            if (qr.getModule(x, y))
            {
                svg << "<rect x=\"" << (x + border) << "\" y=\"" << (y + border)
                    << "\" width=\"1\" height=\"1\" fill=\"#000000\"/>";
            }
        }
    }

    svg << "</svg>";

    // QUrl::toPercentEncoding handles the characters that break a data URL
    // (", #, %, etc.) so the SVG can be embedded directly rather than
    // base64-encoded — keeps this readable if you ever need to debug it.
    const QByteArray encoded = QUrl::toPercentEncoding(QString::fromStdString(svg.str()));
    return QStringLiteral("data:image/svg+xml;utf8,") + QString::fromUtf8(encoded);
}
