#include "server.h"
#include <QSqlQuery>
#include <QSqlError>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QDebug>
#include <QFile>
#include <QTextStream>
#include <QDateTime>

// سازنده سرور
Server::Server(QObject *parent) : QTcpServer(parent) {}

// ... (بقیه توابع بدون تغییر می‌مونن)

// اضافه کردن رستوران جدید
void Server::handleAddRestaurant(const QJsonObject& json, QTcpSocket* client) {
    int ownerId = clientUserIds.value(client->socketDescriptor(), -1);
    QString name = json["name"].toString();

    QJsonObject response;
    if (ownerId == -1 || name.isEmpty()) {
        response["status"] = "error";
        response["message"] = "داده‌های نامعتبر";
        client->write(QJsonDocument(response).toJson());
        logMessage("تلاش برای افزودن رستوران با داده‌های نامعتبر - کلاینت: " + QString::number(client->socketDescriptor()));
        return;
    }

    // بررسی نقش کاربر
    QSqlQuery roleQuery(Database::instance().getConnection());
    roleQuery.prepare("SELECT role FROM users WHERE id = :uid");
    roleQuery.bindValue(":uid", ownerId);
    if (!roleQuery.exec() || !roleQuery.next() || roleQuery.value("role").toString() != "restaurant_owner") {
        response["status"] = "error";
        response["message"] = "فقط صاحب رستوران می‌تواند رستوران اضافه کند";
        client->write(QJsonDocument(response).toJson());
        logMessage("دسترسی غیرمجاز برای افزودن رستوران توسط کاربر: " + QString::number(ownerId));
        return;
    }

    if (restaurantRepo.addRestaurant(ownerId, name)) {
        response["status"] = "success";
        response["message"] = "رستوران با موفقیت ثبت شد و در انتظار تأیید است";
    } else {
        response["status"] = "error";
        response["message"] = "خطا در ثبت رستوران";
    }

    client->write(QJsonDocument(response).toJson());
    logMessage("درخواست افزودن رستوران: " + name + " توسط کاربر: " + QString::number(ownerId));
}

// دریافت لیست رستوران‌ها
void Server::handleGetRestaurants(QTcpSocket* client) {
    QJsonObject response;
    QJsonArray restaurantArray = restaurantRepo.getApprovedRestaurants();

    if (!restaurantArray.isEmpty()) {
        response["status"] = "success";
        response["restaurants"] = restaurantArray;
    } else {
        response["status"] = "error";
        response["message"] = "نمی‌توان رستوران‌ها را دریافت کرد";
    }

    client->write(QJsonDocument(response).toJson());
    logMessage("درخواست دریافت لیست رستوران‌ها از کلاینت: " + QString::number(client->socketDescriptor()));
}

// دریافت منوی رستوران
void Server::handleGetMenu(const QJsonObject& json, QTcpSocket* client) {
    int restaurantId = json["restaurant_id"].toInt();
    QJsonObject response;

    QJsonArray menuArray = menuRepo.getMenu(restaurantId);
    if (!menuArray.isEmpty()) {
        response["status"] = "success";
        response["menu"] = menuArray;
    } else {
        response["status"] = "error";
        response["message"] = "نمی‌توان منو را دریافت کرد یا رستوران یافت نشد";
    }

    client->write(QJsonDocument(response).toJson());
    logMessage("درخواست دریافت منوی رستوران " + QString::number(restaurantId) + " از کلاینت: " + QString::number(client->socketDescriptor()));
}

