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
Server::Server(QObject *parent) : QTcpServer(parent) {
    // اتصال سیگنال‌های مربوط به کلاینت‌ها
}

// شروع به کار سرور روی پورت مشخص
bool Server::startServer(quint16 port) {
    if (!listen(QHostAddress::Any, port)) {
        qWarning() << "نمی‌توان سرور را روی پورت" << port << "راه‌اندازی کرد:" << errorString();
        return false;
    }
    qDebug() << "سرور روی پورت" << port << "شروع به کار کرد";
    logMessage("سرور روی پورت " + QString::number(port) + " شروع به کار کرد");
    return true;
}

// مدیریت اتصالات ورودی
void Server::incomingConnection(qintptr socketDescriptor) {
    QTcpSocket *client = new QTcpSocket(this);
    if (!client->setSocketDescriptor(socketDescriptor)) {
        qWarning() << "خطا در تنظیم سوکت کلاینت:" << client->errorString();
        return;
    }
    clients.insert(socketDescriptor, client);

    connect(client, &QTcpSocket::readyRead, this, &Server::readClientData);
    connect(client, &QTcpSocket::disconnected, this, &Server::clientDisconnected);

    logMessage("کلاینت جدید متصل شد: " + QString::number(socketDescriptor));
}

// خواندن داده‌های ارسالی از کلاینت
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

// مدیریت قطع ارتباط کلاینت
void Server::clientDisconnected() {
    QTcpSocket *client = qobject_cast<QTcpSocket*>(sender());
    if (!client) return;

    qintptr socketDescriptor = client->socketDescriptor();
    clients.remove(socketDescriptor);
    clientUserIds.remove(socketDescriptor);
    logMessage("کلاینت قطع شد: " + QString::number(socketDescriptor));
    client->deleteLater();
}

// ثبت‌نام کاربر
void Server::handleRegister(const QJsonObject& json, QTcpSocket* client) {
    QString username = json["username"].toString();
    QString password = json["password"].toString();
    QString role = json["role"].toString("customer");

    QJsonObject response;
    if (username.isEmpty() || password.isEmpty()) {
        response["status"] = "error";
        response["message"] = "نام کاربری یا رمز عبور نمی‌تواند خالی باشد";
        client->write(QJsonDocument(response).toJson());
        logMessage("تلاش برای ثبت‌نام با داده‌های نامعتبر از کلاینت: " + QString::number(client->socketDescriptor()));
        return;
    }

    if (userRepo.userExists(username)) {
        response["status"] = "error";
        response["message"] = "نام کاربری قبلاً ثبت شده";
        client->write(QJsonDocument(response).toJson());
        logMessage("تلاش برای ثبت‌نام با نام کاربری تکراری: " + username);
        return;
    }

    if (userRepo.addUser(username, password, role)) {
        response["status"] = "success";
        response["message"] = "ثبت‌نام با موفقیت انجام شد";
    } else {
        response["status"] = "error";
        response["message"] = "خطا در ثبت‌نام";
    }

    client->write(QJsonDocument(response).toJson());
    logMessage("درخواست ثبت‌نام: " + username + " با نقش: " + role);
}

// ورود کاربر
void Server::handleLogin(const QJsonObject& json, QTcpSocket* client) {
    QString username = json["username"].toString();
    QString password = json["password"].toString();

    QJsonObject response;
    if (username.isEmpty() || password.isEmpty()) {
        response["status"] = "error";
        response["message"] = "نام کاربری یا رمز عبور نمی‌تواند خالی باشد";
        client->write(QJsonDocument(response).toJson());
        logMessage("تلاش برای ورود با داده‌های نامعتبر از کلاینت: " + QString::number(client->socketDescriptor()));
        return;
    }

    int userId;
    QString role;
    if (userRepo.validateLogin(username, password, userId, role)) {
        clientUserIds[client->socketDescriptor()] = userId;
        response["status"] = "success";
        response["message"] = "ورود موفق";
        response["user_id"] = userId;
        response["role"] = role;
    } else {
        response["status"] = "error";
        response["message"] = "نام کاربری یا رمز عبور اشتباه است یا حساب بلاک شده است";
    }

    client->write(QJsonDocument(response).toJson());
    logMessage("درخواست ورود: " + username);
}

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

