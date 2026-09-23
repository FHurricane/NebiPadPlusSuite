#pragma once

#include <QDialog>
#include <QStringList>

#include <functional>

class QListWidget;
class QLabel;

class WelcomeWindow final : public QDialog
{
public:
    explicit WelcomeWindow(QWidget *parent = nullptr);
    void setHandlers(std::function<void()> newDocument,
                     std::function<void()> start,
                     std::function<bool(const QString &)> openFile);
    void setSuiteHandler(std::function<void(int)> openSuite);
    void setRecentFileHandler(std::function<bool(const QString &, const QString &)> openRecentFile);
    void refreshRecentFiles();

private:
    QListWidget *recentList_;
    QLabel *emptyRecentLabel_;
    std::function<void()> newDocument_;
    std::function<void()> start_;
    std::function<bool(const QString &)> openFile_;
    std::function<void(int)> openSuite_;
    std::function<bool(const QString &, const QString &)> openRecentFile_;
    QStringList recentSnapshot_;

    void openPath(const QString &path, const QString &application);
};
