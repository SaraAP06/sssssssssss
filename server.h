#ifndef SERVER_H
#define SERVER_H

#include <QTcpServer>
#include <QTcpSocket>
#include <QMap>
#include <QJsonObject>
#include "userrepository.h"
#include "restaurantrepository.h"
#include "menurepository.h"
#include "database.h"

class Server : public QTcpServer {
    Q_OBJECT
public:
    explicit Server(QObject *parent = nullptr);

    // شروع به کار سرور روی پورت مشخص
    bool startServer(quint16 port);

protected:
    // مدیریت اتصالات ورودی
    void incomingConnection(qintptr socketDescriptor) override;

private slots:
    // خواندن داده‌های ارسالی از کلاینت
    void readClientData();

    // مدیریت قطع ارتباط کلاینت
    void clientDisconnected();

private:
    // توابع مدیریت درخواست‌های مختلف
    void handleRegister(const QJsonObject& json, QTcpSocket* client);
    void handleLogin(const QJsonObject& json, QTcpSocket* client);
    void handleAddRestaurant(const QJsonObject& json, QTcpSocket* client); // جدید: اضافه کردن رستوران
    void handleGetRestaurants(QTcpSocket* client);
    void handleGetMenu(const QJsonObject& json, QTcpSocket* client);
    void handleOrder(const QJsonObject& json, QTcpSocket* client);
    void handleGetMyOrders(QTcpSocket* client);
    void handleChangeOrderStatus(const QJsonObject& json, QTcpSocket* client);
    void handleGetOrdersForRestaurant(QTcpSocket* client);
    void handleAddMenuItem(const QJsonObject& json, QTcpSocket* client);
    void handleEditMenuItem(const QJsonObject& json, QTcpSocket* client);
    void handleDeleteMenuItem(const QJsonObject& json, QTcpSocket* client);
    void handleRateOrder(const QJsonObject& json, QTcpSocket* client);
    void handleGetRatings(const QJsonObject& json, QTcpSocket* client);
    void handleApproveRestaurant(const QJsonObject& json, QTcpSocket* client);
    void handleBlockUser(const QJsonObject& json, QTcpSocket* client);
    void notifyRestaurant(int restaurantId, const QString& message);
    void notifyClient(int userId, const QString& message);
    void logMessage(const QString& message);

    QMap<qintptr, QTcpSocket*> clients; // ارتباط دادن شناسه سوکت به آبجکت سوکت کلاینت
    QMap<qintptr, int> clientUserIds; // ارتباط دادن شناسه سوکت به شناسه کاربر
    UserRepository userRepo; // برای دسترسی به داده‌های کاربران
    RestaurantRepository restaurantRepo; // جدید: برای دسترسی به داده‌های رستوران‌ها
    MenuRepository menuRepo; // جدید: برای دسترسی به داده‌های منوها
};

#endif // SERVER_H
