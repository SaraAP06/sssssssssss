#include <QCoreApplication>
#include "server.h"
#include "database.h"

int main(int argc, char *argv[]) {
    QCoreApplication a(argc, argv);

    // راه‌اندازی و تنظیم جداول دیتابیس
    Database::instance().setupTables();

    // راه‌اندازی سرور
    Server server;
    if (!server.startServer(12345)) {
        qCritical() << "Failed to start server";
        server.logMessage("Failed to start server");
        return 1;
    }

    server.logMessage("Server application started");
    return a.exec();
}
