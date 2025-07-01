#ifndef USERREPOSITORY_H
#define USERREPOSITORY_H

#include <QString>

class UserRepository {
public:
    // ثبت کاربر جدید در سیستم
    bool addUser(const QString& username, const QString& password, const QString& role = "customer");

    // بررسی وجود کاربر با نام کاربری داده شده
    bool userExists(const QString& username);

    // اعتبار سنجی ورود کاربر و دریافت نقش و شناسه کاربر
    bool validateLogin(const QString& username, const QString& password, int& userId, QString& role);
};

#endif // USERREPOSITORY_H