// ثبت سفارش
void Server::handleOrder(const QJsonObject& json, QTcpSocket* client) {
    int customerId = clientUserIds.value(client->socketDescriptor(), -1);
    int restaurantId = json["restaurant_id"].toInt();
    QJsonArray items = json["items"].toArray();

    QJsonObject response;
    if (customerId == -1 || items.isEmpty()) {
        response["status"] = "error";
        response["message"] = "داده‌های نامعتبر";
        client->write(QJsonDocument(response).toJson());
        logMessage("تلاش برای ثبت سفارش با داده‌های نامعتبر - کلاینت: " + QString::number(client->socketDescriptor()));
        return;
    }

    // بررسی وجود رستوران
    QSqlQuery restQuery(Database::instance().getConnection());
    restQuery.prepare("SELECT id FROM restaurants WHERE id = :rid AND status = 'approved'");
    restQuery.bindValue(":rid", restaurantId);
    if (!restQuery.exec() || !restQuery.next()) {
        response["status"] = "error";
        response["message"] = "رستوران یافت نشد یا تأیید نشده";
        client->write(QJsonDocument(response).toJson());
        logMessage("رستوران یافت نشد: " + QString::number(restaurantId));
        return;
    }

    QSqlQuery orderQuery(Database::instance().getConnection());
    orderQuery.prepare("INSERT INTO orders (user_id, restaurant_id, status, created_at) VALUES (:uid, :rid, 'pending', CURRENT_TIMESTAMP)");
    orderQuery.bindValue(":uid", customerId);
    orderQuery.bindValue(":rid", restaurantId);
    if (!orderQuery.exec()) {
        response["status"] = "error";
        response["message"] = "خطا در ثبت سفارش";
        client->write(QJsonDocument(response).toJson());
        logMessage("خطا در ثبت سفارش برای کاربر: " + QString::number(customerId));
        return;
    }

    int orderId = orderQuery.lastInsertId().toInt();
    bool itemsValid = true;
    for (const QJsonValue& item : items) {
        QJsonObject itemObj = item.toObject();
        int itemId = itemObj["item_id"].toInt();
        int quantity = itemObj["quantity"].toInt();

        QSqlQuery itemQuery(Database::instance().getConnection());
        itemQuery.prepare("SELECT id FROM menu_items WHERE id = :item_id AND restaurant_id = :rid");
        itemQuery.bindValue(":item_id", itemId);
        itemQuery.bindValue(":rid", restaurantId);
        if (!itemQuery.exec() || !itemQuery.next() || quantity <= 0) {
            itemsValid = false;
            break;
        }

        QSqlQuery orderItemQuery(Database::instance().getConnection());
        orderItemQuery.prepare("INSERT INTO order_items (order_id, item_id, quantity) VALUES (:oid, :item_id, :quantity)");
        orderItemQuery.bindValue(":oid", orderId);
        orderItemQuery.bindValue(":item_id", itemId);
        orderItemQuery.bindValue(":quantity", quantity);
        if (!orderItemQuery.exec()) {
            itemsValid = false;
            break;
        }
    }

    if (itemsValid) {
        response["status"] = "success";
        response["message"] = "سفارش ثبت شد";
        response["order_id"] = orderId;
        QString notification = QString("{\"type\":\"new_order\",\"order_id\":%1}").arg(orderId);
        notifyRestaurant(restaurantId, notification);
    } else {
        // حذف سفارش در صورت خطا
        QSqlQuery deleteQuery(Database::instance().getConnection());
        deleteQuery.prepare("DELETE FROM orders WHERE order_id = :oid");
        deleteQuery.bindValue(":oid", orderId);
        deleteQuery.exec();
        response["status"] = "error";
        response["message"] = "خطا در ثبت آیتم‌های سفارش";
    }

    client->write(QJsonDocument(response).toJson());
    logMessage("درخواست ثبت سفارش: ID=" + QString::number(orderId) + " توسط کاربر: " + QString::number(customerId));
}

