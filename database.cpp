#include "database.h"
#include <QSqlQuery>
#include <QSqlError>
#include <QDebug>

// پیاده سازی Singleton
Database& Database::instance() {
    static Database instance;
    return instance;
}

// سازنده کلاس Database
Database::Database() {
    // استفاده از درایو SQLite
    db = QSqlDatabase::addDatabase("QSQLITE");
    db.setDatabaseName("iut_food.db");

    // تلاش برای اتصال به دیتابیس
    if (!db.open()) {
        qCritical() << "Cannot open database:" << db.lastError().text();
        exit(1); // خاتمه برنامه
    } else {
        qDebug() << "Database connected.";
    }
}

// دفع‌کننده برای بستن اتصال دیتابیس
Database::~Database() {
    if (db.isOpen()) {
        db.close();
        qDebug() << "اتصال دیتابیس بسته شد.";
    }
}

// گرفتن اتصال دیتابیس
QSqlDatabase& Database::getConnection() {
    return db;
}

// ساخت جداول اولیه users, restaurant, orders
void Database::setupTables() {
    QSqlQuery query(db);
    db.transaction(); // شروع تراکنش برای اطمینان از سازگاری

    // ایجاد جدول کاربران
    QString createUserTable = R"(
        CREATE TABLE IF NOT EXISTS users (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            username TEXT UNIQUE NOT NULL,
            password TEXT NOT NULL,
            role TEXT NOT NULL CHECK(role IN ('customer', 'restaurant_owner', 'admin')),
            blocked INTEGER DEFAULT 0
        )
    )";
    if (!query.exec(createUserTable)) {
        qCritical() << "Failed to create users table:" << query.lastError().text();
        db.rollback();
        return;
    }

    // ایجاد جدول رستوران‌ها
    QString createRestaurantTable = R"(
        CREATE TABLE IF NOT EXISTS restaurants (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            name TEXT UNIQUE NOT NULL,
            owner_id INTEGER NOT NULL,
            status TEXT NOT NULL CHECK(status IN ('pending', 'approved', 'rejected')),
            FOREIGN KEY (owner_id) REFERENCES users(id)
        )
    )";
    if (!query.exec(createRestaurantTable)) {
        qCritical() << "Failed to create restaurants table:" << query.lastError().text();
        db.rollback();
        return;
    }

    // ایجاد جدول سفارش‌ها
    QString createOrderTable = R"(
        CREATE TABLE IF NOT EXISTS orders (
            order_id INTEGER PRIMARY KEY AUTOINCREMENT,
            user_id INTEGER NOT NULL,
            restaurant_id INTEGER NOT NULL,
            status TEXT NOT NULL CHECK(status IN ('pending', 'preparing', 'delivered')),
            created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
            FOREIGN KEY (user_id) REFERENCES users(id),
            FOREIGN KEY (restaurant_id) REFERENCES restaurants(id)
        )
    )";
    if (!query.exec(createOrderTable)) {
        qCritical() << "Failed to create orders table:" << query.lastError().text();
        db.rollback();
        return;
    }

    // ایجاد جدول آیتم‌های سفارش
    QString createOrderItemsTable = R"(
        CREATE TABLE IF NOT EXISTS order_items (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            order_id INTEGER NOT NULL,
            item_id INTEGER NOT NULL,
            quantity INTEGER NOT NULL,
            FOREIGN KEY (order_id) REFERENCES orders(order_id),
            FOREIGN KEY (item_id) REFERENCES menu_items(id)
        )
    )";
    if (!query.exec(createOrderItemsTable)) {
        qCritical() << "Failed to create order_items table:" << query.lastError().text();
        db.rollback();
        return;
    }

    // ایجاد جدول منو
    QString createMenuItemsTable = R"(
        CREATE TABLE IF NOT EXISTS menu_items (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            restaurant_id INTEGER NOT NULL,
            name TEXT NOT NULL,
            price REAL NOT NULL,
            FOREIGN KEY (restaurant_id) REFERENCES restaurants(id)
        )
    )";
    if (!query.exec(createMenuItemsTable)) {
        qCritical() << "ایجاد جدول آیتم‌های منو ناموفق بود:" << query.lastError().text();
        db.rollback();
        return;
    }

    // ایجاد جدول امتیازات
    QString createRatingsTable = R"(
        CREATE TABLE IF NOT EXISTS ratings (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            order_id INTEGER NOT NULL,
            user_id INTEGER NOT NULL,
            rating INTEGER NOT NULL CHECK(rating BETWEEN 1 AND 5),
            comment TEXT,
            FOREIGN KEY (order_id) REFERENCES orders(order_id),
            FOREIGN KEY (user_id) REFERENCES users(id)
        )
    )";
    if (!query.exec(createRatingsTable)) {
        qCritical() << "ایجاد جدول امتیازات ناموفق بود:" << query.lastError().text();
        db.rollback();
        return;
    }

    db.commit(); // تثبیت تراکنش
    qDebug() << "All tables are set up correctly.";
}
