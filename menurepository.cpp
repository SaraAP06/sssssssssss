#include "menurepository.h"
#include "database.h"
#include <QSqlQuery>
#include <QSqlError>
#include <QJsonObject>
#include <QDebug>

// اضافه کردن آیتم جدید به منو
bool MenuRepository::addMenuItem(int restaurantId, const QString& name, double price) {
    QSqlQuery query(Database::instance().getConnection());

    query.prepare("INSERT INTO menu_items (restaurant_id, name, price) VALUES (:rid, :name, :price)");
    query.bindValue(":rid", restaurantId);
    query.bindValue(":name", name);
    query.bindValue(":price", price);

    if (!query.exec()) {
        qWarning() << "Failed to add menu item:" << query.lastError().text();
        return false;
    }
    return true;
}

// ویرایش آیتم منو
bool MenuRepository::editMenuItem(int itemId, int restaurantId, const QString& name, double price) {
    QSqlQuery query(Database::instance().getConnection());

    query.prepare("UPDATE menu_items SET name = :name, price = :price WHERE id = :item_id AND restaurant_id = :rid");
    query.bindValue(":name", name);
    query.bindValue(":price", price);
    query.bindValue(":item_id", itemId);
    query.bindValue(":rid", restaurantId);

    if (!query.exec()) {
        qWarning() << "Failed to edit menu item:" << query.lastError().text();
        return false;
    }
    return true;
}

// حذف آیتم منو
bool MenuRepository::deleteMenuItem(int itemId, int restaurantId) {
    QSqlQuery query(Database::instance().getConnection());

    query.prepare("DELETE FROM menu_items WHERE id = :item_id AND restaurant_id = :rid");
    query.bindValue(":item_id", itemId);
    query.bindValue(":rid", restaurantId);

    if (!query.exec()) {
        qWarning() << "Failed to delete menu item:" << query.lastError().text();
        return false;
    }
    return true;
}

// گرفتن منوی رستوران
QJsonArray MenuRepository::getMenu(int restaurantId) {
    QSqlQuery query(Database::instance().getConnection());
    QJsonArray menuArray;

    query.prepare("SELECT id, name, price FROM menu_items WHERE restaurant_id = :rid");
    query.bindValue(":rid", restaurantId);

    if (query.exec()) {
        while (query.next()) {
            QJsonObject item;
            item["item_id"] = query.value("id").toInt();
            item["name"] = query.value("name").toString();
            item["price"] = query.value("price").toDouble();
            menuArray.append(item);
        }
    } else {
        qWarning() << "Failed to retrieve menu:" << query.lastError().text();
    }
    return menuArray;
}
