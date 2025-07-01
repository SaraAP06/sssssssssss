#include "restaurantrepository.h"
#include "database.h"
#include <QSqlQuery>
#include <QSqlError>
#include <QJsonObject>
#include <QDebug>

// اضافه کردن رستوران جدید
bool RestaurantRepository::addRestaurant(int ownerId, const QString& name) {
    QSqlQuery query(Database::instance().getConnection());

    query.prepare("INSERT INTO restaurants (name, owner_id, status) VALUES (:name, :owner_id, 'pending')");
    query.bindValue(":name", name);
    query.bindValue(":owner_id", ownerId);

    if (!query.exec()) {
        qWarning() << "Failed to add restaurant:" << query.lastError().text();
        return false;
    }
    return true;
}

// گرفتن لیست رستوران‌های تأییدشده
QJsonArray RestaurantRepository::getApprovedRestaurants() {
    QSqlQuery query(Database::instance().getConnection());
    QJsonArray restaurantArray;

    if (query.exec("SELECT id, name FROM restaurants WHERE status = 'approved'")) {
        while (query.next()) {
            QJsonObject rest;
            rest["id"] = query.value("id").toInt();
            rest["name"] = query.value("name").toString();
            restaurantArray.append(rest);
        }
    } else {
        qWarning() << "Failed to retrieve restaurants:" << query.lastError().text();
    }
    return restaurantArray;
}

// تغییر وضعیت رستوران (توسط ادمین)
bool RestaurantRepository::updateRestaurantStatus(int restaurantId, const QString& status) {
    QSqlQuery query(Database::instance().getConnection());

    query.prepare("UPDATE restaurants SET status = :status WHERE id = :rid");
    query.bindValue(":status", status);
    query.bindValue(":rid", restaurantId);

    if (!query.exec()) {
        qWarning() << "Failed to update restaurant status:" << query.lastError().text();
        return false;
    }
    return true;
}

// بررسی مالکیت رستوران
bool RestaurantRepository::isRestaurantOwner(int userId, int restaurantId) {
    QSqlQuery query(Database::instance().getConnection());
    query.prepare("SELECT owner_id FROM restaurants WHERE id = :rid");
    query.bindValue(":rid", restaurantId);

    if (query.exec() && query.next()) {
        return query.value("owner_id").toInt() == userId;
    }
    return false;
}

// گرفتن شناسه صاحب رستوران
int RestaurantRepository::getRestaurantOwnerId(int restaurantId) {
    QSqlQuery query(Database::instance().getConnection());
    query.prepare("SELECT owner_id FROM restaurants WHERE id = :rid");
    query.bindValue(":rid", restaurantId);

    if (query.exec() && query.next()) {
        return query.value("owner_id").toInt();
    }
    return -1;
}
