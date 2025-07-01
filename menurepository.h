#ifndef MENUREPOSITORY_H
#define MENUREPOSITORY_H

#include <QString>
#include <QJsonArray>

// کلاسی برای مدیریت عملیات مربوط به منوها
class MenuRepository {
public:
    // اضافه کردن آیتم جدید به منو
    bool addMenuItem(int restaurantId, const QString& name, double price);

    // ویرایش آیتم منو
    bool editMenuItem(int itemId, int restaurantId, const QString& name, double price);

    // حذف آیتم منو
    bool deleteMenuItem(int itemId, int restaurantId);

    // گرفتن منوی رستوران
    QJsonArray getMenu(int restaurantId);
};

#endif // MENUREPOSITORY_H