// دریافت سفارش‌های کاربر
void Server::handleGetMyOrders(QTcpSocket* client) {
    int userId = clientUserIds.value(client->socketDescriptor(), -1);
    QJsonObject response;

    if (userId == -1) {
        response["status"] = "error";
        response["message"] = "وارد سیستم نشده‌اید";
        client->write(QJsonDocument(response).toJson());
        logMessage("تلاش برای دریافت سفارش‌ها بدون ورود - کلاینت: " + QString::number(client->socketDescriptor()));
        return;
    }

    QSqlQuery query(Database::instance().getConnection());
    query.prepare("SELECT o.order_id, o.status, o.created_at, r.name AS restaurant_name "
                  "FROM orders o JOIN restaurants r ON o.restaurant_id = r.id "
                  "WHERE o.user_id = :uid");
    query.bindValue(":uid", userId);

    QJsonArray ordersArray;
    if (query.exec()) {
        while (query.next()) {
            QJsonObject order;
            order["order_id"] = query.value("order_id").toInt();
            order["restaurant_name"] = query.value("restaurant_name").toString();
            order["status"] = query.value("status").toString();
            order["date"] = query.value("created_at").toString();
            ordersArray.append(order);
        }
        response["status"] = "success";
        response["orders"] = ordersArray;
    } else {
        response["status"] = "error";
        response["message"] = "خطا در دریافت سفارش‌ها";
    }

    client->write(QJsonDocument(response).toJson());
    logMessage("درخواست دریافت سفارش‌های کاربر: " + QString::number(userId));
}

// تغییر وضعیت سفارش
void Server::handleChangeOrderStatus(const QJsonObject& json, QTcpSocket* client) {
    int userId = clientUserIds.value(client->socketDescriptor(), -1);
    int orderId = json["order_id"].toInt();
    QString newStatus = json["new_status"].toString();

    QJsonObject response;
    if (userId == -1 || orderId <= 0 || !newStatus.contains(QRegExp("^(pending|preparing|delivered)$"))) {
        response["status"] = "error";
        response["message"] = "داده‌های نامعتبر";
        client->write(QJsonDocument(response).toJson());
        logMessage("تلاش برای تغییر وضعیت سفارش با داده‌های نامعتبر - کلاینت: " + QString::number(client->socketDescriptor()));
        return;
    }

    QSqlQuery orderQuery(Database::instance().getConnection());
    orderQuery.prepare("SELECT restaurant_id, user_id FROM orders WHERE order_id = :oid");
    orderQuery.bindValue(":oid", orderId);
    if (!orderQuery.exec() || !orderQuery.next()) {
        response["status"] = "error";
        response["message"] = "سفارش یافت نشد";
        client->write(QJsonDocument(response).toJson());
        logMessage("سفارش یافت نشد: ID=" + QString::number(orderId));
        return;
    }

    int restaurantId = orderQuery.value("restaurant_id").toInt();
    int customerId = orderQuery.value("user_id").toInt();
    if (!restaurantRepo.isRestaurantOwner(userId, restaurantId)) {
        response["status"] = "error";
        response["message"] = "دسترسی غیرمجاز";
        client->write(QJsonDocument(response).toJson());
        logMessage("دسترسی غیرمجاز برای تغییر وضعیت سفارش توسط کاربر: " + QString::number(userId));
        return;
    }

    QSqlQuery updateQuery(Database::instance().getConnection());
    updateQuery.prepare("UPDATE orders SET status = :status WHERE order_id = :oid");
    updateQuery.bindValue(":status", newStatus);
    updateQuery.bindValue(":oid", orderId);
    if (updateQuery.exec()) {
        response["status"] = "success";
        response["message"] = "وضعیت سفارش به‌روزرسانی شد";
        QString notification = QString("{\"type\":\"order_status_update\",\"order_id\":%1,\"new_status\":\"%2\"}")
                                 .arg(orderId).arg(newStatus);
        notifyClient(customerId, notification);
    } else {
        response["status"] = "error";
        response["message"] = "خطا در به‌روزرسانی وضعیت";
    }

    client->write(QJsonDocument(response).toJson());
    logMessage("تغییر وضعیت سفارش: ID=" + QString::number(orderId) + " به " + newStatus);
}

