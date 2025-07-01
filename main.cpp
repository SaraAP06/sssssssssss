#include <QCoreApplication>
#include "server.h"
#include "database.h"
#include <QFile>
#include <QTextStream>
#include <QDateTime>

// ثبت لاگ
void logMessage(const QString& message) {
    QFile logFile("server.log");
    if (logFile.open(QIODevice::Append | QIODevice::Text)) {
        QTextStream out(&logFile);
        out << QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss") << ": " << message << "\n";
        logFile.close();
    }
}

int main(int argc, char *argv[]) {
    QCoreApplication a(argc, argv);

    // ثبت لاگ شروع برنامه
    logMessage("برنامه سرور شروع شد");

    // راه‌اندازی و تنظیم جداول دیتابیس
    Database::instance().setupTables();

    // راه‌اندازی سرور
    Server server;
    if (!server.startServer(12345)) {
        qCritical() << "Failed to start server";
        logMessage("خطا در راه‌اندازی سرور");
        return 1;
    }

    return a.exec();
}
