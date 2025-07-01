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
        qCritical() << "خطا در راه‌اندازی سرور";
        server.logMessage("خطا در راه‌اندازی سرور");
        return 1;
    }

    server.logMessage("برنامه سرور شروع شد");
    return a.exec();
}