// دریافت سفارش‌های رستوران
void Server::handleGetOrdersForRestaurant(QTcpSocket* client) {
    int userId = clientUserIds.value(client->socketDescriptor(), -1);
    QJsonObject response;

    if (userId == -1) {
        response["status"] = "error";
        response["message"] = "وارد سیستم نشده‌اید";
        client->write(QJsonDocument(response).toJson());
        logMessage("تلاش برای دریافت سفارش‌های رستوران بدون ورود - کلاینت: " + QString::number(client->socketDescriptor()));
        return;
    }

    QSqlQuery restQuery(Database::instance().getConnection());
    restQuery.prepare("SELECT id FROM restaurants WHERE owner_id = :oid AND status = 'approved'");
    restQuery.bindValue(":oid", userId);
    if (!restQuery.exec() || !restQuery.next()) {
        response["status"] = "error";
        response["message"] = "رستوران یافت نشد یا تأیید نشده";
        client->write(QJsonDocument(response).toJson());
        logMessage("رستوران یافت نشد برای کاربر: " + QString::number(userId));
        return;
    }

    int restaurantId = restQuery.value("id").toInt();
    QSqlQuery orderQuery(Database::instance().getConnection());
    orderQuery.prepare("SELECT o.order_id, o.user_id, o.status, oi.item_id, oi.quantity, mi.name "
                      "FROM orders o "
                      "JOIN order_items oi ON o.order_id = oi.order_id "
                      "JOIN menu_items mi ON oi.item_id = mi.id "
                      "WHERE o.restaurant_id = :rid");
    orderQuery.bindValue(":rid", restaurantId);

    QJsonArray ordersArray;
    QMap<int, QJsonObject> ordersMap;
    if (orderQuery.exec()) {
        while (orderQuery.next()) {
            int orderId = orderQuery.value("order_id").toInt();
            if (!ordersMap.contains(orderId)) {
                QJsonObject order;
                order["order_id"] = orderId;
                order["customer_id"] = orderQuery.value("user_id").toInt();
                order["status"] = orderQuery.value("status").toString();
                order["items"] = QJsonArray();
                ordersMap[orderId] = order;
            }
            QJsonObject item;
            item["item_id"] = orderQuery.value("item_id").toInt();
            item["name"] = orderQuery.value("name").toString();
            item["quantity"] = orderQuery.value("quantity").toInt();
            ordersMap[orderId]["items"].toArray().append(item);
        }

        for (const QJsonObject& order : ordersMap) {
            ordersArray.append(order);
        }
        response["status"] = "success";
        response["orders"] = ordersArray;
    } else {
        response["status"] = "error";
        response["message"] = "خطا در دریافت سفارش‌ها";
    }

    client->write(QJsonDocument(response).toJson());
    logMessage("درخواست دریافت سفارش‌های رستوران برای کاربر: " + QString::number(userId));
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

// امتیازدهی به سفارش
void Server::handleRateOrder(const QJsonObject& json, QTcpSocket* client) {
    int userId = clientUserIds.value(client->socketDescriptor(), -1);
    int orderId = json["order_id"].toInt();
    int rating = json["rating"].toInt();
    QString comment = json["comment"].toString();

    QJsonObject response;
    if (userId == -1 || orderId <= 0 || rating < 1 || rating > 5) {
        response["status"] = "error";
        response["message"] = "داده‌های نامعتبر";
        client->write(QJsonDocument(response).toJson());
        logMessage("تلاش برای امتیازدهی با داده‌های نامعتبر - کلاینت: " + QString::number(client->socketDescriptor()));
        return;
    }

    QSqlQuery orderQuery(Database::instance().getConnection());
    orderQuery.prepare("SELECT user_id, restaurant_id FROM orders WHERE order_id = :oid");
    orderQuery.bindValue(":oid", orderId);
    if (!orderQuery.exec() || !orderQuery.next() || orderQuery.value("user_id").toInt() != userId) {
        response["status"] = "error";
        response["message"] = "سفارش یافت نشد یا متعلق به شما نیست";
        client->write(QJsonDocument(response).toJson());
        logMessage("سفارش یافت نشد یا متعلق به کاربر نیست: ID=" + QString::number(orderId));
        return;
    }

    QSqlQuery rateQuery(Database::instance().getConnection());
    rateQuery.prepare("INSERT INTO ratings (order_id, user_id, rating, comment) "
                     "VALUES (:oid, :uid, :rating, :comment)");
    rateQuery.bindValue(":oid", orderId);
    rateQuery.bindValue(":uid", userId);
    rateQuery.bindValue(":rating", rating);
    rateQuery.bindValue(":comment", comment);
    if (rateQuery.exec()) {
        response["status"] = "success";
        response["message"] = "امتیاز ثبت شد";
    } else {
        response["status"] = "error";
        response["message"] = "خطا در ثبت امتیاز";
    }

    client->write(QJsonDocument(response).toJson());
    logMessage("امتیازدهی به سفارش: ID=" + QString::number(orderId) + " توسط کاربر: " + QString::number(userId));
}

// دریافت امتیازات رستوران
void Server::handleGetRatings(const QJsonObject& json, QTcpSocket* client) {
    int userId = clientUserIds.value(client->socketDescriptor(), -1);
    int restaurantId = json["restaurant_id"].toInt();

    QJsonObject response;
    if (userId == -1 || restaurantId <= 0) {
        response["status"] = "error";
        response["message"] = "داده‌های نامعتبر";
        client->write(QJsonDocument(response).toJson());
        logMessage("تلاش برای دریافت امتیازات با داده‌های نامعتبر - کلاینت: " + QString::number(client->socketDescriptor()));
        return;
    }

    if (!restaurantRepo.isRestaurantOwner(userId, restaurantId)) {
        response["status"] = "error";
        response["message"] = "دسترسی غیرمجاز";
        client->write(QJsonDocument(response).toJson());
        logMessage("دسترسی غیرمجاز برای دریافت امتیازات توسط کاربر: " + QString::number(userId));
        return;
    }

    QSqlQuery query(Database::instance().getConnection());
    query.prepare("SELECT r.order_id, r.rating, r.comment, u.username "
                  "FROM ratings r JOIN users u ON r.user_id = u.id "
                  "WHERE r.order_id IN (SELECT order_id FROM orders WHERE restaurant_id = :rid)");
    query.bindValue(":rid", restaurantId);

    QJsonArray ratingsArray;
    if (query.exec()) {
        while (query.next()) {
            QJsonObject rating;
            rating["order_id"] = query.value("order_id").toInt();
            rating["username"] = query.value("username").toString();
            rating["rating"] = query.value("rating").toInt();
            rating["comment"] = query.value("comment").toString();
            ratingsArray.append(rating);
        }
        response["status"] = "success";
        response["ratings"] = ratingsArray;
    } else {
        response["status"] = "error";
        response["message"] = "خطا در دریافت امتیازات";
    }

    client->write(QJsonDocument(response).toJson());
    logMessage("درخواست دریافت امتیازات رستوران: " + QString::number(restaurantId));
}

// تأیید یا رد رستوران
void Server::handleApproveRestaurant(const QJsonObject& json, QTcpSocket* client) {
    int userId = clientUserIds.value(client->socketDescriptor(), -1);
    int restaurantId = json["restaurant_id"].toInt();
    QString newStatus = json["status"].toString();

    QJsonObject response;
    if (userId == -1 || restaurantId <= 0 || !newStatus.contains(QRegExp("^(approved|rejected)$"))) {
        response["status"] = "error";
        response["message"] = "داده‌های نامعتبر";
        client->write(QJsonDocument(response).toJson());
        logMessage("تلاش برای تأیید رستوران با داده‌های نامعتبر - کلاینت: " + QString::number(client->socketDescriptor()));
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

// بلاک یا آنبلاک کردن کاربر
void Server::handleBlockUser(const QJsonObject& json, QTcpSocket* client) {
    int userId = clientUserIds.value(client->socketDescriptor(), -1);
    int targetUserId = json["user_id"].toInt();
    bool block = json["block"].toBool();

    QJsonObject response;
    if (userId == -1 || targetUserId <= 0) {
        response["status"] = "error";
        response["message"] = "داده‌های نامعتبر";
        client->write(QJsonDocument(response).toJson());
        logMessage("تلاش برای بلاک/آنبلاک با داده‌های نامعتبر - کلاینت: " + QString::number(client->socketDescriptor()));
        return;
    }

    QSqlQuery roleQuery(Database::instance().getConnection());
    roleQuery.prepare("SELECT role FROM users WHERE id = :uid");
    roleQuery.bindValue(":uid", userId);
    if (!roleQuery.exec() || !roleQuery.next() || roleQuery.value("role").toString() != "admin") {
        response["status"] = "error";
        response["message"] = "دسترسی غیرمجاز";
        client->write(QJsonDocument(response).toJson());
        logMessage("دسترسی غیرمجاز برای بلاک/آنبلاک توسط کاربر: " + QString::number(userId));
        return;
    }

    QSqlQuery blockQuery(Database::instance().getConnection());
    blockQuery.prepare("UPDATE users SET blocked = :blocked WHERE id = :uid");
    blockQuery.bindValue(":blocked", block ? 1 : 0);
    blockQuery.bindValue(":uid", targetUserId);
    if (blockQuery.exec()) {
        response["status"] = "success";
        response["message"] = block ? "کاربر بلاک شد" : "کاربر آنبلاک شد";
        QString notification = QString("{\"type\":\"account_status\",\"blocked\":%1}").arg(block ? "true" : "false");
        notifyClient(targetUserId, notification);
    } else {
        response["status"] = "error";
        response["message"] = "خطا در تغییر وضعیت کاربر";
    }

    client->write(QJsonDocument(response).toJson());
    logMessage("تغییر وضعیت کاربر: ID=" + QString::number(targetUserId) + " به " + (block ? "بلاک" : "آنبلاک"));
}

// اطلاع‌رسانی به رستوران
void Server::notifyRestaurant(int restaurantId, const QString& message) {
    int ownerId = restaurantRepo.getRestaurantOwnerId(restaurantId);
    if (ownerId == -1) return;

    for (auto it = clientUserIds.constBegin(); it != clientUserIds.constEnd(); ++it) {
        if (it.value() == ownerId) {
            QTcpSocket* client = clients.value(it.key());
            if (client && client->state() == QAbstractSocket::ConnectedState) {
                client->write(message.toUtf8());
                logMessage("ارسال نوتیفیکیشن به رستوران: " + QString::number(restaurantId));
            }
        }
    }
}

// اطلاع‌رسانی به کلاینت
void Server::notifyClient(int userId, const QString& message) {
    for (auto it = clientUserIds.constBegin(); it != clientUserIds.constEnd(); ++it) {
        if (it.value() == userId) {
            QTcpSocket* client = clients.value(it.key());
            if (client && client->state() == QAbstractSocket::ConnectedState) {
                client->write(message.toUtf8());
                logMessage("ارسال نوتیفیکیشن به کاربر: " + QString::number(userId));
            }
        }
    }
}

// ثبت لاگ
void Server::logMessage(const QString& message) {
    QFile file("server.log");
    if (file.open(QIODevice::Append | QIODevice::Text)) {
        QTextStream out(&file);
        out << QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss") << ": " << message << "\n";
        file.close();
    }
}
