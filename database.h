#ifndef DATABASE_H
#define DATABASE_H

#include <QSqlDatabase>
#include <QString>

class Database {
public:
    // گرفتن تنها نمونه از این کلاس (Singleton)
    static Database& instance();

    // گرفتن اتصال دیتابیس
    QSqlDatabase& getConnection();

    // ساخت جداول اولیه users, restaurants, orders
    void setupTables();

    // دفع‌کننده برای بستن اتصال دیتابیس
    ~Database();

private:
    Database(); // سازنده خصوصی برای پیاده‌سازی Singleton
    QSqlDatabase db; // اتصال به دیتابیس SQLite
};

#endif // DATABASE_H