// افزودن آیتم به منو
void Server::handleAddMenuItem(const QJsonObject& json, QTcpSocket* client) {
    int ownerId = clientUserIds.value(client->socketDescriptor(), -1);
    QString name = json["name"].toString();
    double price = json["price"].toDouble();

    QJsonObject response;
    if (ownerId == -1 || name.isEmpty() || price <= 0) {
        response["status"] = "error";
        response["message"] = "داده‌های نامعتبر";
        client->write(QJsonDocument(response).toJson());
        logMessage("تلاش برای افزودن آیتم منو با داده‌های نامعتبر - کلاینت: " + QString::number(client->socketDescriptor()));
        return;
    }

    QSqlQuery restQuery(Database::instance().getConnection());
    restQuery.prepare("SELECT id FROM restaurants WHERE owner_id = :oid AND status = 'approved'");
    restQuery.bindValue(":oid", ownerId);
    if (!restQuery.exec() || !restQuery.next()) {
        response["status"] = "error";
        response["message"] = "رستوران یافت نشد یا تأیید نشده";
        client->write(QJsonDocument(response).toJson());
        logMessage("رستوران یافت نشد برای کاربر: " + QString::number(ownerId));
        return;
    }

    int restId = restQuery.value("id").toInt();
    if (menuRepo.addMenuItem(restId, name, price)) {
        response["status"] = "success";
        response["message"] = "آیتم به منو اضافه شد";
    } else {
        response["status"] = "error";
        response["message"] = "خطا در افزودن آیتم";
    }

    client->write(QJsonDocument(response).toJson());
    logMessage("آیتم منو اضافه شد: " + name + " برای رستوران: " + QString::number(restId));
}

// ویرایش آیتم منو
void Server::handleEditMenuItem(const QJsonObject& json, QTcpSocket* client) {
    int ownerId = clientUserIds.value(client->socketDescriptor(), -1);
    int itemId = json["item_id"].toInt();
    QString name = json["name"].toString();
    double price = json["price"].toDouble();

    QJsonObject response;
    if (ownerId == -1 || name.isEmpty() || price <= 0 || itemId <= 0) {
        response["status"] = "error";
        response["message"] = "داده‌های نامعتبر";
        client->write(QJsonDocument(response).toJson());
        logMessage("تلاش برای ویرایش آیتم منو با داده‌های نامعتبر - کلاینت: " + QString::number(client->socketDescriptor()));
        return;
    }

    QSqlQuery restQuery(Database::instance().getConnection());
    restQuery.prepare("SELECT id FROM restaurants WHERE owner_id = :oid AND status = 'approved'");
    restQuery.bindValue(":oid", ownerId);
    if (!restQuery.exec() || !restQuery.next()) {
        response["status"] = "error";
        response["message"] = "رستوران یافت نشد یا تأیید نشده";
        client->write(QJsonDocument(response).toJson());
        logMessage("رستوران یافت نشد برای کاربر: " + QString::number(ownerId));
        return;
    }

    int restId = restQuery.value("id").toInt();
    if (menuRepo.editMenuItem(itemId, restId, name, price)) {
        response["status"] = "success";
        response["message"] = "آیتم منو به‌روزرسانی شد";
    } else {
        response["status"] = "error";
        response["message"] = "خطا در به‌روزرسانی آیتم";
    }

    client->write(QJsonDocument(response).toJson());
    logMessage("آیتم منو به‌روزرسانی شد: ID=" + QString::number(itemId));
}

// حذف آیتم منو
void Server::handleDeleteMenuItem(const QJsonObject& json, QTcpSocket* client) {
    int ownerId = clientUserIds.value(client->socketDescriptor(), -1);
    int itemId = json["item_id"].toInt();

    QJsonObject response;
    if (ownerId == -1 || itemId <= 0) {
        response["status"] = "error";
        response["message"] = "داده‌های نامعتبر";
        client->write(QJsonDocument(response).toJson());
        logMessage("تلاش برای حذف آیتم منو با داده‌های نامعتبر - کلاینت: " + QString::number(client->socketDescriptor()));
        return;
    }

    QSqlQuery restQuery(Database::instance().getConnection());
    restQuery.prepare("SELECT id FROM restaurants WHERE owner_id = :oid AND status = 'approved'");
    restQuery.bindValue(":oid", ownerId);
    if (!restQuery.exec() || !restQuery.next()) {
        response["status"] = "error";
        response["message"] = "رستوران یافت نشد یا تأیید نشده";
        client->write(QJsonDocument(response).toJson());
        logMessage("رستوران یافت نشد برای کاربر: " + QString::number(ownerId));
        return;
    }

    int restId = restQuery.value("id").toInt();
    if (menuRepo.deleteMenuItem(itemId, restId)) {
        response["status"] = "success";
        response["message"] = "آیتم منو حذف شد";
    } else {
        response["status"] = "error";
        response["message"] = "خطا در حذف آیتم";
    }

    client->write(QJsonDocument(response).toJson());
    logMessage("آیتم منو حذف شد: ID=" + QString::number(itemId));
}

