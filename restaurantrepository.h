#ifndef RESTAURANTREPOSITORY_H
#define RESTAURANTREPOSITORY_H

#include <QString>
#include <QJsonArray>

// کلاسی برای مدیریت عملیات مربوط به رستوران‌ها
class RestaurantRepository {
public:
    // اضافه کردن رستوران جدید
    bool addRestaurant(int ownerId, const QString& name);

    // گرفتن لیست رستوران‌های تأییدشده
    QJsonArray getApprovedRestaurants();

    // تغییر وضعیت رستوران (توسط ادمین)
    bool updateRestaurantStatus(int restaurantId, const QString& status);

    // بررسی مالکیت رستوران
    bool isRestaurantOwner(int userId, int restaurantId);

    // گرفتن شناسه صاحب رستوران
    int getRestaurantOwnerId(int restaurantId);
};

#endif // RESTAURANTREPOSITORY_H