// تأیید یا رد رستوران
void Server::handleApproveRestaurant(const QJsonObject& json, QTcpSocket* client) {
    int userId = clientUserIds.value(client->socketDescriptor(), -1);
    int restaurantId = json["restaurant_id"].toInt();
    QString newStatus = json["status"].toString();

    QJsonObject response;
    if (userId == -1) {
        response["status"] = "error";
        response["message"] = "وارد سیستم نشده‌اید";
        client->write(QJsonDocument(response).toJson());
        logMessage("تلاش برای تأیید رستوران بدون ورود - کلاینت: " + QString::number(client->socketDescriptor()));
        return;
    }

    QSqlQuery roleQuery(Database::instance().getConnection());
    roleQuery.prepare("SELECT role FROM users WHERE id = :uid");
    roleQuery.bindValue(":uid", userId);
    if (!roleQuery.exec() || !roleQuery.next() || roleQuery.value("role").toString() != "admin") {
        response["status"] = "error";
        response["message"] = "دسترسی غیرمجاز";
        client->write(QJsonDocument(response).toJson());
        logMessage("دسترسی غیرمجاز برای تأیید رستوران توسط کاربر: " + QString::number(userId));
        return;
    }

    if (restaurantRepo.updateRestaurantStatus(restaurantId, newStatus)) {
        response["status"] = "success";
        response["message"] = "وضعیت رستوران به‌روزرسانی شد";
        QString notification = QString("{\"type\":\"restaurant_status\",\"status\":\"%1\"}").arg(newStatus);
        notifyRestaurant(restaurantId, notification);
        logMessage("وضعیت رستوران " + QString::number(restaurantId) + " به " + newStatus + " تغییر کرد");
    } else {
        response["status"] = "error";
        response["message"] = "خطا در به‌روزرسانی وضعیت رستوران";
    }

    client->write(QJsonDocument(response).toJson());
}


void Server::readClientData() {
    QTcpSocket *client = qobject_cast<QTcpSocket*>(sender());
    if (!client) return;

    QByteArray data = client->readAll();
    QJsonDocument doc = QJsonDocument::fromJson(data);
    if (doc.isNull()) {
        QJsonObject response;
        response["status"] = "error";
        response["message"] = "JSON نامعتبر";
        client->write(QJsonDocument(response).toJson());
        logMessage("دریافت JSON نامعتبر از کلاینت: " + QString::number(client->socketDescriptor()));
        return;
    }

    QJsonObject json = doc.object();
    QString type = json["type"].toString();

    // هدایت درخواست به تابع مناسب بر اساس نوع
    if (type == "register") {
        handleRegister(json, client);
    } else if (type == "login") {
        handleLogin(json, client);
    } else if (type == "add_restaurant") {
        handleAddRestaurant(json, client);
    } else if (type == "get_restaurants") {
        handleGetRestaurants(client);
    } else if (type == "get_menu") {
        handleGetMenu(json, client);
    } else if (type == "order") {
        handleOrder(json, client);
    } else if (type == "get_my_orders") {
        handleGetMyOrders(client);
    } else if (type == "change_order_status") {
        handleChangeOrderStatus(json, client);
    } else if (type == "get_orders_for_restaurant") {
        handleGetOrdersForRestaurant(client);
    } else if (type == "add_menu_item") {
        handleAddMenuItem(json, client);
    } else if (type == "edit_menu_item") {
        handleEditMenuItem(json, client);
    } else if (type == "delete_menu_item") {
        handleDeleteMenuItem(json, client);
    } else if (type == "rate_order") {
        handleRateOrder(json, client);
    } else if (type == "get_ratings") {
        handleGetRatings(json, client);
    } else if (type == "approve_restaurant") {
        handleApproveRestaurant(json, client);
    } else if (type == "block_user") {
        handleBlockUser(json, client);
    } else {
        QJsonObject response;
        response["status"] = "error";
        response["message"] = "نوع درخواست ناشناخته";
        client->write(QJsonDocument(response).toJson());
        logMessage("دریافت درخواست ناشناخته از کلاینت: " + QString::number(client->socketDescriptor()));
    }
}
